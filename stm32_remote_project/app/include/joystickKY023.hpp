#pragma once
#include <array>
#include <cstdint>

#include "cmsis_os.h"
#include "include/logger.hpp"

class JoystickKY023
{
   public:
    bool init();
    JoystickKY023(ADC_HandleTypeDef* hadc, uint16_t adc_channel_x, uint16_t adc_channel_y,
                  GPIO_TypeDef* GPIOx_SW, uint16_t GPIO_Pin_SW)
        : hadc_(hadc),
          ADC_CHANNEL_X_(adc_channel_x),
          ADC_CHANNEL_Y_(adc_channel_y),
          GPIOx_SW_(GPIOx_SW),
          GPIO_Pin_SW_(GPIO_Pin_SW)
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
    static constexpr float MAX_STEP_PERCENT = 0.1F;

    ADC_HandleTypeDef* hadc_ = nullptr;

    uint16_t ADC_CHANNEL_X_{0};
    uint16_t ADC_CHANNEL_Y_{0};

    GPIO_TypeDef* GPIOx_SW_ = nullptr;
    uint16_t GPIO_Pin_SW_{0};

    bool readAdcChannelRaw(uint32_t channel, uint16_t& out);
    bool readAdcChannelFiltered(uint32_t channel, uint16_t& out);
    uint16_t applyRateLimit(uint16_t newValue, uint16_t& previousValue) const;

    uint16_t ADC_MAX_VALUE_{0};
    uint16_t previousX_{0};
    uint16_t previousY_{0};

    uint16_t maxStep_{0};
};
