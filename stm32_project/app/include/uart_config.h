#pragma once

#include <stdint.h>

#define UART_RX_BUF_SIZE 64

typedef enum
{
    CMD_FORWARD,
    CMD_FORWARD_LEFT,
    CMD_FORWARD_RIGHT,
    CMD_BACKWARD,
    CMD_BACKWARD_LEFT,
    CMD_BACKWARD_RIGHT,
    CMD_LEFT,
    CMD_RIGHT,
    CMD_STOP,
    CMD_UNKNOWN
} UartCmdType;

typedef struct
{
    UartCmdType type;
    int8_t f;   // [0; 100]
    int8_t b;   // [0; 100]
    int8_t r;   // [0; 100]
    int8_t l;   // [0; 100]
    int8_t px;  // [-100; 100]
    int8_t py;  // [-100; 100]
} UartCmd;

const char* cmdTypeToString(UartCmdType type);
