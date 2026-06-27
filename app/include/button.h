#ifndef BUTTON_H
#define BUTTON_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
}

class Button
{
   public:
    Button(GPIO_TypeDef* port, uint16_t pin);
    bool isPressed();

   private:
    GPIO_TypeDef* port_;
    uint16_t pin_;
};

#endif

#endif