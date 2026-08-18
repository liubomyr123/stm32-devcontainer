#include <array>

#include "cmsis_os.h"
#include "include/logger.hpp"
#include "include/nrf24radio.hpp"

extern SPI_HandleTypeDef hspi1;

#define NRF_CSN_PORT GPIOB
#define NRF_CSN_PIN GPIO_PIN_1

#define NRF_CE_PORT GPIOB
#define NRF_CE_PIN GPIO_PIN_0

// Тестове значення каналу для перевірки циклу write→read
constexpr uint8_t TEST_RF_CH_VALUE = 0b01001100;  // 76 → 2476 MHz

extern "C" void app_main()
{
    LOG_INFO("APP", "Started!");

    Nrf24Radio nrf{&hspi1, NRF_CSN_PORT, NRF_CSN_PIN, NRF_CE_PORT, NRF_CE_PIN};

    if (!nrf.init())
    {
        vTaskDelete(nullptr);
    }

    uint8_t configValue = nrf.readRegister(REG_CONFIG);
    uint8_t statusValue = nrf.readRegister(REG_STATUS);
    LOG_INFO("NRF", "CONFIG = 0x%02X (expect 0x%02X)", configValue, RESET_CONFIG);
    LOG_INFO("NRF", "STATUS = 0x%02X (expect 0x%02X)", statusValue, RESET_STATUS);

    uint8_t rfChBefore = nrf.readRegister(REG_RF_CH);
    LOG_INFO("NRF", "RF_CH (frequency channel) before write = %d (2400+%d = %d MHz, reset value)",
             rfChBefore, rfChBefore, 2400 + rfChBefore);

    nrf.writeRegister(REG_RF_CH, TEST_RF_CH_VALUE);
    uint8_t rfChAfter = nrf.readRegister(REG_RF_CH);
    LOG_INFO("NRF", "RF_CH (frequency channel) after write = %d (2400+%d = %d MHz)", rfChAfter,
             rfChAfter, 2400 + rfChAfter);

    nrf.writeRegister(REG_RF_CH,
                      RESET_RF_CH);  // повертаємо reset-значення для чистоти наступного тесту

    while (true)
    {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
        osDelay(250);
    }
}