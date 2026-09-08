#pragma once
#include <array>

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
          GPIO_Pin_SW_(GPIO_Pin_SW),
          ADC_MAX_VALUE_(getAdcMaxValue(hadc_)),
          previousX_(ADC_MAX_VALUE_ / 2),
          previousY_(ADC_MAX_VALUE_ / 2)
    {
    }

    bool readAdcChannelXRaw(uint16_t& out);
    bool readAdcChannelYRaw(uint16_t& out);
    bool readAdcChannelXFiltered(uint16_t& out);
    bool readAdcChannelYFiltered(uint16_t& out);
    GPIO_PinState readSwButton();
    uint16_t getAdcMaxValue(ADC_HandleTypeDef* hadc);

   private:
    static constexpr const char* TAG = "JOYSTICK";
    static constexpr float MAX_STEP_PERCENT = 0.05F;  // ~1.2% від ADC-діапазону за один цикл

    ADC_HandleTypeDef* hadc_;

    uint16_t ADC_CHANNEL_X_;
    uint16_t ADC_CHANNEL_Y_;

    GPIO_TypeDef* GPIOx_SW_;
    uint16_t GPIO_Pin_SW_;

    bool readAdcChannelRaw(uint32_t channel, uint16_t& out);
    bool readAdcChannelFiltered(uint32_t channel, uint16_t& out);
    uint16_t applyRateLimit(uint16_t newValue, uint16_t& previousValue, uint16_t maxStep);

    uint16_t ADC_MAX_VALUE_;
    uint16_t previousX_;
    uint16_t previousY_;
};
