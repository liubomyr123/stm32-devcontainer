#include "include/servo_sg90.hpp"

#include "include/logger.hpp"

extern TIM_HandleTypeDef htim11;

ServoSG90& ServoSG90::instance()
{
    static ServoSG90 instance;
    return instance;
}

bool ServoSG90::init()
{
    if (HAL_TIM_PWM_Start(&htim11, TIM_CHANNEL_1) != HAL_OK)
    {
        LOG_ERROR(TAG, "Failed to start PWM");
        return false;
    }
    setPulse(PULSE_CENTER);
    LOG_INFO(TAG, "Initialized, centered at %d us", PULSE_CENTER);
    return true;
}

void ServoSG90::setPulse(uint16_t pulse_us)
{
    __HAL_TIM_SET_COMPARE(&htim11, TIM_CHANNEL_1, pulse_us);
}
