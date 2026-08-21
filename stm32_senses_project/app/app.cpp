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

    LOG_INFO("NRF", "State before enableTx() = %d", static_cast<int>(nrf.getCurrentState()));

    nrf.enableTx();

    RadioState state = nrf.getCurrentState();
    LOG_INFO("NRF", "State after enableTx() = %d (expect StandbyII = %d)", static_cast<int>(state),
             static_cast<int>(RadioState::StandbyII));

    nrf.enableRx();

    RadioState rxState = nrf.getCurrentState();
    LOG_INFO("NRF", "State after enableRx() = %d (expect RxMode = %d)", static_cast<int>(rxState),
             static_cast<int>(RadioState::RxMode));

    // План подальшої роботи над Nrf24Radio:
    //
    // 1) Налаштувати спільну 5-байтну адресу для обох плат
    //    (writeMultiByteRegister вже готовий, лишилось застосувати):
    //    - На пульті (TX): записати TX_ADDR
    //    - На senses-платі (RX): записати RX_ADDR_P0 = та сама адреса
    //
    // 2) Налаштувати довжину очікуваного payload на RX-стороні
    //    (RX_PW_P0 — скільки байт очікувати на pipe 0,
    //    обов'язково без dynamic payload length)
    //
    // 3) Написати transmit() на боці TX (пульт):
    //    - команда W_TX_PAYLOAD (завантажити payload у TX FIFO)
    //    - CE вже піднятий (Standby-II), чіп сам почне передачу
    //    - чекати біт TX_DS у STATUS (підтвердження "пакет пішов")
    //
    // 4) Перевірити transmit() на реальному залізі
    //    (навіть без приймача поруч — TX_DS має спрацювати,
    //    бо auto-ACK вимкнено)
    //
    // 5) Написати receive() на боці RX (senses-плата):
    //    - перевірити RX_DR у STATUS чи RX_EMPTY у FIFO_STATUS
    //    - команда R_RX_PAYLOAD (вичитати дані з RX FIFO)
    //
    // 6) Зібрати обидві плати разом:
    //    пульт (TX) шле щось просте раз на секунду,
    //    senses-плата (RX) приймає і логує — перший реальний
    //    радіозв'язок між двома фізичними пристроями

    while (true)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        osDelay(250);
    }
}