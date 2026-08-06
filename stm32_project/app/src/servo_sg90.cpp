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
    current_pulse_ = PULSE_CENTER;
    setPulse(PULSE_CENTER);

    LOG_INFO(TAG, "Initialized, centered at %d us", PULSE_CENTER);
    return true;
}

bool ServoSG90::setPulse(uint16_t pulse_us) const
{
    if (pulse_us < PULSE_MIN || pulse_us > PULSE_MAX)
    {
        LOG_ERROR(TAG, "setPulse out of range: %u (expected %u..%u)", pulse_us, PULSE_MIN,
                  PULSE_MAX);
        return false;
    }

    __HAL_TIM_SET_COMPARE(&htim11, TIM_CHANNEL_1, pulse_us);
    return true;
}

void ServoSG90::setTargetAngle(int8_t angle)
{
    target_angle_ = angle;
}

uint16_t ServoSG90::convertAngleToPulse() const
{
    // Скільки мкс займає весь діапазон серво
    // Від 90° до -90° в мкс = 2000 - 1000 = 1000 мкс
    const auto pulse_range = static_cast<int16_t>(PULSE_MAX - PULSE_MIN);

    // Скільки одиниць джойстика вкладається у весь діапазон
    // Значення коливається між -100 та 100, отже загально -> 200
    const auto py_range = static_cast<int16_t>(PY_MAX - PY_MIN);

    // Скільки мкс припадає на одну одиницю джойстика
    const auto us_per_unit = static_cast<int16_t>(pulse_range / py_range);

    // На скільки треба зсунутись мікросекунд від центру.
    // Наприклад якщо py = 50, тоді треба зсунутись на 250 мікросекунд від центру (50 * 5 = 250)
    const auto offset = static_cast<int16_t>(target_angle_ * us_per_unit);
    auto pulse = static_cast<int16_t>(PULSE_CENTER + offset);
    if (pulse < PULSE_MIN)
    {
        pulse = PULSE_MIN;
    }
    if (pulse > PULSE_MAX)
    {
        pulse = PULSE_MAX;
    }

    return static_cast<uint16_t>(pulse);
}

void ServoSG90::tick()
{
    uint16_t target = convertAngleToPulse();
    uint16_t previous_pulse = current_pulse_;

    if (current_pulse_ < target)
    {
        current_pulse_ = static_cast<uint16_t>(current_pulse_ + MAX_STEP);
        if (current_pulse_ > target)
        {
            current_pulse_ = target;
        }
    }
    else if (current_pulse_ > target)
    {
        current_pulse_ = static_cast<uint16_t>(current_pulse_ - MAX_STEP);
        if (current_pulse_ < target)
        {
            current_pulse_ = target;
        }
    }

    if (!setPulse(current_pulse_))
    {
        LOG_WARNING(TAG, "Failed to set pulse, servo may be stuck");
        current_pulse_ = previous_pulse;
    }
}