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
    // План подальшої роботи над Nrf24Radio:
    //
    // 1) Налаштувати спільну 5-байтну адресу для обох плат
    //    (writeMultiByteRegister вже готовий, лишилось застосувати):
    //    - На пульті (TX): записати TX_ADDR
    //    - На senses-платі (RX): записати RX_ADDR_P0 = та сама адреса
    //
    nrf.setRxRfFilterKey(SHARED_RF_FILTER_KEY);

    // 2) Налаштувати довжину очікуваного payload на RX-стороні
    //    (RX_PW_P0 — скільки байт очікувати на pipe 0,
    //    обов'язково без dynamic payload length)
    //
    nrf.setRxPayloadLength(TEST_REG_RX_PW_P0_VALUE);

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

    LOG_INFO("NRF", "State before enableRx() = %d", static_cast<int>(nrf.getCurrentState()));
    nrf.enableRx();
    RadioState rxState = nrf.getCurrentState();
    LOG_INFO("NRF", "State after enableRx() = %d (expect RxMode = %d)", static_cast<int>(rxState),
             static_cast<int>(RadioState::RxMode));

    while (true)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        osDelay(250);
    }
}