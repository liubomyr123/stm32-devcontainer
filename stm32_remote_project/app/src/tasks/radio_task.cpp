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

// constexpr std::array<uint8_t, Aes128CcmCipher::aes_key_length> TX_KEY = {
//     'M', 'y', 'S', 'e', 'c', 'r', 'e', 't',  //
//     'T', 'e', 's', 't', 'K', 'e', 'y', '1'   //
// };

// constexpr std::array<uint8_t, Aes128CcmCipher::aes_key_length> RX_KEY = {
//     'M', 'y', 'S', 'e', 'c', 'r', 'e', 't',  //
//     'T', 'e', 's', 't', 'K', 'e', 'y', '2'   //
// };

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
    SEARCHING,
    BROADCASTING,
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

    uint32_t device_id = getDeviceId();
    SoftRandom rng(device_id ^ HAL_GetTick());
    uint32_t device_random = rng.next();

    // Aes128CcmCipher tx_cipher(TX_KEY.data());
    // Aes128CcmCipher rx_cipher(RX_KEY.data());

    // if (!tx_cipher.init())
    // {
    //     vTaskDelete(nullptr);
    // }
    // if (!rx_cipher.init())
    // {
    //     vTaskDelete(nullptr);
    // }

    Aes128CcmCipher tx_cipher;
    Aes128CcmCipher rx_cipher;

    uint8_t tx_counter = 0;

    Nrf24Radio nrf{&hspi1,  //
                   NRF_CSN_PORT,
                   NRF_CSN_PIN,  //
                   NRF_CE_PORT,
                   NRF_CE_PIN,  //
                   Direction::HalfDuplex,
                   device_id,
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

    Role radio_state = Role::TxMode;
    LoopState loop_state = LoopState::IDLE;
    uint8_t idle_count = 0;

    // constexpr size_t CAR_ID_LIST_SIZE = 4;
    // std::array<uint32_t, CAR_ID_LIST_SIZE> car_id_list{};
    // unsigned int car_id_list_length = 0;

    constexpr uint32_t CAR_ID_TO_CONNECT = 1;
    bool found_target_car = false;

    std::array<uint8_t, 16> tx_packet{};
    std::array<uint8_t, 16> rx_packet{};

    while (true)
    {
        switch (loop_state)
        {
            case LoopState::IDLE:
            {
                if (idle_count == 5)  // Just for testing
                {
                    if (radio_state == Role::TxMode)
                    {
                        loop_state = LoopState::SEARCHING;
                    }
                    else if (radio_state == Role::RxMode)
                    {
                        loop_state = LoopState::BROADCASTING;
                    }
                }
                else
                {
                    idle_count++;
                    osDelay(1000);
                }
                continue;
            }
            case LoopState::SEARCHING:  // TxMode
            {
                tx_packet.fill(0);
                rx_packet.fill(0);

                radio_state = Role::RxMode;

                break;
            }
            case LoopState::BROADCASTING:  // RxMode
            {
                tx_packet.fill(0);
                rx_packet.fill(0);

                break;
            }
            case LoopState::BIND_REQ:
            {
                tx_packet.fill(0);
                rx_packet.fill(0);

                if (!found_target_car)
                {
                    radio_state = Role::RxMode;
                    loop_state = LoopState::SEARCHING;
                    continue;
                }

                BindReqCommand bind_req_cmd{};
                bind_req_cmd.remoteId = nrf.getDeviceId();
                bind_req_cmd.remoteRandom = nrf.getDeviceRandom();
                bind_req_cmd.targetCarId = CAR_ID_TO_CONNECT;

                tx_packet[0] = static_cast<uint8_t>(FrameType::Plain);
                std::memcpy(tx_packet.data() + 1, &bind_req_cmd, sizeof(bind_req_cmd));
                break;
            }
            case LoopState::BIND_RESP:
            {
                tx_packet.fill(0);
                rx_packet.fill(0);
                break;
            }
            case LoopState::BIND_CONFIRM:
            {
                tx_packet.fill(0);
                rx_packet.fill(0);

                BindConfirmCommand bind_confirm_cmd{};

                tx_packet[0] = static_cast<uint8_t>(FrameType::Encrypted);
                std::memcpy(tx_packet.data() + 1, &bind_confirm_cmd, sizeof(bind_confirm_cmd));
                break;
            }
            case LoopState::BIND_ACK:
            {
                tx_packet.fill(0);
                rx_packet.fill(0);
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

        switch (radio_state)
        {
            case Role::TxMode:
            {
                auto frame_type = static_cast<FrameType>(tx_packet.at(0));

                switch (frame_type)
                {
                    case FrameType::Plain:
                    {
                        if (!nrf.transmit(tx_packet.data(), static_cast<uint8_t>(tx_packet.size())))
                        {
                            continue;
                        }
                        break;
                    }
                    case FrameType::Encrypted:
                    {
                        std::array<uint8_t, 16> encrypted_tx_packet{};
                        encrypted_tx_packet[0] = static_cast<uint8_t>(FrameType::Encrypted);

                        if (!tx_cipher.encrypt(tx_packet.data() + sizeof(FrameType),
                                               tx_packet.size() - sizeof(FrameType),
                                               encrypted_tx_packet.data() + sizeof(FrameType)))
                        {
                            continue;
                        }
                        if (!nrf.transmit(encrypted_tx_packet.data(),
                                          static_cast<uint8_t>(encrypted_tx_packet.size())))
                        {
                            continue;
                        }
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
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
            case Role::RxMode:
            {
                if (!nrf.startRx())
                {
                    continue;
                }
                break;
            }
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

                switch (radio_state)
                {
                    case Role::TxMode:
                    {
                        break;
                    }
                    case Role::RxMode:
                    {
                        switch (loop_state)
                        {
                            case LoopState::IDLE:
                            {
                                break;
                            }
                            case LoopState::SEARCHING:
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
                                            case CommandOpcode::CarId:
                                            {
                                                CarIdCommand bind_res_cmd{};
                                                std::memcpy(&bind_res_cmd,
                                                            rx_packet.data() + sizeof(FrameType),
                                                            sizeof(CarIdCommand));
                                                LOG_INFO("RADIO", "car_id=%d", bind_res_cmd.carId);
                                                if (bind_res_cmd.carId == CAR_ID_TO_CONNECT)
                                                {
                                                    found_target_car = true;
                                                    radio_state = Role::TxMode;
                                                    loop_state = LoopState::BIND_REQ;
                                                }

                                                // if (car_id_list_length < 4)
                                                // {
                                                //     bool found = false;
                                                //     for (size_t i = 0; i < CAR_ID_LIST_SIZE; i++)
                                                //     {
                                                //         if (car_id_list.at(i) ==
                                                //         bind_res_cmd.carId)
                                                //         {
                                                //             found = true;
                                                //             break;
                                                //         }
                                                //     }
                                                //     if (!found)
                                                //     {
                                                //         car_id_list.at(car_id_list_length) =
                                                //             bind_res_cmd.carId;
                                                //         car_id_list_length++;
                                                //     }
                                                // }

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
                            case LoopState::BIND_RESP:
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

                                                if (bind_res_cmd.remoteRandomEcho != device_id)
                                                {
                                                    // Error
                                                    break;
                                                }

                                                // crating key_remote_to_car and init cipher
                                                //  [remote_id 4][car_id 4][R1 4][R2 4], де блок[0]
                                                //  ^= маркер напрямку:

                                                std::array<uint8_t, 16> aes_key{};
                                                // 4
                                                std::memcpy(aes_key.data(), &device_id,
                                                            sizeof(device_id));
                                                // 4
                                                std::memcpy(aes_key.data() + sizeof(device_id),
                                                            rx_packet.data() + sizeof(FrameType) +
                                                                sizeof(CommandOpcode) +
                                                                sizeof(BindRespCommand::carId),
                                                            sizeof(BindRespCommand::carId));
                                                // 4
                                                std::memcpy(aes_key.data() + sizeof(device_id) +
                                                                sizeof(BindRespCommand::carId),
                                                            rx_packet.data() + sizeof(FrameType) +
                                                                sizeof(CommandOpcode) +
                                                                sizeof(BindRespCommand::carId) +
                                                                sizeof(BindRespCommand::carRandom),
                                                            sizeof(BindRespCommand::carRandom));
                                                // 4
                                                std::memcpy(aes_key.data() + sizeof(device_id) +
                                                                sizeof(BindRespCommand::carId) +
                                                                sizeof(BindRespCommand::carRandom),
                                                            &device_random, sizeof(device_random));

                                                {
                                                    aes_key.at(0) ^= 1;
                                                    rx_cipher.setAesKey(aes_key.data());

                                                    if (!rx_cipher.init())
                                                    {
                                                        vTaskDelete(nullptr);
                                                    }
                                                }
                                                {
                                                    aes_key.at(0) ^= 0;
                                                    tx_cipher.setAesKey(aes_key.data());

                                                    if (!tx_cipher.init())
                                                    {
                                                        vTaskDelete(nullptr);
                                                    }
                                                }
                                                radio_state = Role::TxMode;
                                                loop_state = LoopState::BIND_CONFIRM;
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
                            case LoopState::BIND_CONFIRM:
                            {
                                break;
                            }
                            case LoopState::BIND_ACK:
                            {
                                auto frame_type = static_cast<FrameType>(rx_packet.at(0));
                                switch (frame_type)
                                {
                                    case FrameType::Plain:
                                    {
                                        // Error, we expect encrypted type of packet
                                        break;
                                    }
                                    case FrameType::Encrypted:
                                    {
                                        CommandOpcode opcode{};
                                        std::memcpy(&opcode, rx_packet.data() + sizeof(FrameType),
                                                    sizeof(opcode));

                                        switch (opcode)
                                        {
                                            case CommandOpcode::BindAck:
                                            {
                                                std::array<uint8_t, Aes128CcmCipher::nonce_length>
                                                    nonce{};
                                                std::memcpy(nonce.data(),
                                                            rx_packet.data() + sizeof(FrameType),
                                                            Aes128CcmCipher::nonce_length);

                                                std::array<uint8_t, sizeof(BindAckCommand)>
                                                    encrypted_data{};
                                                std::memcpy(encrypted_data.data(),
                                                            rx_packet.data() + sizeof(FrameType) +
                                                                Aes128CcmCipher::nonce_length,
                                                            sizeof(BindAckCommand));

                                                std::array<uint8_t, Aes128CcmCipher::tag_length>
                                                    tag{};
                                                std::memcpy(tag.data(),
                                                            rx_packet.data() +
                                                                Aes128CcmCipher::nonce_length +
                                                                sizeof(FrameType) +
                                                                sizeof(BindAckCommand),
                                                            Aes128CcmCipher::tag_length);

                                                std::array<uint8_t, sizeof(BindAckCommand)>
                                                    plain_data{};
                                                if (!rx_cipher.decrypt(nonce,                  //
                                                                       encrypted_data.data(),  //
                                                                       encrypted_data.size(),  //
                                                                       tag,                    //
                                                                       plain_data.data()))
                                                {
                                                    continue;
                                                }

                                                BindAckCommand bind_ack_cmd{};
                                                std::memcpy(&bind_ack_cmd,
                                                            rx_packet.data() + sizeof(FrameType),
                                                            sizeof(BindAckCommand));
                                                LOG_INFO("RADIO", "BindAckCommand");
                                                loop_state = LoopState::PING_PONG;
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
                                    default:
                                    {
                                        // Error, we expect plain type of packet
                                        break;
                                    }
                                }
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
                    default:
                    {
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
                // radio_state = Role::TxMode;
            }
            if ((status & STATUS_TX_DS_BIT) != 0)
            {
                LOG_INFO("RADIO", "OK: TX_DS, sent=%d", tx_counter);
                nrf.writeRegister(REG_STATUS, STATUS_TX_DS_BIT);
                switch (loop_state)
                {
                    case LoopState::IDLE:
                    {
                        break;
                    }
                    case LoopState::BIND_REQ:
                    {
                        loop_state = LoopState::BIND_RESP;
                        break;
                    }
                    case LoopState::BIND_CONFIRM:
                    {
                        loop_state = LoopState::BIND_ACK;
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
                radio_state = Role::RxMode;
            }
            continue;
        }

        LOG_ERROR("RADIO", "FAILED: no IRQ within %lums", IRQ_WAIT_TIMEOUT_MS);

        if (radio_state == Role::TxMode)
        {
            nrf.sendCommand(FLUSH_TX);
        }
        if (radio_state == Role::RxMode)
        {
            radio_state = Role::TxMode;
        }
    }
}