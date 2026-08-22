#pragma once

#include <array>

#include "cmsis_os2.h"
#include "include/logger.hpp"

// Час, необхідний nRF24 на завершення внутрішнього power-on reset
// (datasheet, Table 14 "Power on reset", TPOR: max 100ms від моменту
// досягнення VDD 1.9V до готовності регістрів).
constexpr uint32_t NRF_POWER_ON_RESET_DELAY_MS = 100;

// Команди SPI (Table 20, datasheet, "Command set for the nRF24L01+ SPI")
constexpr uint8_t R_REGISTER = 0x00;    // 000A AAAA — читання регістра
constexpr uint8_t W_REGISTER = 0x20;    // 001A AAAA — запис регістра
constexpr uint8_t R_RX_PAYLOAD = 0x61;  // 0110 0001 — читання RX payload (1-32 байти)
constexpr uint8_t W_TX_PAYLOAD = 0xA0;  // 1010 0000 — запис TX payload (1-32 байти)
constexpr uint8_t FLUSH_TX = 0xE1;      // 1110 0001 — очистити TX FIFO
constexpr uint8_t FLUSH_RX = 0xE2;      // 1110 0010 — очистити RX FIFO
constexpr uint8_t REUSE_TX_PL = 0xE3;  // 1110 0011 — повторно передати останній TX payload
constexpr uint8_t R_RX_PL_WID = 0x60;  // 0110 0000 — прочитати довжину RX payload (для DPL)
constexpr uint8_t W_ACK_PAYLOAD = 0xA8;  // 1010 1PPP — payload для ACK, pipe (PPP)
constexpr uint8_t W_TX_PAYLOAD_NO_ACK = 0xB0;  // 1011 0000 — TX payload без auto-ACK на цей пакет
constexpr uint8_t NOP = 0xFF;  // 1111 1111 — No Operation (можна використати для читання STATUS)

// Адреси регістрів nRF24 (Table 28, datasheet, "Register Map")
constexpr uint8_t REG_CONFIG = 0x00;  // Configuration Register
constexpr uint8_t CONFIG_EN_CRC_BIT = 1 << 3;
constexpr uint8_t CONFIG_PWR_UP_BIT = 1 << 1;
constexpr uint8_t CONFIG_PRIM_RX_BIT = 1 << 0;

constexpr uint8_t REG_EN_AA = 0x01;       // Enable Auto Acknowledgment
constexpr uint8_t REG_EN_RXADDR = 0x02;   // Enabled RX Addresses
constexpr uint8_t REG_SETUP_AW = 0x03;    // Setup of Address Widths
constexpr uint8_t REG_SETUP_RETR = 0x04;  // Setup of Automatic Retransmission
constexpr uint8_t REG_RF_CH = 0x05;       // RF Channel
constexpr uint8_t REG_RF_SETUP = 0x06;    // RF Setup Register
constexpr uint8_t REG_STATUS = 0x07;      // Status Register
constexpr uint8_t REG_OBSERVE_TX = 0x08;  // Transmit observe register
constexpr uint8_t REG_RPD = 0x09;         // Received Power Detector
constexpr uint8_t REG_RX_ADDR_P0 = 0x0A;  // Receive address data pipe 0 (5 байт)
constexpr uint8_t REG_RX_ADDR_P1 = 0x0B;  // Receive address data pipe 1 (5 байт)
constexpr uint8_t REG_RX_ADDR_P2 = 0x0C;  // Receive address data pipe 2 (тільки LSB)
constexpr uint8_t REG_RX_ADDR_P3 = 0x0D;  // Receive address data pipe 3 (тільки LSB)
constexpr uint8_t REG_RX_ADDR_P4 = 0x0E;  // Receive address data pipe 4 (тільки LSB)
constexpr uint8_t REG_RX_ADDR_P5 = 0x0F;  // Receive address data pipe 5 (тільки LSB)
constexpr uint8_t REG_TX_ADDR = 0x10;     // Transmit address (5 байт)
constexpr uint8_t REG_RX_PW_P0 = 0x11;    // Number of bytes in RX payload, pipe 0
constexpr uint8_t REG_RX_PW_P1 = 0x12;    // Number of bytes in RX payload, pipe 1
constexpr uint8_t REG_RX_PW_P2 = 0x13;    // Number of bytes in RX payload, pipe 2
constexpr uint8_t REG_RX_PW_P3 = 0x14;    // Number of bytes in RX payload, pipe 3
constexpr uint8_t REG_RX_PW_P4 = 0x15;    // Number of bytes in RX payload, pipe 4
constexpr uint8_t REG_RX_PW_P5 = 0x16;    // Number of bytes in RX payload, pipe 5

constexpr uint8_t REG_FIFO_STATUS = 0x17;  // FIFO Status Register
constexpr uint8_t FIFO_STATUS_TX_REUSE_BIT = 1 << 6;
constexpr uint8_t FIFO_STATUS_TX_FULL_BIT = 1 << 5;
constexpr uint8_t FIFO_STATUS_TX_EMPTY_BIT = 1 << 4;
constexpr uint8_t FIFO_STATUS_RX_FULL_BIT = 1 << 1;
constexpr uint8_t FIFO_STATUS_RX_EMPTY_BIT = 1 << 0;

