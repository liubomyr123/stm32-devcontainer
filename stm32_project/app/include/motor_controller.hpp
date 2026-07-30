#pragma once

#include <array>
#include <cstdint>

#include "main.h"
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
    void apply(UartCmd cmd_);
    void stop();

   private:
    static constexpr const char* TAG = "MOTOR_CONTROLLER";
    MotorController() = default;
    void setLeft(int16_t speed);
    void setRight(int16_t speed);
};
