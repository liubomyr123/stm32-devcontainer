#include "cmsis_os.h"
#include "include/button.h"
#include "include/led.h"
#include "include/logger.hpp"
#include "include/state_machine.hpp"

extern osMessageQueueId_t uartCmdQueueHandle;

extern "C" void app_main()
{
    LED led(GPIOG, GPIO_PIN_13);
    Button btn(GPIOA, GPIO_PIN_0);
    UartCmd cmd;

    LOG_INFO("APP", "Started!");
    while (true)
    {
        if (osMessageQueueGet(uartCmdQueueHandle, &cmd, NULL, 0) == osOK)
        {
            LOG_INFO("APP", "Data: cmd=%s speed=%d", cmdTypeToString(cmd.type), cmd.value);

            switch (StateMachine::instance().getState())
            {
                case State::IDLE:
                {
                    if (cmd.type == CMD_FORWARD || cmd.type == CMD_BACKWARD ||
                        cmd.type == CMD_LEFT || cmd.type == CMD_RIGHT)
                    {
                        StateMachine::instance().setDriveParams(cmd.type, cmd.value);
                        StateMachine::instance().updateState(State::DRIVING);
                    }
                    break;
                }

                case State::DRIVING:
                {
                    if (cmd.type == CMD_STOP)
                    {
                        StateMachine::instance().setDriveParams(CMD_STOP, 0);
                        StateMachine::instance().updateState(State::IDLE);
                    }
                    else
                    {
                        StateMachine::instance().setDriveParams(cmd.type, cmd.value);
                        LOG_INFO("APP", "Update params: dir=%d speed=%d", cmd.type, cmd.value);
                    }
                    break;
                }

                default:
                    break;
            }
        }

        switch (StateMachine::instance().getState())
        {
            case State::IDLE:
            {
                // const DriveParams& p = StateMachine::instance().getDriveParams();
                // LOG_DEBUG("APP", "STOP dir=%d speed=%d", p.direction, p.speed);
                break;
            }
            case State::DRIVING:
            {
                // const DriveParams& p = StateMachine::instance().getDriveParams();
                // LOG_DEBUG("APP", "DRIVING dir=%d speed=%d", p.direction, p.speed);
                break;
            }

            default:
                break;
        }

        osDelay(10);
    }
}
