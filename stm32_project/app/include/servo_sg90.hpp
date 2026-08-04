#pragma once

#include <cstdint>

#include "main.h"

class ServoSG90
{
   public:
    static ServoSG90& instance();

    bool init();
    void setPulse(uint16_t pulse_us);

   private:
    static constexpr const char* TAG = "SERVOSG90";
    ServoSG90() = default;

    static constexpr uint16_t PULSE_MIN = 1000;     // -90°
    static constexpr uint16_t PULSE_CENTER = 1500;  //   0°
    static constexpr uint16_t PULSE_MAX = 2000;     // +90°
};