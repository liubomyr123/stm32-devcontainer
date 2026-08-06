#pragma once

#include <cstdint>

#include "main.h"

class ServoSG90
{
   public:
    static ServoSG90& instance();

    bool init();
    void tick();
    void setTargetAngle(int8_t angle);

   private:
    static constexpr const char* TAG = "SERVOSG90";
    ServoSG90() = default;

    bool setPulse(uint16_t pulse_us) const;
    uint16_t convertAngleToPulse() const;

    static constexpr int16_t PY_MIN = -100;
    static constexpr int16_t PY_MAX = 100;

    static constexpr uint16_t PULSE_MIN = 1000;     // -90°
    static constexpr uint16_t PULSE_CENTER = 1500;  //   0°
    static constexpr uint16_t PULSE_MAX = 2000;     // +90°
    static constexpr uint16_t MAX_STEP = 30;

    volatile int8_t target_angle_ = 0;
    uint16_t current_pulse_ = PULSE_CENTER;
};