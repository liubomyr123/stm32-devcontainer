#include "include/button.h"
#include "include/led.h"
#include "include/logger.hpp"

extern "C" void app_main()
{
    LED led(GPIOG, GPIO_PIN_13);
    Button btn(GPIOA, GPIO_PIN_0);

    LOG_INFO("APP", "Started!");

    while (true)
    {
        if (btn.isPressed())
        {
            led.toggle();
            LOG_DEBUG("APP", "Button pressed");
            HAL_Delay(100);
        }
        else
        {
            led.off();
        }
    }
}