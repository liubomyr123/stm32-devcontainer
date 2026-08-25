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
    if (direction_ != Direction::Tx && direction_ != Direction::HalfDuplex)
    {
        LOG_ERROR(TAG, "enableTx() called, but radio is not in TX mode");
        return false;
    }

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
    if (direction_ != Direction::Rx && direction_ != Direction::HalfDuplex)
    {
        LOG_ERROR(TAG, "enableRx() called, but radio is not in RX mode");
        return false;
    }

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

// Встановлює RF-фільтр-ключ передавача (TX_ADDR) — за цим самим
// ключем приймач розпізнає, що пакет адресований саме йому.
// Використовується лише на TX-стороні.
bool Nrf24Radio::setTxRfFilterKey(const std::array<uint8_t, 5>& address)
{
    if (direction_ != Direction::Tx && direction_ != Direction::HalfDuplex)
    {
        LOG_ERROR(TAG, "setTxRfFilterKey() called, but radio is not in TX mode");
        return false;
    }

    writeMultiByteRegister(REG_TX_ADDR, address.data(), address.size());
    return true;
}

// Встановлює RF-фільтр-ключ, за яким PRX апаратно відфільтровує
// вхідні пакети на pipe 0 (RX_ADDR_P0) — приймаються лише ті,
// що мають точний збіг з цим ключем. Має дорівнювати TX-ключу
// передавача, з яким плануємо спілкуватись.
bool Nrf24Radio::setRxRfFilterKey(const std::array<uint8_t, 5>& address)
{
    if (direction_ != Direction::Rx && direction_ != Direction::HalfDuplex)
    {
        LOG_ERROR(TAG, "setRxRfFilterKey() called, but radio is not in RX mode");
        return false;
    }

    writeMultiByteRegister(REG_RX_ADDR_P0, address.data(), address.size());
    return true;
}

// Встановлює очікувану довжину payload на pipe 0 (RX_PW_P0) —
// скільки байт чекати в кожному пакеті
bool Nrf24Radio::setRxPayloadLength(uint8_t bytes)
{
    if (direction_ != Direction::Rx && direction_ != Direction::HalfDuplex)
    {
        LOG_ERROR(TAG, "setRxPayloadLength() called, but radio is not in RX mode");
        return false;
    }

    if (bytes > 32)
    {
        LOG_ERROR(TAG, "setRxPayloadLength(%d) exceeds max payload size (32 bytes)", bytes);
        return false;
    }

    writeRegister(REG_RX_PW_P0, bytes);
    return true;
}

// Записує TX payload (W_TX_PAYLOAD, Table 20) — 1-32 байти, LSByte first.
uint8_t Nrf24Radio::writeTxPayload(const uint8_t* data, size_t length)
{
    std::array<uint8_t, 33> txBuf{};  // 1 опкод + максимум 32 байти даних
    txBuf[0] = W_TX_PAYLOAD;
    for (size_t i = 0; i < length; i++)
    {
        txBuf.at(i + 1) = data[i];
    }

    std::array<uint8_t, 33> rxBuf{};

    beginTransaction();
    HAL_SPI_TransmitReceive(hspi_,                              //
                            txBuf.data(),                       //
                            rxBuf.data(),                       //
                            static_cast<uint16_t>(length + 1),  //
                            HAL_MAX_DELAY);
    endTransaction();

    return rxBuf[0];  // STATUS
}

// Читає RX payload (R_RX_PAYLOAD, Table 20) — 1-32 байти, LSByte first.
void Nrf24Radio::readRxPayload(uint8_t* buffer, size_t length)
{
    std::array<uint8_t, 33> txBuf{};
    txBuf[0] = R_RX_PAYLOAD;
    for (size_t i = 1; i <= length; i++)
    {
        txBuf.at(i) = 0xFF;  // "пусте"/NOP, генерує такти SCK для отримання даних
    }

    std::array<uint8_t, 33> rxBuf{};

    beginTransaction();
    HAL_SPI_TransmitReceive(hspi_,                              //
                            txBuf.data(),                       //
                            rxBuf.data(),                       //
                            static_cast<uint16_t>(length + 1),  //
                            HAL_MAX_DELAY);
    endTransaction();

    for (size_t i = 0; i < length; i++)
    {
        buffer[i] = rxBuf.at(i + 1);  // rxBuf[0] = STATUS, дані з rxBuf[1..]
    }
}

