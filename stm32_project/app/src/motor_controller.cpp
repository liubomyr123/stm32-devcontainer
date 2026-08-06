#include "include/motor_controller.hpp"

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

bool MotorController::init()
{
    if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
    {
        return false;
    }
    if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2) != HAL_OK)
    {
        return false;
    }
    if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3) != HAL_OK)
    {
        return false;
    }
    if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4) != HAL_OK)
    {
        return false;
    }

    // STBY1 — драйвер FL/FR
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    // STBY2 — драйвер RL/RR
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_SET);

    return true;
}

void MotorController::setMotor(Motor motor, UartCmdType direction, uint8_t speed)
{
    static const std::array<MotorConfig, 4> motorConfigs = {{
        {GPIOC, GPIO_PIN_0, GPIOC, GPIO_PIN_1, TIM_CHANNEL_1},  // FL
        {GPIOC, GPIO_PIN_2, GPIOC, GPIO_PIN_3, TIM_CHANNEL_2},  // FR
        {GPIOA, GPIO_PIN_3, GPIOA, GPIO_PIN_4, TIM_CHANNEL_4},  // RL
        {GPIOA, GPIO_PIN_5, GPIOB, GPIO_PIN_2, TIM_CHANNEL_3},  // RR
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

    switch (motor)
    {
        case Motor::FL:
            current_fl_ = static_cast<int8_t>(direction == CMD_BACKWARD ? -speed : speed);
            break;
        case Motor::FR:
            current_fr_ = static_cast<int8_t>(direction == CMD_BACKWARD ? -speed : speed);
            break;
        case Motor::RL:
            current_rl_ = static_cast<int8_t>(direction == CMD_BACKWARD ? -speed : speed);
            break;
        case Motor::RR:
            current_rr_ = static_cast<int8_t>(direction == CMD_BACKWARD ? -speed : speed);
            break;
    }
}

void MotorController::apply(UartCmd cmd_)
{
    auto forward = static_cast<int16_t>(cmd_.f - cmd_.b);
    auto steer = static_cast<int16_t>(cmd_.r - cmd_.l);

    auto left = static_cast<int16_t>(forward - steer);
    auto right = static_cast<int16_t>(forward + steer);

    left = std::max((int16_t)-100, std::min((int16_t)100, left));
    right = std::max((int16_t)-100, std::min((int16_t)100, right));

    auto dir = [](int16_t v) -> UartCmdType
    {
        if (v > 0)
        {
            return CMD_FORWARD;
        }
        if (v < 0)
        {
            return CMD_BACKWARD;
        }
        return CMD_STOP;
    };

    auto spd = [](int16_t v) -> uint8_t {  //
        return static_cast<uint8_t>(std::abs(v));
    };

    setMotor(Motor::FL, dir(left), spd(left));
    setMotor(Motor::RL, dir(left), spd(left));
    setMotor(Motor::FR, dir(right), spd(right));
    setMotor(Motor::RR, dir(right), spd(right));
}

void MotorController::stop()
{
    UartCmd stop_cmd{};
    stop_cmd.type = CMD_STOP;
    apply(stop_cmd);
}
