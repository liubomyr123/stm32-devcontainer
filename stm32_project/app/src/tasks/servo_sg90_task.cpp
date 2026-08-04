#include "cmsis_os.h"
#include "include/servo_sg90.hpp"
#include "logger.h"

extern "C" void ServoSG90Task(void* argument)
{
    (void)argument;
    LOG_INFO("SERVO", "Task started");

    if (!ServoSG90::instance().init())
    {
        vTaskDelete(nullptr);
    }

    const uint16_t PULSE_MIN = 1000;
    const uint16_t PULSE_CENTER = 1500;
    const uint16_t PULSE_MAX = 2000;

    while (true)
    {
        ServoSG90::instance().setPulse(PULSE_MIN);
        LOG_INFO("SERVO", "Pulse -> 1000 (-90)");
        vTaskDelay(pdMS_TO_TICKS(1000));

        ServoSG90::instance().setPulse(PULSE_CENTER);
        LOG_INFO("SERVO", "Pulse -> 1500 (0)");
        vTaskDelay(pdMS_TO_TICKS(1000));

        ServoSG90::instance().setPulse(PULSE_MAX);
        LOG_INFO("SERVO", "Pulse -> 2000 (+90)");
        vTaskDelay(pdMS_TO_TICKS(1000));

        ServoSG90::instance().setPulse(PULSE_CENTER);
        LOG_INFO("SERVO", "Pulse -> 1500 (0)");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}