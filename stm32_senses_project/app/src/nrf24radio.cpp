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
void Nrf24Radio::beginTransaction() const
{
    HAL_GPIO_WritePin(csnPort_, csnPin_, GPIO_PIN_RESET);
}

// CSN піднімається в high — чіп "перестає слухати" SPI-шину,
// транзакція (команда + відповідь) вважається завершеною.
void Nrf24Radio::endTransaction() const
{
    HAL_GPIO_WritePin(csnPort_, csnPin_, GPIO_PIN_SET);
}

uint8_t Nrf24Radio::readRegister(uint8_t reg) const
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

uint8_t Nrf24Radio::writeRegister(uint8_t reg, uint8_t value) const
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

// Записує багатобайтний R_REGISTER-регістр (5-байтну адресу — RX_ADDR_P0, RX_ADDR_P1 чи TX_ADDR)
uint8_t Nrf24Radio::writeMultiByteRegister(uint8_t reg, const uint8_t* data, size_t length)
{
    std::array<uint8_t, 6> txBuf{};
    txBuf[0] = static_cast<uint8_t>(W_REGISTER | reg);
    for (size_t i = 0; i < length; i++)
    {
        txBuf.at(i + 1) = data[i];
    }

    std::array<uint8_t, 6> rxBuf{};

    beginTransaction();
    HAL_SPI_TransmitReceive(hspi_,                              //
                            txBuf.data(),                       //
                            rxBuf.data(),                       //
                            static_cast<uint16_t>(length + 1),  //
                            HAL_MAX_DELAY);
    endTransaction();

    return rxBuf[0];
}

// CE піднімається в high — при увімкненому TX FIFO чіп автоматично
// почне передачу щойно з'явиться новий payload (Standby-II mode, datasheet §6.1.5).
void Nrf24Radio::ceHigh()
{
    HAL_GPIO_WritePin(cePort_, cePin_, GPIO_PIN_SET);
}

// CE опускається в low — чіп повертається в Standby-I, передача зупиняється.
void Nrf24Radio::ceLow()
{
    HAL_GPIO_WritePin(cePort_, cePin_, GPIO_PIN_RESET);
}

// bool Nrf24Radio::isTxEnabled() const
// {
//     return mode_ == Mode::Tx;
// }

// bool Nrf24Radio::isRxEnabled() const
// {
//     return mode_ == Mode::Rx;
// }

bool Nrf24Radio::isTxEnabled() const
{
    RadioState state = getCurrentState();
    return state == RadioState::TxMode || state == RadioState::StandbyII;
}

bool Nrf24Radio::isRxEnabled() const
{
    return getCurrentState() == RadioState::RxMode;
}

bool Nrf24Radio::enableTx()
{
    // pwrUp = 1 | primRx = 0 | ceHigh = 1 | FIFO = 1
    uint8_t config = readRegister(REG_CONFIG);
    config |= CONFIG_PWR_UP_BIT;
    config &= ~CONFIG_PRIM_RX_BIT;
    writeRegister(REG_CONFIG, config);
    ceHigh();

    // 6.1.7 Timing Information:
    // Standby modes -> TX/RX mode => 130µs
    // [1ms = 1000µs]
    osDelay(2);
    return true;
}

bool Nrf24Radio::enableRx()
{
    // pwrUp = 1 | primRx = 1 | ceHigh = 1 | FIFO = 0
    uint8_t config = readRegister(REG_CONFIG);
    config |= CONFIG_PWR_UP_BIT;
    config |= CONFIG_PRIM_RX_BIT;
    writeRegister(REG_CONFIG, config);
    ceHigh();

    // 6.1.7 Timing Information:
    // Standby modes -> TX/RX mode => 130µs
    // [1ms = 1000µs]
    osDelay(2);
    return true;
}

// Перевіряє, чи порожній TX FIFO (FIFO_STATUS, біт 4 = TX_EMPTY).
// 1 = порожній, 0 = є дані, ще не передані.
bool Nrf24Radio::isTxFifoEmpty() const
{
    uint8_t fifoStatus = readRegister(REG_FIFO_STATUS);
    return (fifoStatus & FIFO_STATUS_TX_EMPTY_BIT) != 0;
}

