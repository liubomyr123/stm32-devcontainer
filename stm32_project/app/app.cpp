#include "cmsis_os.h"
#include "include/button.h"
#include "include/led.h"
#include "include/logger.hpp"
#include "include/motor_controller.hpp"
#include "include/state_machine.hpp"
#include "uart_config.h"

extern osMessageQueueId_t uartCmdQueueHandle;

extern "C" void app_main()
{
    LED led(GPIOG, GPIO_PIN_13);
    Button btn(GPIOA, GPIO_PIN_0);
    UartCmd cmd{};

    LOG_INFO("APP", "Started!");
    while (true)
    {
        if (osMessageQueueGet(uartCmdQueueHandle, &cmd, nullptr, 0) == osOK)
        {
            LOG_INFO("APP", "Data: cmd=%s", cmdTypeToString(cmd.type));

            switch (StateMachine::instance().getState())
            {
                case State::IDLE:
                {
                    if (cmd.type == CMD_STOP || cmd.type == CMD_UNKNOWN)
                    {
                        break;
                    }
                    StateMachine::instance().setDriveParams(cmd);
                    StateMachine::instance().updateState(State::DRIVING);
                    break;
                }

                case State::DRIVING:
                {
                    if (cmd.type == CMD_STOP || cmd.type == CMD_UNKNOWN)
                    {
                        UartCmd stop_cmd{};
                        stop_cmd.type = CMD_STOP;
                        StateMachine::instance().setDriveParams(stop_cmd);
                        StateMachine::instance().updateState(State::IDLE);
                    }
                    else
                    {
                        StateMachine::instance().setDriveParams(cmd);
                    }
                    break;
                }

                default:
                {
                    break;
                }
            }

            const UartCmd& p = StateMachine::instance().getDriveParams();
            LOG_INFO("APP", "State=%s dir=%s F=%d B=%d L=%d R=%d",
                     StateMachine::stateToString(StateMachine::instance().getState()),
                     cmdTypeToString(p.type), p.f, p.b, p.l, p.r);
        }

        switch (StateMachine::instance().getState())
        {
            case State::IDLE:
            {
                const UartCmd& p = StateMachine::instance().getDriveParams();
                if (p.type == CMD_STOP)
                {
                    break;
                }
                MotorController::instance().stop();
                break;
            }
            case State::DRIVING:
            {
                const UartCmd& p = StateMachine::instance().getDriveParams();
                MotorController::instance().apply(p);
                break;
            }

            default:
            {
                break;
            }
        }

        osDelay(10);
    }
}
