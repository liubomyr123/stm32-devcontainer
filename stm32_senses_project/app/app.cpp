#include "cmsis_os.h"
#include "include/logger.hpp"

extern "C" void app_main()
{
    LOG_INFO("APP", "Started!");
    while (true)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        osDelay(250);
    }
}