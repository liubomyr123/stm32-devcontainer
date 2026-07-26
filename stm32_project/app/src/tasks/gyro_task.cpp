#include <array>
#include <cmath>

#include "cmsis_os.h"
#include "include/logger.hpp"
#include "include/mpu6050.hpp"

extern "C" void GyroTask(void* argument)
{
    (void)argument;

    LOG_INFO("GYRO", "Task started");

    if (!MPU6050::instance().waitReady())
    {
        vTaskDelete(nullptr);
    }

    if (!MPU6050::instance().init())
    {
        vTaskDelete(nullptr);
    }

    while (true)
    {
        float pitch;
        float roll;
        MPU6050::instance().readAccel(pitch, roll);

        LOG_INFO("GYRO", "Pitch=%.1f Roll=%.1f deg", pitch, roll);

        vTaskDelay(pdMS_TO_TICKS(50));  // 50 Hz
    }
}
