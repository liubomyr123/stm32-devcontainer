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

/**
   Пульт:
   - Пульт починає в режим TxMode.
   - Із запуску в стані BIND_REQ. Формує пакет [прапорець 0xA1][опкод 1][remote_id 4][R1 4].
   - Надсилає і переходить в стан BIND_RESP та переключається слухати - в RxMode.
   - Якщо машинка НЕ надсилає відповідь за IRQ_WAIT_TIMEOUT_MS (50мс), тоді пульт переключається в
   TxMode і заново будемо надсилати BIND_REQ.
   - Якщо ж машинка відповіла в рамках IRQ_WAIT_TIMEOUT_MS затримки, пульт все ще в RxMode і зчитує
   що машинка надіслала. Пульт все ще в стані BIND_RESP.
   - Зчитуємо FrameType і він має бути Plain. Якщо він не Plain, а Encrypted - пропускаємо і
   далі по колу чекаємо відповідь від машинки. Робимо це 5 раз, якщо на 5 раз не вдалося, тоді
   переходимо на попередній крок і знову шлемо BIND_REQ.
   - Якщо відповідь правильна - Plain, перевряємо чи має опкод типу BIND_RESP. Якщо не той опкод -
   знову по колу слухаємо 5 раз на правильну відповідь. І знову таки якщо не вдалося - переходимо на
   крок BIND_REQ.
   - Далі, якщо відповідь Encrypted та BIND_RESP, перевіряєжмо чи [ехо R1 4] що надіслала машинка і
   те що має пульт співпадає. Якщо не співпадає - зову чекаємо 5 разів і якщо не приходить -
   вертаємося на крок BIND_REQ.
   - Якщо [ехо R1 4] співпало з [R1 4] пульта - переходимо на крок BIND_CONFIRM. Переключаємось
   назад в TxMode. І переходимо в стан BIND_CONFIRM.
   - Формуємо пакет [прапорець 0xA2][nonce 7] [опкод 1] [tag 4] + шифруємо.
   - Надсилаємо і переходимо в стан BIND_ACK та переключається слухати - в RxMode.
   - Якщо машинка НЕ надсилає відповідь за IRQ_WAIT_TIMEOUT_MS (50мс), тоді пульт переключається в
   TxMode і заново будемо надсилати BIND_CONFIRM 5 раз а потім перейде на крок 1 - в BIND_REQ.
   - Якщо ж машинка відповіла в рамках IRQ_WAIT_TIMEOUT_MS затримки, пульт все ще в RxMode і зчитує
   що машинка надіслала. Пульт все ще в стані BIND_ACK.
   - Зчитуємо FrameType і він має бути Encrypted. Якщо він не Encrypted, а Plain - пропускаємо і
   далі по колу чекаємо відповідь від машинки. Робимо це 5 раз, якщо на 5 раз не вдалося, тоді
   переходимо на перший крок і знову шлемо BIND_REQ.
   - Якщо він Encrypted, розшифровуємо. Якщо не вдається, знову 5 раз і в BIND_REQ.
   - Якщо вдається - зчитаємо опкод - BIND_ACK. Якщо інший - чекаємо 5 раз і в BIND_REQ.
   - Якщо правильний опкод - BIND_ACK, все ок. Ми готові обмінюватися даними з машинкою.
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