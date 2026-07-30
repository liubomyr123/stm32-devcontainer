#include "mpu6050.hpp"

bool MPU6050::init() const
{
    uint8_t check;
    uint8_t Data;

    // check device ID WHO_AM_I

    HAL_StatusTypeDef status =
        HAL_I2C_Mem_Read(hi2c1, MPU6050_ADDR, WHO_AM_I_REG, 1, &check, 1, i2c_timeout);
    LOG_INFO("GYRO", "WHO_AM_I status=%d check=0x%02X", status, check);

    if (check == 104)  // 0x68 will be returned by the sensor if everything goes well
    {
        // power management register 0X6B we should write all 0's to wake the sensor up
        Data = 0;
        HAL_I2C_Mem_Write(hi2c1, MPU6050_ADDR, PWR_MGMT_1_REG, 1, &Data, 1, i2c_timeout);

        // Set DATA RATE of 1KHz by writing SMPLRT_DIV register
        Data = 0x07;
        HAL_I2C_Mem_Write(hi2c1, MPU6050_ADDR, SMPLRT_DIV_REG, 1, &Data, 1, i2c_timeout);

        // Set accelerometer configuration in ACCEL_CONFIG Register
        // XA_ST=0,YA_ST=0,ZA_ST=0, FS_SEL=0 -> � 2g
        Data = 0x00;
        HAL_I2C_Mem_Write(hi2c1, MPU6050_ADDR, ACCEL_CONFIG_REG, 1, &Data, 1, i2c_timeout);

        // Set Gyroscopic configuration in GYRO_CONFIG Register
        // XG_ST=0,YG_ST=0,ZG_ST=0, FS_SEL=0 -> � 250 �/s
        Data = 0x00;
        HAL_I2C_Mem_Write(hi2c1, MPU6050_ADDR, GYRO_CONFIG_REG, 1, &Data, 1, i2c_timeout);
        return true;
    }
    return false;
}

void MPU6050::readAccel()
{
    std::array<uint8_t, 6> Rec_Data{};

    // Read 6 BYTES of data starting from ACCEL_XOUT_H register

    HAL_I2C_Mem_Read(hi2c1, MPU6050_ADDR, ACCEL_XOUT_H_REG, 1, Rec_Data.data(), 6, i2c_timeout);

    data.Accel_X_RAW = (int16_t)(Rec_Data[0] << 8 | Rec_Data[1]);
    data.Accel_Y_RAW = (int16_t)(Rec_Data[2] << 8 | Rec_Data[3]);
    data.Accel_Z_RAW = (int16_t)(Rec_Data[4] << 8 | Rec_Data[5]);

    /*** convert the RAW values into acceleration in 'g'
         we have to divide according to the Full scale value set in FS_SEL
         I have configured FS_SEL = 0. So I am dividing by 16384.0
         for more details check ACCEL_CONFIG Register
    ****/

    data.Ax = data.Accel_X_RAW / 16384.0;
    data.Ay = data.Accel_Y_RAW / 16384.0;
    data.Az = data.Accel_Z_RAW / Accel_Z_corrector;
}

void MPU6050::readGyro()
{
    std::array<uint8_t, 6> Rec_Data{};

    // Read 6 BYTES of data starting from GYRO_XOUT_H register

    HAL_I2C_Mem_Read(hi2c1, MPU6050_ADDR, GYRO_XOUT_H_REG,  //
                     1, Rec_Data.data(), 6, i2c_timeout);

    data.Gyro_X_RAW = (int16_t)(Rec_Data[0] << 8 | Rec_Data[1]);
    data.Gyro_Y_RAW = (int16_t)(Rec_Data[2] << 8 | Rec_Data[3]);
    data.Gyro_Z_RAW = (int16_t)(Rec_Data[4] << 8 | Rec_Data[5]);

    /*** convert the RAW values into dps (�/s)
         we have to divide according to the Full scale value set in FS_SEL
         I have configured FS_SEL = 0. So I am dividing by 131.0
         for more details check GYRO_CONFIG Register
    ****/

    data.Gx = data.Gyro_X_RAW / 131.0;
    data.Gy = data.Gyro_Y_RAW / 131.0;
    data.Gz = data.Gyro_Z_RAW / 131.0;
}

void MPU6050::readTemp()
{
    std::array<uint8_t, 2> Rec_Data{};
    int16_t temp;

    // Read 2 BYTES of data starting from TEMP_OUT_H_REG register

    HAL_I2C_Mem_Read(hi2c1, MPU6050_ADDR, TEMP_OUT_H_REG,  //
                     1, Rec_Data.data(), 2, i2c_timeout);

    temp = (int16_t)(Rec_Data[0] << 8 | Rec_Data[1]);
    data.Temperature = (float)((int16_t)temp / (float)340.0 + (float)36.53);
}