// Відправляє один payload (1-32 байти). CE вже піднятий (enableTx()),
// тому чіп сам почне передачу щойно payload потрапить у TX FIFO
// (Standby-II → TX mode, datasheet §6.1.5).
bool Nrf24Radio::transmit(const uint8_t* data, uint8_t length)
{
    if (direction_ != Direction::Tx && direction_ != Direction::HalfDuplex)
    {
        LOG_ERROR(TAG, "transmit() called, but radio is not in TX mode");
        return false;
    }

    if (!isCeHigh())
    {
        LOG_ERROR(TAG, "transmit() called, but CE is not high — call enableTx() first");
        return false;
    }

    // Завантажуємо payload у TX FIFO (W_TX_PAYLOAD) по SPI.
    writeTxPayload(data, length);

    // Чекаємо підтвердження реальної радіопередачі.
    // Активний поллінг - щоразу заново читаємо TX_DS біт через SPI
    // Register Map, Table 28: "Asserted when packet transmitted on TX"
    constexpr uint32_t TIMEOUT_MS = 100;

    uint32_t elapsed = 0;
    while (elapsed < TIMEOUT_MS)
    {
        uint8_t status = readRegister(REG_STATUS);
        if ((status & STATUS_TX_DS_BIT) != 0)
        {
            // "Write 1 to clear bit" (Table 28) — обов'язково скидаємо
            // прапорець. Якщо цього не зробити, наступний виклик
            // transmit() одразу побачить ЗАСТАРІЛИЙ TX_DS від цієї
            // передачі й помилково поверне true, навіть не почавши
            // нову передачу.
            writeRegister(REG_STATUS, STATUS_TX_DS_BIT);
            return true;
        }
        osDelay(1);
        elapsed++;
    }

    LOG_ERROR(TAG, "transmit() timed out waiting for TX_DS");
    return false;
}

bool Nrf24Radio::receive(uint8_t* buffer, uint8_t length)
{
    if (direction_ != Direction::Rx && direction_ != Direction::HalfDuplex)
    {
        LOG_ERROR(TAG, "receive() called, but radio is not in RX mode");
        return false;
    }

    if (!isCeHigh())
    {
        LOG_ERROR(TAG, "receive() called, but CE is not high — call enableRx() first");
        return false;
    }

    uint8_t status = readRegister(REG_STATUS);
    if ((status & STATUS_RX_DR_BIT) == 0)
    {
        return false;  // немає нових даних
    }

    readRxPayload(buffer, length);

    // "Write 1 to clear bit" (Table 28)
    writeRegister(REG_STATUS, STATUS_RX_DR_BIT);

    return true;
}

// Встановлює air data rate (RF_SETUP, біти RF_DR_LOW/RF_DR_HIGH).
// Обидві сторони (TX і RX) МАЮТЬ мати однакове значення (datasheet, 6.2) —
// інакше пристрої просто не зрозуміють сигнал одне одного.
bool Nrf24Radio::setAirDataRate(DataRate rate)
{
    /**
        9.1 Register map table
        06 RF_SETUP:
        [RF_DR_LOW, RF_DR_HIGH]:
        ‘00’ – 1Mbps
        ‘01’ – 2Mbps
        ‘10’ – 250kbps
        ‘11’ – Reserved
    */
    uint8_t rfSetup = readRegister(REG_RF_SETUP);

    rfSetup &= ~RF_SETUP_RF_DR_LOW_BIT;   // RF_DR_LOW = 0
    rfSetup &= ~RF_SETUP_RF_DR_HIGH_BIT;  // RF_DR_HIGH = 0

    switch (rate)
    {
        case DataRate::Mbps1:
            break;
        case DataRate::Mbps2:
            rfSetup |= RF_SETUP_RF_DR_HIGH_BIT;  // RF_DR_LOW=0, RF_DR_HIGH=1
            break;
        case DataRate::Kbps250:
            rfSetup |= RF_SETUP_RF_DR_LOW_BIT;  // RF_DR_LOW=1, RF_DR_HIGH=0
            break;
    }

    writeRegister(REG_RF_SETUP, rfSetup);
    dataRate_ = rate;
    return true;
}

// Встановлює робочу частоту каналу (RF_CH): F0 = 2400 + RF_CH [MHz].
bool Nrf24Radio::setChannel(uint8_t channel)
{
    /**
        6.3 RF channel frequency
        ...nRF24L01+ can operate on frequencies from 2.400GHz to 2.525GHz...
        ...The programming resolution of the RF channel frequency setting is 1MHz...
        ...To ensure non-overlapping channels in 2Mbps mode, the channel spacing must be 2MHz or
       more...
        ...At 1Mbps and 250kbps the channel bandwidth is the same or lower than the resolution of
       the RF frequency...
    */

    /**
        9.1 Register map table
        05 RF_CH:
        Bits: 6:0
        Reset value: 0000010

        7 біт дає 128 можливих комбінацій (2^7 = 128)
        Тобто теоретично можна записати будь-яке число від 0 до 127 (0b1111111)
        Діапазон: від 2.400GHz до 2.525GHz. Різниця: 2525 - 2400 = 125 MHz.
        Тобто доступні нам є від 0 до 125.

        Значення 126 (0b1111110) і 127 (0b1111111) можна фізично записати в регістр,
        але вони відповідали б частотам 2526MHz і 2527MHz, які виходять за межі робочого діапазону
    */

    if (channel > 125)
    {
        LOG_ERROR(TAG, "setChannel(%d) exceeds valid range (0-125)", channel);
        return false;
    }

    if (dataRate_ == DataRate::Mbps2)
    {
        LOG_WARNING(TAG,
                    "Data rate is 2Mbps - channel occupies ~2MHz bandwidth. "
                    "Ensure other RF sources (WiFi, other nRF24) are at least "
                    "2MHz away from channel %d to avoid overlap.",
                    channel);
    }

    writeRegister(REG_RF_CH, channel);
    return true;
}
