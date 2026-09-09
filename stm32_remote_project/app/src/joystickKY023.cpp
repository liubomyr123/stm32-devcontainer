#include "joystickKY023.hpp"

bool JoystickKY023::init()
{
    if (hadc_ == nullptr)
    {
        LOG_ERROR(TAG, "hadc must be initialised");
        return false;
    }
    if (GPIOx_SW_ == nullptr)
    {
        LOG_ERROR(TAG, "GPIOx_SW_ must be initialised");
        return false;
    }
    if (ADC_CHANNEL_X_ == ADC_CHANNEL_Y_)
    {
        LOG_ERROR(TAG, "ADC_CHANNEL_X_ and ADC_CHANNEL_Y_ must be different");
        return false;
    }
    if (GPIO_Pin_SW_ == 0)
    {
        LOG_ERROR(TAG, "GPIO_Pin_SW_ must be initialised");
        return false;
    }

    ADC_MAX_VALUE_ = getAdcMaxValue(hadc_);

    previousX_ = ADC_MAX_VALUE_ / 2;
    previousY_ = ADC_MAX_VALUE_ / 2;

    maxStep_ = static_cast<uint16_t>(static_cast<float>(ADC_MAX_VALUE_) * MAX_STEP_PERCENT);

    return true;
}

bool JoystickKY023::readAdcChannelXFiltered(uint16_t& out)
{
    uint16_t medianValue = 0;
    if (!readAdcChannelFiltered(ADC_CHANNEL_X_, medianValue))
    {
        return false;
    }

    out = applyRateLimit(medianValue, previousX_);
    return true;
}

bool JoystickKY023::readAdcChannelYFiltered(uint16_t& out)
{
    uint16_t medianValue = 0;
    if (!readAdcChannelFiltered(ADC_CHANNEL_Y_, medianValue))
    {
        return false;
    }

    out = applyRateLimit(medianValue, previousY_);
    return true;
}

bool JoystickKY023::readAdcChannelFiltered(uint32_t channel, uint16_t& out)
{
    constexpr uint8_t SAMPLES = 5;
    std::array<uint16_t, SAMPLES> samples = {0};

    for (size_t i = 0; i < SAMPLES; i++)
    {
        uint16_t adcValue = 0;
        if (!readAdcChannelRaw(channel, adcValue))
        {
            return false;
        }
        samples.at(i) = adcValue;
        osDelay(1);
    }

    for (size_t i = 0; i < SAMPLES - 1; i++)
    {
        for (size_t k = 0; k < SAMPLES - 1 - i; k++)
        {
            if (samples.at(k) > samples.at(k + 1))
            {
                uint16_t temp = samples.at(k);
                samples.at(k) = samples.at(k + 1);
                samples.at(k + 1) = temp;
            }
        }
    }

    out = samples.at(samples.size() / 2);
    return true;
}

bool JoystickKY023::readAdcChannelXRaw(uint16_t& out)
{
    return readAdcChannelRaw(ADC_CHANNEL_X_, out);
}

bool JoystickKY023::readAdcChannelYRaw(uint16_t& out)
{
    return readAdcChannelRaw(ADC_CHANNEL_Y_, out);
}

bool JoystickKY023::readAdcChannelRaw(uint32_t channel, uint16_t& out)
{
    ADC_ChannelConfTypeDef config{};
    config.Channel = channel;
    config.Rank = 1;
    config.SamplingTime = ADC_SAMPLETIME_84CYCLES;

    HAL_StatusTypeDef result = HAL_ADC_ConfigChannel(hadc_, &config);
    if (result != HAL_OK)
    {
        LOG_ERROR(TAG, "HAL_ADC_ConfigChannel failed");
        return false;
    }
    result = HAL_ADC_Start(hadc_);
    if (result != HAL_OK)
    {
        LOG_ERROR(TAG, "HAL_ADC_Start failed");
        return false;
    }
    result = HAL_ADC_PollForConversion(hadc_, HAL_MAX_DELAY);
    if (result != HAL_OK)
    {
        LOG_ERROR(TAG, "HAL_ADC_PollForConversion failed");
        return false;
    }

    uint16_t value = HAL_ADC_GetValue(hadc_);
    result = HAL_ADC_Stop(hadc_);
    if (result != HAL_OK)
    {
        LOG_ERROR(TAG, "HAL_ADC_Stop failed");
        return false;
    }

    out = value;
    return true;
}

