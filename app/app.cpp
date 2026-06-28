#include "include/button.h"
#include "include/led.h"
#include "include/logger.hpp"
#include "include/state_machine.hpp"

extern "C" void app_main()
{
    LED led(GPIOG, GPIO_PIN_13);
    Button btn(GPIOA, GPIO_PIN_0);

    LOG_INFO("APP", "Started!");

    while (true)
    {
        switch (StateMachine::instance().getState())
        {
            case State::IDLE:
            {
                // LOG_DEBUG("State", "IDLE");
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
                break;
            }
            case State::ARMED:
            {
                LOG_DEBUG("State", "ARMED");
                break;
            }
            case State::DRIVING:
            {
                LOG_DEBUG("State", "DRIVING");
                break;
            }
            default:
            {
                break;
            }
        }
    }
}