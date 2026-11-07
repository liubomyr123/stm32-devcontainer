#include "helpers.hpp"

uint32_t getDeviceId()
{
    const std::array<uint32_t, 3> words = {HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2()};

    uint32_t hash = 2166136261U;  // FNV offset basis
    for (uint32_t word : words)
    {
        for (uint32_t i = 0; i < 4; ++i)
        {
            hash ^= (word >> (8 * i)) & 0xFFU;
            hash *= 16777619U;  // FNV prime
        }
    }
    return hash;
}

uint32_t SoftRandom::next()
{
    state_ ^= state_ << 13;
    state_ ^= state_ >> 17;
    state_ ^= state_ << 5;
    return state_;
}