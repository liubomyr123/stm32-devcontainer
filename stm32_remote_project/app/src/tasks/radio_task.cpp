#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "../include/aes128ccm_cipher.hpp"
#include "../include/commands.hpp"
#include "../include/logger.hpp"
#include "../include/nrf24radio.hpp"
#include "cmsis_os2.h"

extern SPI_HandleTypeDef hspi1;

#define NRF_CSN_PORT GPIOB
#define NRF_CSN_PIN GPIO_PIN_1

#define NRF_CE_PORT GPIOB
#define NRF_CE_PIN GPIO_PIN_0

#define NRF24_IRQ_PIN GPIO_PIN_10
#define NRF24_IRQ_FLAG (1UL << 0)

constexpr uint32_t IRQ_WAIT_TIMEOUT_MS = 50;

// Спільний RF-фільтр-ключ для комунікації пульт↔senses-плата (MVP, один канал)
constexpr std::array<uint8_t, 5> SHARED_RF_FILTER_KEY = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};

constexpr std::array<uint8_t, Aes128CcmCipher::aes_key_length> TX_KEY = {
    'M', 'y', 'S', 'e', 'c', 'r', 'e', 't',  //
    'T', 'e', 's', 't', 'K', 'e', 'y', '1'   //
};

constexpr std::array<uint8_t, Aes128CcmCipher::aes_key_length> RX_KEY = {
    'M', 'y', 'S', 'e', 'c', 'r', 'e', 't',  //
    'T', 'e', 's', 't', 'K', 'e', 'y', '2'   //
};

// Робочий канал — подалі від типових WiFi-каналів (1/6/11)
// F0 = 2400 + 100 = 2500 MHz
constexpr uint8_t SHARED_CHANNEL = 100;

extern osThreadId_t RadioTaskHandle;

extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == NRF24_IRQ_PIN)
    {
        osThreadFlagsSet(RadioTaskHandle, NRF24_IRQ_FLAG);
    }
}

inline uint32_t getDeviceId()
{
    const std::array<uint32_t, 3> words = {HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2()};

    uint32_t hash = 2166136261U;  // FNV offset basis
    for (uint32_t word : words)
    {
        for (uint32_t i = 0; i < 4; ++i)
        {
            hash ^= (word >> (8 * i)) & 0xFFU;
            hash *= 16777619U;  // FNV prime
        }
    }
    return hash;
}

class SoftRandom
{
   public:
    explicit SoftRandom(uint32_t seed) : state_(seed != 0 ? seed : 0x6D2B79F5U)
    {
    }

    uint32_t next()
    {
        state_ ^= state_ << 13;
        state_ ^= state_ >> 17;
        state_ ^= state_ << 5;
        return state_;
    }

   private:
    uint32_t state_;
};

enum class LoopState
{
    IDLE,
    BIND_REQ,
    BIND_RESP,
    BIND_CONFIRM,
    BIND_ACK,
    PING_PONG,
};

