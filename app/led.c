#include "led.h"

void LED_Init(void)
{
    // тут нічого не треба, GPIO вже CubeMX налаштував
}

void LED_Toggle(void)
{
    HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_13);
}

void LED_Delay(uint32_t ms)
{
    HAL_Delay(ms);
}