GPIO_PinState JoystickKY023::readSwButton()
{
    return HAL_GPIO_ReadPin(GPIOx_SW_, GPIO_Pin_SW_);
}

uint16_t JoystickKY023::getAdcMaxValue(ADC_HandleTypeDef* hadc)
{
    // RM0402 §13.12.2, ADC control register 1 (ADC_CR1)
    // Bits 25:24 RES[1:0]: Resolution
    // "These bits are written by software to select the resolution of the conversion."
    // 00: 12-bit (minimum 15 ADCCLK cycles)
    // 01: 10-bit (minimum 13 ADCCLK cycles)
    // 10: 8-bit (minimum 11 ADCCLK cycles)
    // 11: 6-bit (minimum 9 ADCCLK cycles)
    uint32_t resBits = (hadc->Instance->CR1 & ADC_CR1_RES) >> ADC_CR1_RES_Pos;

    uint8_t bitDepth;
    switch (resBits)
    {
        case 0:
            bitDepth = 12;
            break;
        case 1:
            bitDepth = 10;
            break;
        case 2:
            bitDepth = 8;
            break;
        case 3:
            bitDepth = 6;
            break;
        default:
            bitDepth = 12;
            break;
    }

    return static_cast<uint16_t>((1UL << bitDepth) - 1);
}

uint16_t JoystickKY023::applyRateLimit(uint16_t newValue, uint16_t& previousValue) const
{
    int32_t diff = static_cast<int32_t>(newValue) - static_cast<int32_t>(previousValue);

    if (diff > static_cast<int32_t>(maxStep_))
    {
        previousValue += maxStep_;
    }
    else if (diff < -static_cast<int32_t>(maxStep_))
    {
        previousValue -= maxStep_;
    }
    else
    {
        previousValue = newValue;
    }

    return previousValue;
}

bool JoystickKY023::applyChannelRateLimit(uint32_t channel, uint16_t newValue, uint16_t& out)
{
    if (channel == ADC_CHANNEL_X_)
    {
        out = applyRateLimit(newValue, previousX_);
        return true;
    }
    if (channel == ADC_CHANNEL_Y_)
    {
        out = applyRateLimit(newValue, previousY_);
        return true;
    }

    LOG_ERROR(TAG, "applyChannelRateLimit: unknown channel %lu", channel);
    return false;
}

bool JoystickKY023::readAdcChannelPercentage(uint32_t channel, int16_t& out)
{
    uint16_t medianValue = 0;
    if (!readAdcChannelFiltered(channel, medianValue))
    {
        return false;
    }

    uint16_t smoothedValue = 0;
    if (!applyChannelRateLimit(channel, medianValue, smoothedValue))
    {
        return false;
    }

    int32_t percent = (static_cast<int32_t>(smoothedValue) * 100) / ADC_MAX_VALUE_;
    auto result = static_cast<int16_t>((percent - 50) * 2);

    constexpr int16_t DEAD_ZONE = 5;
    if (result > -DEAD_ZONE && result < DEAD_ZONE)
    {
        result = 0;
    }

    constexpr int16_t EDGE_ZONE = 3;
    if (result > (100 - EDGE_ZONE))
    {
        result = 100;
    }
    else if (result < (-100 + EDGE_ZONE))
    {
        result = -100;
    }

    out = result;
    return true;
}

bool JoystickKY023::readAdcChannelXPercentage(int16_t& out)
{
    return readAdcChannelPercentage(ADC_CHANNEL_X_, out);
}

bool JoystickKY023::readAdcChannelYPercentage(int16_t& out)
{
    return readAdcChannelPercentage(ADC_CHANNEL_Y_, out);
}