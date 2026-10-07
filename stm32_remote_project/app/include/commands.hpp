#pragma once

#include <cstdint>

/*
 * 1) BIND_REQ      пульт -> машинка    відкритий
 *      [прапорець 0xA1][опкод 1][remote_id 4][R1 4]                        = 9 байт
 *
 * 2) BIND_RESP     машинка -> пульт    відкритий
 *      [прапорець 0xA1][опкод 1][car_id 4][R2 4][ехо R1 4]                 = 13 байт
 *
 * 3) BIND_CONFIRM  пульт -> машинка    зашифрований ключем key_remote_to_car
 *      [прапорець 0xA2][nonce 7] [опкод 1] [tag 4]                         = 7 + 1 + 4 = 12 байт
 *
 * 4) BIND_ACK      машинка -> пульт    зашифрований ключем key_car_to_remote
 *      [прапорець 0xA2][nonce 7] [опкод 1] [tag 4]                         = 7 + 1 + 4 = 12 байт
 *
 * 5) PING_PONG     в обидва боки       зашифрований ключем свого напрямку
 *      [прапорець 0xA2][nonce 7] [опкод 1][counter 1] [tag 4]              = 7 + 2 + 4 = 13 байт
 */

enum class FrameType : uint8_t
{
    Plain = 0xA1,
    Encrypted = 0xA2,
};

enum class CommandOpcode : uint8_t
{
    PingPong = 0x01,
    BindReq,      // remote to car
    BindResp,     // car to remote
    BindConfirm,  // remote to car
    BindAck,      // car to remote
};

#pragma pack(push, 1)
struct PingPongCommand
{
    CommandOpcode opcode = CommandOpcode::PingPong;  // 1
    uint8_t counter = 0;                             // 1
};                                                   // total: 2

struct BindReqCommand
{
    CommandOpcode opcode = CommandOpcode::BindReq;  // 1
    uint32_t remoteId = 0;                          // 4
    uint32_t remoteRandom = 0;                      // 4
};                                                  // total: 9

struct BindRespCommand
{
    CommandOpcode opcode = CommandOpcode::BindResp;  // 1
    uint32_t carId = 0;                              // 4
    uint32_t carRandom = 0;                          // 4
    uint32_t remoteRandomEcho = 0;                   // 4
};                                                   // total: 13

struct BindConfirmCommand
{
    CommandOpcode opcode = CommandOpcode::BindConfirm;  // 1
};                                                      // total: 1

struct BindAckCommand
{
    CommandOpcode opcode = CommandOpcode::BindAck;  // 1
};                                                  // total: 1

#pragma pack(pop)

static_assert(sizeof(PingPongCommand) == 2, "unexpected padding in PingPongCommand");
static_assert(sizeof(BindReqCommand) == 9, "unexpected padding in BindReqCommand");
static_assert(sizeof(BindRespCommand) == 13, "unexpected padding in BindRespCommand");
static_assert(sizeof(BindConfirmCommand) == 1, "unexpected padding in BindConfirmCommand");
static_assert(sizeof(BindAckCommand) == 1, "unexpected padding in BindAckCommand");