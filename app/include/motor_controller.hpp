#pragma once

#include <cstdint>

#include "uart_config.h"

enum class Motor
{
    FL,  // Front Left
    FR,  // Front Right
    RL,  // Rear Left
    RR   // Rear Right
};

class MotorController
{
   public:
    static MotorController& instance();
    void setMotor(Motor motor, UartCmdType direction, uint8_t speed);
    void apply(UartCmdType direction, int8_t throttle, int8_t steering);
    void stop();

   private:
    MotorController() = default;
    void setLeft(int16_t speed);
    void setRight(int16_t speed);
};
