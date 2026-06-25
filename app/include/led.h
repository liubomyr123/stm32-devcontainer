#ifndef LED_H
#define LED_H

#include "stm32f4xx_hal.h"

#ifdef __cplusplus

class LED {
public:
    LED(GPIO_TypeDef* port, uint16_t pin);
    void on();
    void off();
    void toggle();
    void delay(uint32_t ms);

private:
    GPIO_TypeDef* port_;
    uint16_t pin_;
};

#endif

#endif