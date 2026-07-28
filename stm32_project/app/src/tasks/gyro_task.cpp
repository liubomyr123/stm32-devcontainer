#include <array>
#include <cmath>

#include "cmsis_os.h"
#include "include/logger.hpp"
#include "include/mpu6050.hpp"

extern I2C_HandleTypeDef hi2c1;

extern "C" void GyroTask(void* argument)
{
    (void)argument;

    LOG_INFO("GYRO", "Task started");

    MPU6050 mpu{&hi2c1};

    if (!mpu.init())
    {
        vTaskDelete(nullptr);
    }

    while (true)
    {
        mpu.readAll();
        LOG_INFO("GYRO", "Pitch=%.1f Roll=%.1f", mpu.getPitch(), mpu.getRoll());

        vTaskDelay(pdMS_TO_TICKS(50));  // 50 Hz
    }
}
