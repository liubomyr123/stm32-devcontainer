#include <array>
#include <cmath>

#include "cmsis_os.h"
#include "include/logger.hpp"
#include "include/mpu6050.hpp"
#include "motor_controller.hpp"
#include "telemetry_packet.h"
#include "uart_manager.hpp"

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart3;

extern "C" void GyroTask(void* argument)
{
    (void)argument;
    LOG_INFO("GYRO", "Task started");

    vTaskDelay(pdMS_TO_TICKS(500));

    MPU6050 mpu{&hi2c1};

    if (!mpu.init())
    {
        vTaskDelete(nullptr);
    }

    while (true)
    {
        if (!mpu.readAll())
        {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        LOG_INFO("GYRO", "Pitch=%.1f Roll=%.1f", mpu.getPitch(), mpu.getRoll());

        TelemetryPacket pkt;
        pkt.pitch = static_cast<float>(mpu.getPitch());
        pkt.roll = static_cast<float>(mpu.getRoll());
        pkt.fl = MotorController::instance().getFL();
        pkt.fr = MotorController::instance().getFR();
        pkt.rl = MotorController::instance().getRL();
        pkt.rr = MotorController::instance().getRR();

        UartManager::instance().send((uint8_t*)&pkt, sizeof(pkt));

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