extern "C" void RadioTask(void* argument)
{
    (void)argument;
    LOG_INFO("RADIO", "Task started");

    SoftRandom rng(getDeviceId() ^ HAL_GetTick());

    Aes128CcmCipher tx_cipher(TX_KEY.data());
    Aes128CcmCipher rx_cipher(RX_KEY.data());

    if (!tx_cipher.init())
    {
        vTaskDelete(nullptr);
    }
    if (!rx_cipher.init())
    {
        vTaskDelete(nullptr);
    }

    uint8_t tx_counter = 0;

    Nrf24Radio nrf{&hspi1,  //
                   NRF_CSN_PORT,
                   NRF_CSN_PIN,  //
                   NRF_CE_PORT,
                   NRF_CE_PIN,  //
                   Direction::HalfDuplex,
                   getDeviceId(),
                   rng.next()};

    if (!nrf.init())
    {
        vTaskDelete(nullptr);
    }

    bool result = nrf.setAirDataRate(DataRate::Mbps1);
    if (!result)
    {
        vTaskDelete(nullptr);
    }
    result = nrf.setChannel(SHARED_CHANNEL);
    if (!result)
    {
        vTaskDelete(nullptr);
    }
    result = nrf.setTxRfFilterKey(SHARED_RF_FILTER_KEY);
    if (!result)
    {
        vTaskDelete(nullptr);
    }

    result = nrf.setRxRfFilterKey(SHARED_RF_FILTER_KEY);
    if (!result)
    {
        vTaskDelete(nullptr);
    }

    result = nrf.setRxPayloadLength(
        static_cast<uint8_t>(sizeof(Aes128CcmCipher::Packet<PingPongCommand>)));
    if (!result)
    {
        vTaskDelete(nullptr);
    }

    RadioState state = RadioState::TxMode;
    LoopState loopState = LoopState::IDLE;
    while (true)
    {
        /*
         * 1) BIND_REQ      пульт -> машинка    відкритий
         *      [прапорець 0xA1][опкод 1][remote_id 4][R1 4]                        = 9 байт
         *
         * 2) BIND_RESP     машинка -> пульт    відкритий
         *      [прапорець 0xA1][опкод 1][car_id 4][R2 4][ехо R1 4]                 = 13 байт
         *
         * 3) BIND_CONFIRM  пульт -> машинка    зашифрований ключем key_remote_to_car
         *      [прапорець 0xA2][nonce 7] [опкод 1] [tag 4]                         = 7 + 1 + 4 = 12
         * байт
         *
         * 4) BIND_ACK      машинка -> пульт    зашифрований ключем key_car_to_remote
         *      [прапорець 0xA2][nonce 7] [опкод 1] [tag 4]                         = 7 + 1 + 4 = 12
         * байт
         *
         * 5) PING_PONG     в обидва боки       зашифрований ключем свого напрямку
         *      [прапорець 0xA2][nonce 7] [опкод 1][counter 1] [tag 4]              = 7 + 2 + 4 = 13
         * байт
         */
        std::array<uint8_t, 16> tx_packet{};
        std::array<uint8_t, 16> rx_packet{};
        switch (loopState)
        {
            case LoopState::IDLE:
            {
                BindReqCommand bind_req_cmd{};
                bind_req_cmd.remoteId = nrf.getDeviceId();
                bind_req_cmd.remoteRandom = nrf.getDeviceRandom();

                tx_packet[0] = static_cast<uint8_t>(FrameType::Plain);
                std::memcpy(tx_packet.data() + 1, &bind_req_cmd, sizeof(bind_req_cmd));
                break;
            }
            case LoopState::BIND_REQ:
            {
                break;
            }
            case LoopState::BIND_CONFIRM:
            {
                break;
            }
            case LoopState::BIND_ACK:
            {
                break;
            }
            case LoopState::PING_PONG:
            {
                break;
            }
            default:
            {
                break;
            }
        }

        result = nrf.goStandbyI();
        if (!result)
        {
            LOG_ERROR("RADIO", "goStandbyI() failed unexpectedly");
            continue;
        }

        switch (state)
        {
            case RadioState::TxMode:
            {
                // PingPongCommand ping_pong_cmd{};
                // ping_pong_cmd.counter = tx_counter;

                // Aes128CcmCipher::Packet<PingPongCommand> packet{};
                // if (!tx_cipher.encryptAndPack(ping_pong_cmd, packet))
                // {
                //     continue;
                // }

                if (!nrf.transmit(tx_packet.data(), static_cast<uint8_t>(tx_packet.size())))
                {
                    continue;
                }

                if (!nrf.startTx())
                {
                    continue;
                }
                break;
            }
            case RadioState::RxMode:
            {
                if (!nrf.startRx())
                {
                    continue;
                }
                break;
            }
            case RadioState::Unknown:
            case RadioState::StandbyI:
            case RadioState::StandbyII:
            case RadioState::PowerDown:
            default:
                break;
        }

        uint32_t flags = osThreadFlagsWait(NRF24_IRQ_FLAG, osFlagsWaitAny, IRQ_WAIT_TIMEOUT_MS);
        if (flags == NRF24_IRQ_FLAG)
        {
            uint8_t status = nrf.readRegister(REG_STATUS);

            if ((status & STATUS_RX_DR_BIT) != 0)
            {
                if (!nrf.receive(rx_packet.data(), static_cast<uint8_t>(rx_packet.size())))
                {
                    LOG_ERROR("RADIO", "receive() failed unexpectedly despite RX_DR set");
                    continue;
                }

                switch (state)
                {
                    case RadioState::TxMode:
                    {
                        switch (loopState)
                        {
                            case LoopState::IDLE:
                            {
                                auto frame_type = static_cast<FrameType>(rx_packet.at(0));
                                switch (frame_type)
                                {
                                    case FrameType::Plain:
                                    {
                                        CommandOpcode opcode{};
                                        std::memcpy(&opcode, rx_packet.data() + sizeof(FrameType),
                                                    sizeof(opcode));

                                        switch (opcode)
                                        {
                                            case CommandOpcode::BindResp:
                                            {
                                                BindRespCommand bind_res_cmd{};
                                                std::memcpy(&bind_res_cmd,
                                                            rx_packet.data() + sizeof(FrameType),
                                                            sizeof(BindRespCommand));
                                                LOG_INFO("RADIO",
                                                         "car_id=%d | carRandom=%d | "
                                                         "remoteRandomEcho=%d",
                                                         bind_res_cmd.carId, bind_res_cmd.carRandom,
                                                         bind_res_cmd.remoteRandomEcho);
                                                break;
                                            }
                                            default:
                                            {
                                                //
                                                break;
                                            }
                                        }
                                        break;
                                    }
                                    case FrameType::Encrypted:
                                    {
                                        // Error, we expect plain type of packet
                                        break;
                                    }
                                    default:
                                    {
                                        // Error, we expect plain type of packet
                                        break;
                                    }
                                }

                                break;
                            }
                            case LoopState::BIND_REQ:
                            {
                                break;
                            }
                            case LoopState::BIND_CONFIRM:
                            {
                                break;
                            }
                            case LoopState::BIND_ACK:
                            {
                                break;
                            }
                            case LoopState::PING_PONG:
                            {
                                break;
                            }
                            default:
                            {
                                break;
                            }
                        }
                        break;
                    }
                }

                // Aes128CcmCipher::Packet<PingPongCommand> packet{};
                // if (!nrf.receive(packet.data(), static_cast<uint8_t>(packet.size())))
                // {
                //     LOG_ERROR("RADIO", "receive() failed unexpectedly despite RX_DR set");
                //     continue;
                // }

                // "Write 1 to clear bit" (Table 28)
                nrf.writeRegister(REG_STATUS, STATUS_RX_DR_BIT);

                // PingPongCommand received_cmd{};
                // if (!rx_cipher.unpackAndDecrypt(packet, received_cmd))
                // {
                //     LOG_WARNING("RADIO", "unpackAndDecrypt failed — пакет відкинуто");
                //     continue;
                // }

                // if (received_cmd.opcode != CommandOpcode::PingPong)
                // {
                //     LOG_WARNING("RADIO", "Unexpected opcode=%d",
                //                 static_cast<int>(received_cmd.opcode));
                //     continue;
                // }

                // LOG_INFO("RADIO", "OK: RX_DR, received=%d", received_cmd.counter);
                // tx_counter = received_cmd.counter + 1;
                // LOG_INFO("RADIO", "Next tx_counter=%d", tx_counter);
                state = RadioState::TxMode;
            }
            if ((status & STATUS_TX_DS_BIT) != 0)
            {
                LOG_INFO("RADIO", "OK: TX_DS, sent=%d", tx_counter);
                nrf.writeRegister(REG_STATUS, STATUS_TX_DS_BIT);
                state = RadioState::RxMode;
            }
            continue;
        }

        LOG_ERROR("RADIO", "FAILED: no IRQ within %lums", IRQ_WAIT_TIMEOUT_MS);

        if (state == RadioState::TxMode)
        {
            nrf.sendCommand(FLUSH_TX);
        }
        if (state == RadioState::RxMode)
        {
            state = RadioState::TxMode;
        }
    }
}