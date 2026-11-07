#include "../include/helpers.hpp"
#include "../include/radio_car.hpp"

extern SPI_HandleTypeDef hspi1;
extern osThreadId_t RadioTaskHandle;

constexpr uint32_t IRQ_WAIT_TIMEOUT_MS = 50;
constexpr std::array<uint8_t, 5> SHARED_RF_FILTER_KEY = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
constexpr uint8_t SHARED_CHANNEL = 100;

extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == NRF24_IRQ_PIN)
    {
        osThreadFlagsSet(RadioTaskHandle, NRF24_IRQ_FLAG);
    }
}

extern "C" void RadioTask(void* argument)
{
    (void)argument;
    LOG_INFO("RADIO", "Task started");

    uint32_t device_id = getDeviceId();
    SoftRandom rng(device_id ^ HAL_GetTick());

    RadioCar radio{&hspi1,          //
                   device_id,       //
                   rng.next(),      //
                   SHARED_CHANNEL,  //
                   SHARED_RF_FILTER_KEY};

    if (!radio.init())
    {
        vTaskDelete(nullptr);
    }

    while (true)
    {
        if (!radio.update())
        {
            continue;
        }

        uint32_t flags = osThreadFlagsWait(NRF24_IRQ_FLAG, osFlagsWaitAny, IRQ_WAIT_TIMEOUT_MS);
        if (flags == NRF24_IRQ_FLAG)
        {
            if (!radio.handle_flags())
            {
                continue;
            }
        }

        LOG_ERROR("RADIO", "FAILED: no IRQ within %lums", IRQ_WAIT_TIMEOUT_MS);
        radio.handle_no_flags();
    }
}