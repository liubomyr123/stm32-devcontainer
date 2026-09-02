#pragma once

#include "cmsis_os.h"
#include "include/logger.hpp"

class JoystickKY023
{
   public:
    JoystickKY023(ADC_HandleTypeDef* hadc, uint16_t adc_channel_x, uint16_t adc_channel_y,
                  GPIO_TypeDef* GPIOx_SW, uint16_t GPIO_Pin_SW)
        : hadc_(hadc),
          ADC_CHANNEL_X_(adc_channel_x),
          ADC_CHANNEL_Y_(adc_channel_y),
          GPIOx_SW_(GPIOx_SW),
          GPIO_Pin_SW_(GPIO_Pin_SW)
    {
    }

    uint16_t readAdcChannelX();
    uint16_t readAdcChannelY();
    GPIO_PinState readSwButton();

   private:
    ADC_HandleTypeDef* hadc_;

    uint16_t ADC_CHANNEL_X_;
    uint16_t ADC_CHANNEL_Y_;

    GPIO_TypeDef* GPIOx_SW_;
    uint16_t GPIO_Pin_SW_;

    uint16_t readAdcChannel(uint32_t channel);
};
