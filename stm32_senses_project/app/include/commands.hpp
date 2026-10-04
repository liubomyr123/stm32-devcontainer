#pragma once

#include <cstdint>

enum class CommandOpcode : uint8_t
{
    PingPong = 0x01,
    Handshake
};

#pragma pack(push, 1)
struct PingPongCommand
{
    CommandOpcode opcode = CommandOpcode::PingPong;  // 1
    uint8_t counter = 0;                             // 1
};                                                   // total: 2

struct HandshakeCommand
{
    CommandOpcode opcode = CommandOpcode::Handshake;  // 1
    uint32_t senderId = 0;                            // 4
};                                                    // total: 5

#pragma pack(pop)

static_assert(sizeof(PingPongCommand) == 2, "unexpected padding in PingPongCommand");
static_assert(sizeof(HandshakeCommand) == 5, "unexpected padding in HandshakeCommand");