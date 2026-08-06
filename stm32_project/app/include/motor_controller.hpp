#pragma once

#include <array>
#include <cstdint>
#include <cstdlib>

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

    bool init();
    void apply(UartCmd cmd_);
    void stop();

    int8_t getFL() const
    {
        return current_fl_;
    }
    int8_t getFR() const
    {
        return current_fr_;
    }
    int8_t getRL() const
    {
        return current_rl_;
    }
    int8_t getRR() const
    {
        return current_rr_;
    }

   private:
    static constexpr const char* TAG = "MOTOR_CONTROLLER";
    MotorController() = default;
    void setMotor(Motor motor, UartCmdType direction, uint8_t speed);
    void setLeft(int16_t speed);
    void setRight(int16_t speed);

    int8_t current_fl_ = 0;
    int8_t current_fr_ = 0;
    int8_t current_rl_ = 0;
    int8_t current_rr_ = 0;
};
