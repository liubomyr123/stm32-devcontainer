#include "joystickKY023.hpp"

uint16_t JoystickKY023::readAdcChannelX()
{
    return readAdcChannel(ADC_CHANNEL_X_);
}

uint16_t JoystickKY023::readAdcChannelY()
{
    return readAdcChannel(ADC_CHANNEL_Y_);
}

uint16_t JoystickKY023::readAdcChannel(uint32_t channel)
{
    ADC_ChannelConfTypeDef config{};
    config.Channel = channel;
    config.Rank = 1;
    config.SamplingTime = ADC_SAMPLETIME_84CYCLES;

    HAL_ADC_ConfigChannel(hadc_, &config);

    HAL_ADC_Start(hadc_);
    HAL_ADC_PollForConversion(hadc_, HAL_MAX_DELAY);
    uint16_t value = HAL_ADC_GetValue(hadc_);
    HAL_ADC_Stop(hadc_);

    return value;
}

GPIO_PinState JoystickKY023::readSwButton()
{
    GPIO_PinState joyState = HAL_GPIO_ReadPin(GPIOx_SW_, GPIO_Pin_SW_);
    return joyState;
}