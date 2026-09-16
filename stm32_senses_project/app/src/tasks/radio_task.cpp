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

constexpr uint8_t PAYLOAD_LENGTH = 4;

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
                   Direction::Rx};

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
    result = nrf.setRxRfFilterKey(SHARED_RF_FILTER_KEY);
    if (!result)
    {
        vTaskDelete(nullptr);
    }
    result = nrf.setRxPayloadLength(PAYLOAD_LENGTH);
    if (!result)
    {
        vTaskDelete(nullptr);
    }

    LOG_INFO("RADIO", "State before enableRx() = %d", static_cast<int>(nrf.getCurrentState()));
    result = nrf.enableRx();
    if (!result)
    {
        vTaskDelete(nullptr);
    }

    RadioState rxState = nrf.getCurrentState();
    LOG_INFO("RADIO", "State after enableRx() = %d (expect RxMode = %d)", static_cast<int>(rxState),
             static_cast<int>(RadioState::RxMode));

    while (true)
    {
        uint32_t flags = osThreadFlagsWait(NRF24_IRQ_FLAG, osFlagsWaitAny, osWaitForever);

        if (flags == NRF24_IRQ_FLAG)
        {
            uint8_t status = nrf.readRegister(REG_STATUS);

            if ((status & STATUS_TX_DS_BIT) != 0)
            {
                LOG_WARNING("RADIO", "OK: TX_DS");
            }
            if ((status & STATUS_MAX_RT_BIT) != 0)
            {
                LOG_WARNING("RADIO", "OK: MAX_RT");
            }

            if ((status & STATUS_RX_DR_BIT) != 0)
            {
                uint8_t buffer[PAYLOAD_LENGTH];
                if (!nrf.receive(buffer, sizeof(buffer)))
                {
                    LOG_ERROR("RADIO", "FAILED: receive()");
                    continue;
                }
                LOG_INFO("RADIO", "OK: RX_DR — received %d %d %d %d", buffer[0], buffer[1],
                         buffer[2], buffer[3]);
            }

            nrf.writeRegister(REG_STATUS, status);
        }
    }
}