void MPU6050::readAll()
{
    std::array<uint8_t, 14> Rec_Data{};
    int16_t temp;

    // Read 14 BYTES of data starting from ACCEL_XOUT_H register

    HAL_I2C_Mem_Read(hi2c1, MPU6050_ADDR, ACCEL_XOUT_H_REG,  //
                     1, Rec_Data.data(), 14, i2c_timeout);

    data.Accel_X_RAW = (int16_t)(Rec_Data[0] << 8 | Rec_Data[1]);
    data.Accel_Y_RAW = (int16_t)(Rec_Data[2] << 8 | Rec_Data[3]);
    data.Accel_Z_RAW = (int16_t)(Rec_Data[4] << 8 | Rec_Data[5]);
    temp = (int16_t)(Rec_Data[6] << 8 | Rec_Data[7]);
    data.Gyro_X_RAW = (int16_t)(Rec_Data[8] << 8 | Rec_Data[9]);
    data.Gyro_Y_RAW = (int16_t)(Rec_Data[10] << 8 | Rec_Data[11]);
    data.Gyro_Z_RAW = (int16_t)(Rec_Data[12] << 8 | Rec_Data[13]);

    data.Ax = data.Accel_X_RAW / 16384.0;
    data.Ay = data.Accel_Y_RAW / 16384.0;
    data.Az = data.Accel_Z_RAW / Accel_Z_corrector;
    data.Temperature = (float)((int16_t)temp / (float)340.0 + (float)36.53);
    data.Gx = data.Gyro_X_RAW / 131.0;
    data.Gy = data.Gyro_Y_RAW / 131.0;
    data.Gz = data.Gyro_Z_RAW / 131.0;

    // Kalman angle solve
    double dt = (double)(HAL_GetTick() - timer) / 1000;
    timer = HAL_GetTick();
    double roll;
    double roll_sqrt =
        sqrt(data.Accel_X_RAW * data.Accel_X_RAW + data.Accel_Z_RAW * data.Accel_Z_RAW);
    if (roll_sqrt != 0.0)
    {
        roll = atan(data.Accel_Y_RAW / roll_sqrt) * RAD_TO_DEG;
    }
    else
    {
        roll = 0.0;
    }
    double pitch = atan2(-data.Accel_X_RAW, data.Accel_Z_RAW) * RAD_TO_DEG;
    if ((pitch < -90 && data.KalmanAngleY > 90) || (pitch > 90 && data.KalmanAngleY < -90))
    {
        KalmanY.angle = pitch;
        data.KalmanAngleY = pitch;
    }
    else
    {
        data.KalmanAngleY = kalmanGetAngle(&KalmanY, pitch, data.Gy, dt);
    }
    if (fabs(data.KalmanAngleY) > 90)
    {
        data.Gx = -data.Gx;
    }
    data.KalmanAngleX = kalmanGetAngle(&KalmanX, roll, data.Gx, dt);
}

double MPU6050::kalmanGetAngle(Kalman_t *Kalman, double newAngle, double newRate, double dt)
{
    double rate = newRate - Kalman->bias;
    Kalman->angle += dt * rate;

    Kalman->P[0][0] += dt *                   //
                       (dt * Kalman->P[1][1]  //
                        - Kalman->P[0][1]     //
                        - Kalman->P[1][0]     //
                        + Kalman->Q_angle);

    Kalman->P[0][1] -= dt * Kalman->P[1][1];
    Kalman->P[1][0] -= dt * Kalman->P[1][1];
    Kalman->P[1][1] += Kalman->Q_bias * dt;

    double S = Kalman->P[0][0] + Kalman->R_measure;
    std::array<double, 2> K{};
    K[0] = Kalman->P[0][0] / S;
    K[1] = Kalman->P[1][0] / S;

    double y = newAngle - Kalman->angle;
    Kalman->angle += K[0] * y;
    Kalman->bias += K[1] * y;

    double P00_temp = Kalman->P[0][0];
    double P01_temp = Kalman->P[0][1];

    Kalman->P[0][0] -= K[0] * P00_temp;
    Kalman->P[0][1] -= K[0] * P01_temp;
    Kalman->P[1][0] -= K[1] * P00_temp;
    Kalman->P[1][1] -= K[1] * P01_temp;

    return Kalman->angle;
}
