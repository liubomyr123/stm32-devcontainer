#include "nrf24radio.hpp"

bool Nrf24Radio::init()
{
    HAL_GPIO_WritePin(cePort_, cePin_, GPIO_PIN_RESET);
    osDelay(NRF_POWER_ON_RESET_DELAY_MS);

    // Найнадійніший спосіб перевірити реальну відповідь чіпа (немає
    // окремого WHO_AM_I регістра, як у MPU6050) — записати довільне
    // тестове значення в "безпечний" регістр (RF_CH, не впливає на
    // живлення/режими) і перевірити, чи прочитається те саме назад.
    // Якщо SPI floating/не підключено — назад прийде "сміття",
    // яке не збіжиться з записаним значенням.
    constexpr uint8_t PROBE_VALUE = 0b00010101;  // довільне тестове значення

    writeRegister(REG_RF_CH, PROBE_VALUE);
    uint8_t readBack = readRegister(REG_RF_CH);

    if (readBack != PROBE_VALUE)
    {
        LOG_ERROR(TAG, "nRF24 not responding (wrote 0x%02X, read back 0x%02X)", PROBE_VALUE,
                  readBack);
        return false;
    }

    writeRegister(REG_RF_CH, RESET_RF_CH);  // повертаємо чистий стан

    return true;
}

// CSN опускається в low — чіп починає "слухати" SPI-шину.
// Кожна нова SPI-команда (за datasheet) обов'язково починається
// саме з high→low переходу на CSN.
void Nrf24Radio::beginTransaction()
{
    HAL_GPIO_WritePin(csnPort_, csnPin_, GPIO_PIN_RESET);
}

// CSN піднімається в high — чіп "перестає слухати" SPI-шину,
// транзакція (команда + відповідь) вважається завершеною.
void Nrf24Radio::endTransaction()
{
    HAL_GPIO_WritePin(csnPort_, csnPin_, GPIO_PIN_SET);
}

uint8_t Nrf24Radio::readRegister(uint8_t reg)
{
    std::array<uint8_t, 2> txBuf = {static_cast<uint8_t>(R_REGISTER | reg), 0xFF};
    std::array<uint8_t, 2> rxBuf = {0, 0};

    beginTransaction();
    HAL_SPI_TransmitReceive(hspi_,
                            txBuf.data(),  //
                            rxBuf.data(),  //
                            txBuf.size(),  //
                            HAL_MAX_DELAY);
    endTransaction();

    return rxBuf[1];
}

uint8_t Nrf24Radio::writeRegister(uint8_t reg, uint8_t value)
{
    std::array<uint8_t, 2> txBuf = {static_cast<uint8_t>(W_REGISTER | reg), value};
    std::array<uint8_t, 2> rxBuf = {0, 0};

    beginTransaction();
    HAL_SPI_TransmitReceive(hspi_,
                            txBuf.data(),  //
                            rxBuf.data(),  //
                            txBuf.size(),  //
                            HAL_MAX_DELAY);
    endTransaction();

    return rxBuf[0];
}
