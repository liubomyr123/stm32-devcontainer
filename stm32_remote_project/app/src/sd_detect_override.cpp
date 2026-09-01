#include "bsp_driver_sd.h"
#include "fatfs_platform.h"

extern "C" uint8_t BSP_SD_IsDetected(void)
{
    uint8_t status = SD_PRESENT;
    if (HAL_GPIO_ReadPin(SD_DETECT_GPIO_PORT, SD_DETECT_PIN) == GPIO_PIN_RESET)
    {
        status = SD_NOT_PRESENT;
    }
    return status;
}
