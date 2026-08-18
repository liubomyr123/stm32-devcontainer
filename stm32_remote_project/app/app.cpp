#include <array>

#include "cmsis_os.h"
#include "include/logger.hpp"

extern SPI_HandleTypeDef hspi1;

// Час, необхідний nRF24 на завершення внутрішнього power-on reset
// (datasheet, Table 14 "Power on reset", TPOR: max 100ms від моменту
// досягнення VDD 1.9V до готовності регістрів).
constexpr uint32_t NRF_POWER_ON_RESET_DELAY_MS = 100;

constexpr uint8_t R_REGISTER = 0x00;  // 000A AAAA — читання регістра
constexpr uint8_t W_REGISTER = 0x20;  // 001A AAAA — запис регістра

#define NRF_CSN_PORT GPIOB
#define NRF_CSN_PIN GPIO_PIN_1

#define NRF_CE_PORT GPIOB
#define NRF_CE_PIN GPIO_PIN_0

// Адреси регістрів nRF24 (Register Map, Table 28, datasheet)
constexpr uint8_t REG_CONFIG = 0x00;
constexpr uint8_t REG_STATUS = 0x07;
constexpr uint8_t REG_RF_CH = 0x05;

// Reset-значення регістрів (те, що очікуємо одразу після power-on,
// до будь-якої нашої конфігурації) — для звірки в тестах
constexpr uint8_t RESET_CONFIG = 0b00001000;  // EN_CRC=1, решта 0
constexpr uint8_t RESET_STATUS = 0b00001110;  // RX_P_NO=111 (RX FIFO Empty)
constexpr uint8_t RESET_RF_CH = 0b00000010;   // 2402 MHz (2400 + 2)

// Тестове значення каналу для перевірки циклу write→read
constexpr uint8_t TEST_RF_CH_VALUE = 0b01001100;  // 76 → 2476 MHz

// CSN опускається в low — чіп починає "слухати" SPI-шину.
// Кожна нова SPI-команда (за datasheet) обов'язково починається
// саме з high→low переходу на CSN.
void beginTransaction(GPIO_TypeDef* csPort, uint16_t csPin)
{
    HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_RESET);
}

// CSN піднімається в high — чіп "перестає слухати" SPI-шину,
// транзакція (команда + відповідь) вважається завершеною.
void endTransaction(GPIO_TypeDef* csPort, uint16_t csPin)
{
    HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_SET);
}

uint8_t readRegister(uint8_t reg)
{
    std::array<uint8_t, 2> txBuf = {static_cast<uint8_t>(R_REGISTER | reg), 0xFF};
    std::array<uint8_t, 2> rxBuf = {0, 0};

    beginTransaction(NRF_CSN_PORT, NRF_CSN_PIN);
    HAL_SPI_TransmitReceive(&hspi1,
                            txBuf.data(),  //
                            rxBuf.data(),  //
                            txBuf.size(),  //
                            HAL_MAX_DELAY);
    endTransaction(NRF_CSN_PORT, NRF_CSN_PIN);

    return rxBuf[1];
}

uint8_t writeRegister(uint8_t reg, uint8_t value)
{
    std::array<uint8_t, 2> txBuf = {static_cast<uint8_t>(W_REGISTER | reg), value};
    std::array<uint8_t, 2> rxBuf = {0, 0};

    beginTransaction(NRF_CSN_PORT, NRF_CSN_PIN);
    HAL_SPI_TransmitReceive(&hspi1,
                            txBuf.data(),  //
                            rxBuf.data(),  //
                            txBuf.size(),  //
                            HAL_MAX_DELAY);
    endTransaction(NRF_CSN_PORT, NRF_CSN_PIN);

    return rxBuf[0];
}

extern "C" void app_main()
{
    LOG_INFO("APP", "Started!");

    osDelay(NRF_POWER_ON_RESET_DELAY_MS);

    uint8_t configValue = readRegister(REG_CONFIG);
    uint8_t statusValue = readRegister(REG_STATUS);
    LOG_INFO("NRF", "CONFIG = 0x%02X (expect 0x%02X)", configValue, RESET_CONFIG);
    LOG_INFO("NRF", "STATUS = 0x%02X (expect 0x%02X)", statusValue, RESET_STATUS);

    uint8_t rfChBefore = readRegister(REG_RF_CH);
    LOG_INFO("NRF", "RF_CH (frequency channel) before write = %d (2400+%d = %d MHz, reset value)",
             rfChBefore, rfChBefore, 2400 + rfChBefore);

    writeRegister(REG_RF_CH, TEST_RF_CH_VALUE);
    uint8_t rfChAfter = readRegister(REG_RF_CH);
    LOG_INFO("NRF", "RF_CH (frequency channel) after write = %d (2400+%d = %d MHz)", rfChAfter,
             rfChAfter, 2400 + rfChAfter);

    writeRegister(REG_RF_CH,
                  RESET_RF_CH);  // повертаємо reset-значення для чистоти наступного тесту

    while (true)
    {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
        osDelay(250);
    }
}