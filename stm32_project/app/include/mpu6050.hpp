#pragma once

#include <array>
#include <cmath>

#include "cmsis_os.h"
#include "include/logger.hpp"

// MPU6050 structure
typedef struct
{
    int16_t Accel_X_RAW;
    int16_t Accel_Y_RAW;
    int16_t Accel_Z_RAW;
    double Ax;
    double Ay;
    double Az;

    int16_t Gyro_X_RAW;
    int16_t Gyro_Y_RAW;
    int16_t Gyro_Z_RAW;
    double Gx;
    double Gy;
    double Gz;

    float Temperature;

    double KalmanAngleX;  // Roll
    double KalmanAngleY;  // Pitch
} MPU6050_t;

// Kalman structure
typedef struct
{
    double Q_angle;
    double Q_bias;
    double R_measure;
    double angle;
    double bias;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
    double P[2][2];
} Kalman_t;

#define RAD_TO_DEG 57.295779513082320876798154814105

#define WHO_AM_I_REG 0x75
#define PWR_MGMT_1_REG 0x6B
#define SMPLRT_DIV_REG 0x19
#define ACCEL_CONFIG_REG 0x1C
#define ACCEL_XOUT_H_REG 0x3B
#define TEMP_OUT_H_REG 0x41
#define GYRO_CONFIG_REG 0x1B
#define GYRO_XOUT_H_REG 0x43

// Setup MPU6050
#define MPU6050_ADDR 0xD0

class MPU6050
{
   public:
    bool init() const;

    bool readAccel();
    bool readGyro();
    bool readTemp();
    bool readAll();

    const double& getPitch() const
    {
        return data.KalmanAngleY;
    }
    const double& getRoll() const
    {
        return data.KalmanAngleX;
    }
    const float& getTemp() const
    {
        return data.Temperature;
    }

    MPU6050(I2C_HandleTypeDef* hi2c1_) : hi2c1(hi2c1_)
    {
        KalmanX.Q_angle = 0.001F;
        KalmanX.Q_bias = 0.003F;
        KalmanX.R_measure = 0.03F;

        KalmanY.Q_angle = 0.001F;
        KalmanY.Q_bias = 0.003F;
        KalmanY.R_measure = 0.03F;
    };

   private:
    static constexpr const char* TAG = "MPU6050";

    double kalmanGetAngle(Kalman_t* Kalman, double newAngle, double newRate, double dt);

    MPU6050_t data{};
    I2C_HandleTypeDef* hi2c1;
    uint32_t timer{};
    Kalman_t KalmanX{};
    Kalman_t KalmanY{};

    const uint16_t i2c_timeout = 100;
    const double Accel_Z_corrector = 14418.0;
};