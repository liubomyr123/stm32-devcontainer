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

extern "C" void RadioTask(void* argument)
{
    (void)argument;
    LOG_INFO("RADIO", "Task started");

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
                   Direction::HalfDuplex};

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

    RadioState loopState = RadioState::TxMode;
    while (true)
    {
        LOG_INFO("RADIO", "Before goStandbyI() = %d", static_cast<int>(nrf.getCurrentState()));
        result = nrf.goStandbyI();
        if (!result)
        {
            LOG_ERROR("RADIO", "goStandbyI() failed unexpectedly");
            continue;
        }

        RadioState txState = nrf.getCurrentState();
        LOG_INFO("RADIO", "After goStandbyI() = %d (expect StandbyI = %d)",
                 static_cast<int>(txState), static_cast<int>(RadioState::StandbyI));

        switch (loopState)
        {
            case RadioState::TxMode:
            {
                PingPongCommand ping_pong_cmd{};
                ping_pong_cmd.counter = tx_counter;

                Aes128CcmCipher::Packet<PingPongCommand> packet{};
                if (!tx_cipher.encryptAndPack(ping_pong_cmd, packet))
                {
                    continue;
                }

                if (!nrf.transmit(packet.data(), static_cast<uint8_t>(packet.size())))
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
                Aes128CcmCipher::Packet<PingPongCommand> packet{};
                if (!nrf.receive(packet.data(), static_cast<uint8_t>(packet.size())))
                {
                    LOG_ERROR("RADIO", "receive() failed unexpectedly despite RX_DR set");
                    continue;
                }

                // "Write 1 to clear bit" (Table 28)
                nrf.writeRegister(REG_STATUS, STATUS_RX_DR_BIT);

                PingPongCommand received_cmd{};
                if (!rx_cipher.unpackAndDecrypt(packet, received_cmd))
                {
                    LOG_WARNING("RADIO", "unpackAndDecrypt failed — пакет відкинуто");
                    continue;
                }

                if (received_cmd.opcode != CommandOpcode::PingPong)
                {
                    LOG_WARNING("RADIO", "Unexpected opcode=%d",
                                static_cast<int>(received_cmd.opcode));
                    continue;
                }

                LOG_INFO("RADIO", "OK: RX_DR, received=%d", received_cmd.counter);
                tx_counter = received_cmd.counter + 1;
                LOG_INFO("RADIO", "Next tx_counter=%d", tx_counter);
                loopState = RadioState::TxMode;
            }
            if ((status & STATUS_TX_DS_BIT) != 0)
            {
                LOG_INFO("RADIO", "OK: TX_DS, sent=%d", tx_counter);
                nrf.writeRegister(REG_STATUS, STATUS_TX_DS_BIT);
                loopState = RadioState::RxMode;
            }
            continue;
        }

        LOG_ERROR("RADIO", "FAILED: no IRQ within %lums", IRQ_WAIT_TIMEOUT_MS);

        if (loopState == RadioState::TxMode)
        {
            nrf.sendCommand(FLUSH_TX);
        }
        if (loopState == RadioState::RxMode)
        {
            loopState = RadioState::TxMode;
        }
    }
}