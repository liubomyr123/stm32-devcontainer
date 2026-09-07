#include "joystickKY023.hpp"

#include <cstdint>

bool JoystickKY023::readAdcChannelYFiltered(uint16_t& out)
{
    return readAdcChannelFiltered(ADC_CHANNEL_Y_, out);
}

bool JoystickKY023::readAdcChannelXFiltered(uint16_t& out)
{
    return readAdcChannelFiltered(ADC_CHANNEL_X_, out);
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