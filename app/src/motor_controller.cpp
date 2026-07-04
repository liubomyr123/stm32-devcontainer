#include "include/motor_controller.hpp"

#include <array>

#include "main.h"
#include "uart_config.h"

extern TIM_HandleTypeDef htim3;

MotorController& MotorController::instance()
{
    static MotorController instance;
    return instance;
}

struct MotorConfig
{
    GPIO_TypeDef* in1Port;
    uint16_t in1Pin;

    GPIO_TypeDef* in2Port;
    uint16_t in2Pin;

    uint32_t channel;
};

void MotorController::setMotor(Motor motor, UartCmdType direction, uint8_t speed)
{
    static const std::array<MotorConfig, 4> motorConfigs = {{
        {GPIOC, GPIO_PIN_0, GPIOC, GPIO_PIN_1, TIM_CHANNEL_1},  // FL
        {GPIOC, GPIO_PIN_2, GPIOC, GPIO_PIN_3, TIM_CHANNEL_2},  // FR
        {GPIOA, GPIO_PIN_5, GPIOB, GPIO_PIN_2, TIM_CHANNEL_3},  // RL
        {GPIOA, GPIO_PIN_3, GPIOA, GPIO_PIN_4, TIM_CHANNEL_4},  // RR
    }};
    const MotorConfig& cfg = motorConfigs.at((size_t)motor);
    switch (direction)
    {
        case CMD_FORWARD:
        {
            HAL_GPIO_WritePin(cfg.in1Port, cfg.in1Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(cfg.in2Port, cfg.in2Pin, GPIO_PIN_RESET);
            break;
        }
        case CMD_STOP:
        {
            HAL_GPIO_WritePin(cfg.in1Port, cfg.in1Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(cfg.in2Port, cfg.in2Pin, GPIO_PIN_RESET);
            speed = 0;
            break;
        }
        case CMD_BACKWARD:
        {
            HAL_GPIO_WritePin(cfg.in1Port, cfg.in1Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(cfg.in2Port, cfg.in2Pin, GPIO_PIN_SET);
            break;
        }
        default:
        {
            break;
        }
    }

    __HAL_TIM_SET_COMPARE(&htim3, cfg.channel, (uint32_t)speed);
}

void MotorController::apply(UartCmdType direction, int8_t throttle, int8_t steering)
{
    (void)steering;
    switch (direction)
    {
        case UartCmdType::CMD_FORWARD:
        {
            setMotor(Motor::FL, CMD_FORWARD, throttle);
            setMotor(Motor::RL, CMD_FORWARD, throttle);
            setMotor(Motor::FR, CMD_FORWARD, throttle);
            setMotor(Motor::RR, CMD_FORWARD, throttle);
            break;
        }
        case UartCmdType::CMD_BACKWARD:
        {
            setMotor(Motor::FL, CMD_BACKWARD, throttle);
            setMotor(Motor::RL, CMD_BACKWARD, throttle);
            setMotor(Motor::FR, CMD_BACKWARD, throttle);
            setMotor(Motor::RR, CMD_BACKWARD, throttle);
            break;
        }
        case UartCmdType::CMD_LEFT:
        {
            break;
        }
        case UartCmdType::CMD_RIGHT:
        {
            break;
        }
        case UartCmdType::CMD_STOP:
        {
            setMotor(Motor::FL, CMD_STOP, 0);
            setMotor(Motor::FR, CMD_STOP, 0);
            setMotor(Motor::RL, CMD_STOP, 0);
            setMotor(Motor::RR, CMD_STOP, 0);
            break;
        }
        case UartCmdType::CMD_UNKNOWN:
        {
            break;
        }
        default:
        {
            break;
        }
    }
}

void MotorController::stop()
{
    apply(UartCmdType::CMD_STOP, 0, 0);
}
