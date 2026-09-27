#include <array>

#include "cmsis_os.h"
#include "fatfs.h"
#include "include/crypto_selftest.hpp"
#include "include/joystickKY023.hpp"
#include "include/logger.hpp"
#include "include/nrf24radio.hpp"
#include "include/sdcard.hpp"

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;

extern "C" void app_main()
{
    LOG_INFO("APP", "Started!");

    runCryptoSelfTest();

    // SdCard card = {};

    // if (!card.init())
    // {
    //     vTaskDelete(nullptr);
    // }

    // const char message[] = "Hello from STM32!\r\n";
    // card.updateCurrentLog(message);

    JoystickKY023 joystickControl{&hadc1,         //
                                  &htim2,         //
                                  ADC_CHANNEL_0,  //
                                  ADC_CHANNEL_1,  //
                                  GPIOC,          //
                                  GPIO_PIN_1};

    if (!joystickControl.init())
    {
        vTaskDelete(nullptr);
    }

    JoystickKY023 joystickCamera{&hadc1,          //
                                 &htim2,          //
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
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);

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