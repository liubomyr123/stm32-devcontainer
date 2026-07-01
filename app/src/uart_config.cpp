#include "include/uart_config.h"

const char* cmdTypeToString(UartCmdType type)
{
    switch (type)
    {
        case CMD_FORWARD:
            return "FORWARD";
        case CMD_BACKWARD:
            return "BACKWARD";
        case CMD_LEFT:
            return "LEFT";
        case CMD_RIGHT:
            return "RIGHT";
        case CMD_STOP:
            return "STOP";
        default:
            return "UNKNOWN";
    }
}