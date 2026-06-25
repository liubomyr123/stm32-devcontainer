#include "include/button.h"

Button::Button(GPIO_TypeDef* port, uint16_t pin)
    : port_(port), pin_(pin) {}

bool Button::isPressed() {
    return HAL_GPIO_ReadPin(port_, pin_) == GPIO_PIN_SET;
}