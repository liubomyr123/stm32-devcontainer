#pragma once

#include <stdint.h>

#define UART_RX_BUF_SIZE 64

typedef enum
{
    CMD_FORWARD,
    CMD_BACKWARD,
    CMD_LEFT,
    CMD_RIGHT,
    CMD_STOP,
    CMD_UNKNOWN
} UartCmdType;

typedef struct
{
    UartCmdType type;
    uint8_t value;  // 0-100
} UartCmd;

const char* cmdTypeToString(UartCmdType type);
