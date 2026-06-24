#ifndef LED_H
#define LED_H

#include "stm32f4xx_hal.h"

void LED_Init(void);
void LED_Toggle(void);
void LED_Delay(uint32_t ms);

#endif
