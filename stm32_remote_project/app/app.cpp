#include "include/logger.hpp"
#include "main.h"

extern "C" void app_main()
{
    LOG_INFO("APP", "Started!");

    while (true)
    {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
        HAL_Delay(250);
    }
}