// 0x18-0x1B — Reserved for test purposes, altering them makes the chip malfunction
constexpr uint8_t REG_DYNPD = 0x1C;    // Enable dynamic payload length
constexpr uint8_t REG_FEATURE = 0x1D;  // Feature Register

// Reset-значення регістрів
constexpr uint8_t RESET_CONFIG = 0b00001000;  // EN_CRC=1, решта 0
constexpr uint8_t RESET_EN_AA = 0b00111111;   // auto-ack увімкнено на всіх 6 pipes
constexpr uint8_t RESET_EN_RXADDR = 0b00000011;   // увімкнені лише pipe 0 і pipe 1
constexpr uint8_t RESET_SETUP_AW = 0b00000011;    // ширина адреси = 5 байт
constexpr uint8_t RESET_SETUP_RETR = 0b00000011;  // ARD=250µs, ARC=3 (до 3 повторів)
constexpr uint8_t RESET_RF_CH = 0b00000010;       // 2402 MHz (2400 + 2)
constexpr uint8_t RESET_RF_SETUP = 0b00001110;    // 1Mbps, RF_PWR=11 (0dBm)
constexpr uint8_t RESET_STATUS = 0b00001110;      // RX_P_NO=111 (RX FIFO Empty)
constexpr uint8_t RESET_OBSERVE_TX = 0b00000000;  // лічильники втрат/ретраїв на нулі
constexpr uint8_t RESET_RPD = 0b00000000;  // сигнал не задетектовано

// Адресні регістри (5 байт кожен) — reset-значення як масив байтів
// LSByte записаний першим (за конвенцією, LSByte first, з datasheet)
constexpr std::array<uint8_t, 5> RESET_RX_ADDR_P0 = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
constexpr std::array<uint8_t, 5> RESET_RX_ADDR_P1 = {0xC2, 0xC2, 0xC2, 0xC2, 0xC2};
constexpr std::array<uint8_t, 5> RESET_TX_ADDR = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
// Однобайтні адреси pipe 2-5 (лише LSB, старші 4 байти спільні з RX_ADDR_P1)
constexpr uint8_t RESET_RX_ADDR_P2 = 0xC3;
constexpr uint8_t RESET_RX_ADDR_P3 = 0xC4;
constexpr uint8_t RESET_RX_ADDR_P4 = 0xC5;
constexpr uint8_t RESET_RX_ADDR_P5 = 0xC6;

constexpr uint8_t RESET_RX_PW_P0 = 0b00000000;
constexpr uint8_t RESET_RX_PW_P1 = 0b00000000;
constexpr uint8_t RESET_RX_PW_P2 = 0b00000000;
constexpr uint8_t RESET_RX_PW_P3 = 0b00000000;
constexpr uint8_t RESET_RX_PW_P4 = 0b00000000;
constexpr uint8_t RESET_RX_PW_P5 = 0b00000000;
constexpr uint8_t RESET_FIFO_STATUS = 0b00010001;  // TX_EMPTY=1, RX_EMPTY=1
constexpr uint8_t RESET_DYNPD = 0b00000000;  // dynamic payload вимкнено на всіх pipes
constexpr uint8_t RESET_FEATURE = 0b00000000;  // усі додаткові фічі вимкнені

enum class Direction
{
    Tx,
    Rx,
    HalfDuplex,
};

enum class RadioState
{
    Unknown,
    StandbyI,
    StandbyII,
    TxMode,
    RxMode,
    PowerDown
};

class Nrf24Radio
{
   public:
    bool init();

    uint8_t readRegister(uint8_t reg) const;
    uint8_t writeRegister(uint8_t reg, uint8_t value) const;

    Nrf24Radio(SPI_HandleTypeDef* hspi, GPIO_TypeDef* csnPort, uint16_t csnPin,
               GPIO_TypeDef* cePort, uint16_t cePin, Direction direction)
        : hspi_(hspi),
          csnPort_(csnPort),
          csnPin_(csnPin),
          cePort_(cePort),
          cePin_(cePin),
          direction_(direction)
    {
    }

    bool enableTx();
    bool enableRx();

    bool isTxEnabled() const;
    bool isRxEnabled() const;
    bool isTxFifoEmpty() const;
    bool isCeHigh() const;
    RadioState getCurrentState() const;
    bool setTxRfFilterKey(const std::array<uint8_t, 5>& address);
    bool setRxRfFilterKey(const std::array<uint8_t, 5>& address);
    bool setRxPayloadLength(const uint8_t bytes);

   private:
    static constexpr const char* TAG = "NRF24";

    void beginTransaction() const;
    void endTransaction() const;
    void ceHigh();
    void ceLow();
    uint8_t writeMultiByteRegister(uint8_t reg, const uint8_t* data, size_t length);

    SPI_HandleTypeDef* hspi_;

    GPIO_TypeDef* csnPort_;
    uint16_t csnPin_;

    GPIO_TypeDef* cePort_;
    uint16_t cePin_;

    Direction direction_;
};
