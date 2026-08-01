#include <cstdint>

typedef struct
{
    float pitch{};
    float roll{};
    int8_t fl{};  // Front Left speed (-100..+100, від'ємне = назад)
    int8_t fr{};  // Front Right
    int8_t rl{};  // Rear Left
    int8_t rr{};  // Rear Right
} TelemetryPacket;