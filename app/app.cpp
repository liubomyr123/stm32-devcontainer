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
    UartCmd cmd;

    LOG_INFO("APP", "Started!");
    while (true)
    {
        if (osMessageQueueGet(uartCmdQueueHandle, &cmd, nullptr, 0) == osOK)
        {
            LOG_INFO("APP", "Data: cmd=%s speed=%d", cmdTypeToString(cmd.type), cmd.value);

            switch (StateMachine::instance().getState())
            {
                case State::IDLE:
                {
                    if (cmd.type == CMD_FORWARD || cmd.type == CMD_BACKWARD)
                    {
                        StateMachine::instance().setDriveParams(cmd.type, cmd.value, 0);
                        StateMachine::instance().updateState(State::DRIVING);
                    }
                    // else if (cmd.type == CMD_LEFT || cmd.type == CMD_RIGHT)
                    // {
                    //     // skip...
                    // }
                    // else if (cmd.type == CMD_STOP || cmd.type == CMD_UNKNOWN)
                    // {
                    //     // also skip...
                    // }
                    break;
                }

                case State::DRIVING:
                {
                    const DriveParams& p = StateMachine::instance().getDriveParams();
                    if (cmd.type == CMD_FORWARD || cmd.type == CMD_BACKWARD)
                    {
                        StateMachine::instance().setDriveParams(cmd.type, cmd.value, p.steering);
                    }
                    else if (cmd.type == CMD_LEFT || cmd.type == CMD_RIGHT)
                    {
                        StateMachine::instance().setDriveParams(cmd.type, p.throttle, cmd.value);
                    }
                    else if (cmd.type == CMD_STOP)
                    {
                        StateMachine::instance().setDriveParams(CMD_STOP, 0, 0);
                        StateMachine::instance().updateState(State::IDLE);
                    }
                    break;
                }

                default:
                {
                    break;
                }
            }

            const DriveParams& p = StateMachine::instance().getDriveParams();
            LOG_INFO("APP", "State=%s dir=%s throttle=%d steering=%d",
                     StateMachine::stateToString(StateMachine::instance().getState()),
                     cmdTypeToString(p.direction), p.throttle, p.steering);
        }

        switch (StateMachine::instance().getState())
        {
            case State::IDLE:
            {
                MotorController::instance().stop();
                break;
            }
            case State::DRIVING:
            {
                const DriveParams& p = StateMachine::instance().getDriveParams();
                MotorController::instance().apply(p.direction, p.throttle, p.steering);
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
