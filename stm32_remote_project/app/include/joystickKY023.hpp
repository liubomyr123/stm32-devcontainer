#pragma once
#include <array>
#include <cstdint>

#include "cmsis_os.h"
#include "include/logger.hpp"
#include "main.h"

class JoystickKY023
{
   public:
    bool init();
    JoystickKY023(ADC_HandleTypeDef* hadc,  //
                  TIM_HandleTypeDef* htim,  //
                  uint16_t adc_channel_x,   //
                  uint16_t adc_channel_y,   //
                  GPIO_TypeDef* GPIOx_SW,   //
                  uint16_t GPIO_Pin_SW)
        : hadc_(hadc),
          htim_(htim),
          ADC_CHANNEL_X_(adc_channel_x),
          ADC_CHANNEL_Y_(adc_channel_y),
          GPIOx_SW_(GPIOx_SW),
          GPIO_Pin_SW_(GPIO_Pin_SW)
    {
    }

    // bool readAdcChannelXRaw(uint16_t& out);
    // bool readAdcChannelYRaw(uint16_t& out);
    bool readAdcChannelXFiltered(uint16_t& out);
    bool readAdcChannelYFiltered(uint16_t& out);
    bool readAdcChannelXPercentage(int16_t& out);
    bool readAdcChannelYPercentage(int16_t& out);
    GPIO_PinState readSwButton();
    uint16_t getAdcMaxValue(ADC_HandleTypeDef* hadc);

   private:
    static constexpr const char* TAG = "JOYSTICK";
    static constexpr float MAX_STEP_PERCENT = 0.1F;

    // NOLINTNEXTLINE(bugprone-dynamic-static-initializers)
    static bool dmaStarted_;

    // TIM2 генерує TRGO-сигнал кожні ADC_POLL_INTERVAL_MS мс, і КОЖЕН такий сигнал запускає ОДНЕ
    // повне сканування всіх 4 каналів (X1, Y1, X2, Y2)
    static constexpr uint32_t ADC_POLL_INTERVAL_MS = 20;

    ADC_HandleTypeDef* hadc_ = nullptr;
    TIM_HandleTypeDef* htim_ = nullptr;

    uint16_t ADC_CHANNEL_X_{0};
    uint16_t ADC_CHANNEL_Y_{0};

    GPIO_TypeDef* GPIOx_SW_ = nullptr;
    uint16_t GPIO_Pin_SW_{0};

    // bool readAdcChannelRaw(uint32_t channel, uint16_t& out);
    bool readAdcChannelFiltered(uint32_t channel, uint16_t& out);
    bool readAdcChannelPercentage(uint32_t channel, int16_t& out);
    uint16_t applyRateLimit(uint16_t newValue, uint16_t& previousValue) const;
    bool applyChannelRateLimit(uint32_t channel, uint16_t newValue, uint16_t& out);
    int getChannelIndex(uint32_t channel) const;
    bool startAdcSampling();

    uint16_t ADC_MAX_VALUE_{0};
    uint16_t previousX_{0};
    uint16_t previousY_{0};

    uint16_t maxStep_{0};
};
