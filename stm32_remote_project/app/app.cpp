#include <array>

#include "cmsis_os.h"
#include "fatfs.h"
#include "include/joystickKY023.hpp"
#include "include/logger.hpp"
#include "include/nrf24radio.hpp"
#include "include/sdcard.hpp"

extern SPI_HandleTypeDef hspi1;
extern SD_HandleTypeDef hsd;

#define NRF_CSN_PORT GPIOB
#define NRF_CSN_PIN GPIO_PIN_1

#define NRF_CE_PORT GPIOB
#define NRF_CE_PIN GPIO_PIN_0

// Спільний RF-фільтр-ключ для комунікації пульт↔senses-плата (MVP, один канал)
// constexpr std::array<uint8_t, 5> SHARED_RF_FILTER_KEY = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};

// Робочий канал — подалі від типових WiFi-каналів (1/6/11)
// F0 = 2400 + 100 = 2500 MHz
// constexpr uint8_t SHARED_CHANNEL = 100;

extern ADC_HandleTypeDef hadc1;

extern "C" void app_main()
{
    LOG_INFO("APP", "Started!");

    SdCard card = {};

    if (!card.init())
    {
        vTaskDelete(nullptr);
    }

    const char message[] = "Hello from STM32!\r\n";
    card.updateCurrentLog(message);

    // Nrf24Radio nrf{&hspi1,                     //
    //                NRF_CSN_PORT, NRF_CSN_PIN,  //
    //                NRF_CE_PORT,  NRF_CE_PIN,   //
    //                Direction::Tx};

    // if (!nrf.init())
    // {
    //     vTaskDelete(nullptr);
    // }

    // nrf.setAirDataRate(DataRate::Mbps1);
    // nrf.setChannel(SHARED_CHANNEL);
    // nrf.setTxRfFilterKey(SHARED_RF_FILTER_KEY);

    // LOG_INFO("NRF", "State before enableTx() = %d", static_cast<int>(nrf.getCurrentState()));
    // nrf.enableTx();
    // RadioState txState = nrf.getCurrentState();
    // LOG_INFO("NRF", "State after enableTx() = %d (expect StandbyII = %d)",
    //          static_cast<int>(txState), static_cast<int>(RadioState::StandbyII));

    // uint32_t counter = 0;

    JoystickKY023 joystickControl{&hadc1,         //
                                  ADC_CHANNEL_0,  //
                                  ADC_CHANNEL_1,  //
                                  GPIOC,          //
                                  GPIO_PIN_1};

    if (!joystickControl.init())
    {
        vTaskDelete(nullptr);
    }

    JoystickKY023 joystickCamera{&hadc1,          //
                                 ADC_CHANNEL_4,   //
                                 ADC_CHANNEL_10,  //
                                 GPIOC,           //
                                 GPIO_PIN_2};

    if (!joystickCamera.init())
    {
        vTaskDelete(nullptr);
    }

    while (true)
    {
        // uint8_t buffer[4] = {
        //     static_cast<uint8_t>((counter >> 24) & 0xFF),
        //     static_cast<uint8_t>((counter >> 16) & 0xFF),
        //     static_cast<uint8_t>((counter >> 8) & 0xFF),
        //     static_cast<uint8_t>(counter & 0xFF),
        // };

        // bool sent = nrf.transmit(buffer, sizeof(buffer));
        // LOG_INFO("NRF", "Transmit #%lu: %s", counter, sent ? "OK" : "FAILED");

        // counter++;

        // HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
        // osDelay(1000);

        int16_t joyControlX = 0;
        joystickControl.readAdcChannelXPercentage(joyControlX);
        int16_t joyControlY = 0;
        joystickControl.readAdcChannelYPercentage(joyControlY);
        int16_t joyCameraX = 0;
        joystickCamera.readAdcChannelXPercentage(joyCameraX);
        int16_t joyCameraY = 0;
        joystickCamera.readAdcChannelYPercentage(joyCameraY);

        GPIO_PinState joyControlSw = joystickControl.readSwButton();
        GPIO_PinState joyCameraSw = joystickCamera.readSwButton();

        LOG_INFO("JOY", "joyControl: X=[%d%%] Y=[%d%%] SW=%d | joyCamera: X=[%d%%] Y=[%d%%] SW=%d",
                 joyControlX, joyControlY, joyControlSw, joyCameraX, joyCameraY, joyCameraSw);

        osDelay(250);
    }
}