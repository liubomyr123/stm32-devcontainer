#include "mpu6050.hpp"

extern I2C_HandleTypeDef hi2c1;

MPU6050& MPU6050::instance()
{
    static MPU6050 instance;
    return instance;
}

bool MPU6050::init()
{
    uint8_t check;
    uint8_t data;

    // Перевіряємо WHO_AM_I регістр
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1, MPU6050_I2C_ADDR,  //
                                                MPU6050_REG_WHO_AM_I, 1, &check, 1, 100);

    if (status != HAL_OK)
    {
        LOG_ERROR(TAG, "init WHO_AM_I read error: %d", status);
        return false;
    }
    if (check != MPU6050_WHO_AM_I_VALUE)
    {
        LOG_ERROR(TAG, "MPU6050 not found, WHO_AM_I=0x%02X", check);
        return false;
    }

    // By default after wake up MPU-6050 is sleeping
    // PWR_MGMT_1 Register (0x6B)
    data = (0 << 7) |  // DEVICE_RESET = 0
           (0 << 6) |  // SLEEP = 0        <- exit sleep mode
           (0 << 5) |  // CYCLE = 0
           (0 << 4) |  // reserved
           (0 << 3) |  // TEMP_DIS = 0     <- temperature sensor enabled
           (1 << 0);   // CLKSEL = 1       <- PLL with X gyroscope (recommended in datasheet)

    /**
        Upon power up, the MPU-60X0 clock source defaults to the internal oscillator. However, it
        is highly recommended that the device be configured to use one of the gyroscopes (or an
        external clock source) as the clock reference for improved stability.
    */

    status = HAL_I2C_Mem_Write(&hi2c1, MPU6050_I2C_ADDR,  //
                               MPU6050_REG_PWR_MGMT_1,    //
                               1, &data, 1, 100);
    if (status != HAL_OK)
    {
        LOG_ERROR(TAG, "init PWR_MGMT_1 write error: %d", status);
        return false;
    }

    LOG_INFO(TAG, "MPU6050 initialized OK");
    return true;
}

bool MPU6050::waitReady()
{
    uint8_t check = 0;
    for (int i = 0; i < 10; i++)
    {
        HAL_StatusTypeDef status =
            HAL_I2C_Mem_Read(&hi2c1, MPU6050_I2C_ADDR, MPU6050_REG_WHO_AM_I, 1, &check, 1, 100);
        if (status == HAL_OK && check == MPU6050_WHO_AM_I_VALUE)
        {
            return true;
        }
        LOG_WARNING(TAG, "status=%d whoami=0x%02X", status, check);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    LOG_ERROR(TAG, "MPU6050 not ready");
    return false;
}

bool MPU6050::readAccel(float& pitch, float& roll)
{
    std::array<uint8_t, 6> buf{};
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1, MPU6050_I2C_ADDR,  //
                                                MPU6050_REG_ACCEL_XOUT_H,  //
                                                1, buf.data(), 6, 100);

    if (status != HAL_OK)
    {
        LOG_ERROR(TAG, "readAccel I2C error: %d", status);
        return false;
    }

    float Ax = static_cast<float>((int16_t)(buf[0] << 8 | buf[1])) / 16384.0F;
    float Ay = static_cast<float>((int16_t)(buf[2] << 8 | buf[3])) / 16384.0F;
    float Az = static_cast<float>((int16_t)(buf[4] << 8 | buf[5])) / 16384.0F;

    /**
        Pitch:  їде на гірку/з гірки
        Roll:   перевернулась або нахилилась вбік
     */

    pitch = atan2f(Ax, Az) * 180.0F / 3.14159F;
    roll = atan2f(Ay, Az) * 180.0F / 3.14159F;
    return true;
}

bool MPU6050::readGyro(float& Gx_dps, float& Gy_dps, float& Gz_dps,  //
                       float offset_x, float offset_y, float offset_z)
{
    std::array<uint8_t, 6> buf{};
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1, MPU6050_I2C_ADDR,  //
                                                MPU6050_REG_GYRO_XOUT_H,   //
                                                1, buf.data(), 6, 100);

    if (status != HAL_OK)
    {
        LOG_ERROR(TAG, "readGyro I2C error: %d", status);
        return false;
    }

    Gx_dps = static_cast<float>((int16_t)(buf[0] << 8 | buf[1])) / 131.0F - offset_x;
    Gy_dps = static_cast<float>((int16_t)(buf[2] << 8 | buf[3])) / 131.0F - offset_y;
    Gz_dps = static_cast<float>((int16_t)(buf[4] << 8 | buf[5])) / 131.0F - offset_z;
    return true;
}

bool MPU6050::calibrateGyro(float& offset_x, float& offset_y, float& offset_z)
{
    const int samples = 100;
    float sum_x = 0.0F;
    float sum_y = 0.0F;
    float sum_z = 0.0F;
    std::array<uint8_t, 6> buf{};

    LOG_INFO(TAG, "Calibrating... keep sensor still");

    for (int i = 0; i < samples; i++)
    {
        HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1, MPU6050_I2C_ADDR,  //
                                                    MPU6050_REG_GYRO_XOUT_H,   //
                                                    1, buf.data(), 6, 100);

        if (status != HAL_OK)
        {
            LOG_ERROR(TAG, "calibrateGyro I2C error: %d", status);
            return false;
        }

        sum_x += static_cast<float>((int16_t)(buf[0] << 8 | buf[1])) / 131.0F;
        sum_y += static_cast<float>((int16_t)(buf[2] << 8 | buf[3])) / 131.0F;
        sum_z += static_cast<float>((int16_t)(buf[4] << 8 | buf[5])) / 131.0F;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    offset_x = sum_x / static_cast<float>(samples);
    offset_y = sum_y / static_cast<float>(samples);
    offset_z = sum_z / static_cast<float>(samples);

    LOG_INFO(TAG, "Offsets: Gx=%.2f Gy=%.2f Gz=%.2f", offset_x, offset_y, offset_z);
    return true;
}