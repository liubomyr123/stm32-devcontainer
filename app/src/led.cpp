#include "include/led.h"

LED::LED(GPIO_TypeDef* port, uint16_t pin)
    : port_(port), pin_(pin) {}

void LED::on() {
    HAL_GPIO_WritePin(port_, pin_, GPIO_PIN_SET);
}

void LED::off() {
    HAL_GPIO_WritePin(port_, pin_, GPIO_PIN_RESET);
}

void LED::toggle() {
    HAL_GPIO_TogglePin(port_, pin_);
}

void LED::delay(uint32_t ms) {
    HAL_Delay(ms);
}
