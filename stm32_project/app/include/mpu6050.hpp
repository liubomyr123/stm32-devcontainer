#pragma once

#include <array>
#include <cmath>

#include "cmsis_os.h"
#include "include/logger.hpp"

// I2C адреса MPU6050 (AD0 = GND → 0x68, зсув на 1 для HAL)
#define MPU6050_I2C_ADDR (0x68 << 1)

// Register 117 (0x75) - WHO_AM_I - повертає 0x68 якщо датчик знайдений
#define MPU6050_REG_WHO_AM_I 0x75
// Register 117 (0x75) - WHO_AM_I - очікуване значення при успішному підключенні
#define MPU6050_WHO_AM_I_VALUE 0x68

// Register 107 (0x6B) - PWR_MGMT_1 - управління живленням, при старті = sleep mode
#define MPU6050_REG_PWR_MGMT_1 0x6B

// Register 67 (0x43) - GYRO_XOUT_H - початок регістрів гіроскопу (X, Y, Z по 2 байти)
#define MPU6050_REG_GYRO_XOUT_H 0x43
// Register 59 (0x3B) - ACCEL_XOUT_H - початок регістрів акселерометру (X, Y, Z по 2 байти)
#define MPU6050_REG_ACCEL_XOUT_H 0x3B

class MPU6050
{
   public:
    static MPU6050& instance();

    bool init();
    bool readAccel(float& pitch, float& roll);
    bool readGyro(float& Gx_dps, float& Gy_dps, float& Gz_dps,  //
                  float offset_x, float offset_y, float offset_z);
    bool calibrateGyro(float& offset_x, float& offset_y, float& offset_z);
    bool waitReady();

   private:
    static constexpr const char* TAG = "mpu6050";
    MPU6050() = default;
};