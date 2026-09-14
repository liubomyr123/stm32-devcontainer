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
constexpr uint32_t FRAME_INTERVAL_MS = 50;

// Спільний RF-фільтр-ключ для комунікації пульт↔senses-плата (MVP, один канал)
constexpr std::array<uint8_t, 5> SHARED_RF_FILTER_KEY = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};

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

    Nrf24Radio nrf{&hspi1,                     //
                   NRF_CSN_PORT, NRF_CSN_PIN,  //
                   NRF_CE_PORT,  NRF_CE_PIN,   //
                   Direction::Tx};

    if (!nrf.init())
    {
        vTaskDelete(nullptr);
    }

    bool result = nrf.enableAutoAck(EN_AA_P0_BIT);
    if (!result)
    {
        vTaskDelete(nullptr);
    }
    result = nrf.setAirDataRate(DataRate::Mbps1);
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

    LOG_INFO("RADIO", "State before enableTx() = %d", static_cast<int>(nrf.getCurrentState()));
    result = nrf.enableTx();
    if (!result)
    {
        vTaskDelete(nullptr);
    }

    RadioState txState = nrf.getCurrentState();
    LOG_INFO("RADIO", "State after enableTx() = %d (expect StandbyII = %d)",
             static_cast<int>(txState), static_cast<int>(RadioState::StandbyII));

    uint32_t counter = 0;

    while (true)
    {
        uint8_t buffer[4] = {
            static_cast<uint8_t>((counter >> 24) & 0xFF),
            static_cast<uint8_t>((counter >> 16) & 0xFF),
            static_cast<uint8_t>((counter >> 8) & 0xFF),
            static_cast<uint8_t>(counter & 0xFF),
        };

        counter++;

        if (!nrf.transmit(buffer, sizeof(buffer)))
        {
            osDelay(FRAME_INTERVAL_MS);
            continue;
        }

        uint32_t flags = osThreadFlagsWait(NRF24_IRQ_FLAG, osFlagsWaitAny, IRQ_WAIT_TIMEOUT_MS);
        if (flags == NRF24_IRQ_FLAG)
        {
            uint8_t status = nrf.readRegister(REG_STATUS);

            if ((status & STATUS_RX_DR_BIT) != 0)
            {
                LOG_INFO("RADIO", "OK: RX_DR (received data ready)");
            }
            if ((status & STATUS_TX_DS_BIT) != 0)
            {
                LOG_INFO("RADIO", "OK: TX_DS (packet #%lu delivered)", counter - 1);
            }
            /**
                Maximum number of TX retransmits interrupt
                Write 1 to clear bit. If MAX_RT is asserted it must
                be cleared to enable further communication.
            */
            if ((status & STATUS_MAX_RT_BIT) != 0)
            {
                LOG_ERROR("RADIO", "FAILED: MAX_RT (packet #%lu lost) — recovering", counter - 1);
                nrf.sendCommand(FLUSH_TX);
            }

            nrf.writeRegister(REG_STATUS, status);
        }
        else
        {
            LOG_ERROR("RADIO", "FAILED: no IRQ within %lums (packet #%lu) — recovering",
                      IRQ_WAIT_TIMEOUT_MS, counter - 1);
            nrf.sendCommand(FLUSH_TX);
        }

        osDelay(FRAME_INTERVAL_MS);
    }
}