bool Nrf24Radio::isCeHigh() const
{
    return HAL_GPIO_ReadPin(cePort_, cePin_) == GPIO_PIN_SET;
}

RadioState Nrf24Radio::getCurrentState() const
{
    /**
    The following table describes how to configure the operational modes.

        Mode        |PWR_UP|PRIM_RX|CE|FIFO
        -----------------------------------------------------------------------------------------------------
        RX mode     |1|1|1|-
        TX mode     |1|0|1|Data in TX FIFOs. Will empty all levels in TX FIFOs(a*)
        TX mode     |1|0|Minimum 10µs high pulse|Data in TX FIFOs. Will empty one level in TX
        FIFOs(b*)
        Standby-II  |1|0|1|TX FIFO empty
        Standby-I   |1|-|0|No ongoing packet transmission
        Power Down  |0|-|-|-

    (a*) If CE is held high all TX FIFOs are emptied and all necessary ACK and possible retransmits
        are carried out. The transmission continues as long as the TX FIFO is refilled. If the TX
        FIFO is empty when the CE is still high, nRF24L01+ enters standby-II mode. In this mode the
        transmission of a packet is started as soon as the CSN is set high after an upload (UL) of a
        packet to TX FIFO.

    (b*) This operating mode pulses the CE high for at least 10µs. This allows one packet to be
        transmitted. This is the normal operating mode. After the packet is transmitted, the
        nRF24L01+ enters standby-I mode.
    */

    uint8_t config = readRegister(REG_CONFIG);
    bool pwrUp = (config & CONFIG_PWR_UP_BIT) != 0;
    if (!pwrUp)
    {
        return RadioState::PowerDown;
    }
    bool ceHigh = isCeHigh();
    if (!ceHigh)  // pwrUp = 1 | primRx = ? | ceHigh = 0 | FIFO = 0
    {
        return RadioState::StandbyI;
    }
    bool primRx = (config & CONFIG_PRIM_RX_BIT) != 0;
    if (primRx)  // pwrUp = 1 | primRx = 1 | ceHigh = 1 | FIFO = 0
    {
        return RadioState::RxMode;
    }
    bool txFifoEmpty = isTxFifoEmpty();
    if (txFifoEmpty)  // pwrUp = 1 | primRx = 0 | ceHigh = 1 | FIFO = 0
    {
        return RadioState::StandbyII;
    }
    return RadioState::TxMode;  // pwrUp = 1 | primRx = 0 | ceHigh = 1 | FIFO = 1
}

// RadioState Nrf24Radio::getCurrentState() const
// {
//     RadioState state = RadioState::Unknown;
//     switch (mode_)
//     {
//         case Mode::Undefined:
//         {
//             state = RadioState::Unknown;
//             break;
//         }
//         case Mode::Rx:
//         {
//             if (!isCeHigh())
//             {
//                 state = RadioState::StandbyI;  // when CE is set low, the nRF24L01 returns to
//                                                // standby-I mode from both the TX and RX modes.
//             }
//             else
//             {
//                 state = RadioState::RxMode;  // To enter this mode, the nRF24L01+ must have the
//                                              // PWR_UP bit, PRIM_RX bit and the CE pin set high.
//             }
//             break;
//         }
//         case Mode::Tx:
//         {
//             if (!isCeHigh())
//             {
//                 state = RadioState::StandbyI;  // when CE is set low, the nRF24L01 returns to
//                                                // standby-I mode from both the TX and RX modes.
//             }
//             else if (isTxFifoEmpty())
//             {
//                 state = RadioState::StandbyII;  // enters standby-II mode if CE is held high on a
//                                                 // PTX device with an empty TX FIFO
//             }
//             else
//             {
//                 state =
//                     RadioState::TxMode;  // To enter this mode, the nRF24L01+ must have the
//                     PWR_UP
//                                          // bit set high, PRIM_RX bit set low, a payload in the
//                                          TX
//                                          // FIFO and a high pulse on the CE for more than 10µs.
//             }
//             break;
//         }
//     }
//     return state;
// }
