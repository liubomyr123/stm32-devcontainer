#include "cmsis_os.h"
#include "include/servo_sg90.hpp"
#include "logger.h"

extern "C" void ServoSG90Task(void* argument)
{
    (void)argument;
    LOG_INFO("SERVO", "Task started");

    while (true)
    {
        ServoSG90::instance().tick();
        osDelay(50);
    }
}