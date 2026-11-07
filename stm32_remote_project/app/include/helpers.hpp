#include <array>

#include "stm32f4xx_hal.h"

uint32_t getDeviceId();

class SoftRandom
{
   public:
    explicit SoftRandom(uint32_t seed) : state_(seed != 0 ? seed : 0x6D2B79F5U)
    {
    }

    uint32_t next();

   private:
    uint32_t state_;
};