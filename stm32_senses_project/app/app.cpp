#include <array>

#include "cmsis_os.h"
#include "include/logger.hpp"
#include "include/nrf24radio.hpp"

extern SPI_HandleTypeDef hspi1;

#define NRF_CSN_PORT GPIOB
#define NRF_CSN_PIN GPIO_PIN_1

#define NRF_CE_PORT GPIOB
#define NRF_CE_PIN GPIO_PIN_0

// Спільний RF-фільтр-ключ для комунікації пульт↔senses-плата (MVP, один канал)
constexpr std::array<uint8_t, 5> SHARED_RF_FILTER_KEY = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};

// Робочий канал — подалі від типових WiFi-каналів (1/6/11)
// F0 = 2400 + 100 = 2500 MHz
constexpr uint8_t SHARED_CHANNEL = 100;

constexpr uint8_t TEST_REG_RX_PW_P0_VALUE = 4;  // 4 bytes

extern "C" void app_main()
{
    LOG_INFO("APP", "Started!");

    Nrf24Radio nrf{&hspi1,                     //
                   NRF_CSN_PORT, NRF_CSN_PIN,  //
                   NRF_CE_PORT,  NRF_CE_PIN,   //
                   Direction::Rx};

    if (!nrf.init())
    {
        vTaskDelete(nullptr);
    }

    nrf.setAirDataRate(DataRate::Mbps1);
    nrf.setChannel(SHARED_CHANNEL);
    nrf.setRxRfFilterKey(SHARED_RF_FILTER_KEY);
    nrf.setRxPayloadLength(TEST_REG_RX_PW_P0_VALUE);

    LOG_INFO("NRF", "State before enableRx() = %d", static_cast<int>(nrf.getCurrentState()));
    nrf.enableRx();
    RadioState rxState = nrf.getCurrentState();
    LOG_INFO("NRF", "State after enableRx() = %d (expect RxMode = %d)", static_cast<int>(rxState),
             static_cast<int>(RadioState::RxMode));

    while (true)
    {
        uint8_t buffer[4];
        if (nrf.receive(buffer, sizeof(buffer)))
        {
            LOG_INFO("NRF", "Received: %d %d %d %d", buffer[0], buffer[1], buffer[2], buffer[3]);
        }

        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        osDelay(250);
    }
}