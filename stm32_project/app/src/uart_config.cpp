#include "include/uart_config.h"

const char* cmdTypeToString(UartCmdType type)
{
    switch (type)
    {
        case CMD_FORWARD:
            return "FORWARD";
        case CMD_FORWARD_LEFT:
            return "FORWARD_LEFT";
        case CMD_FORWARD_RIGHT:
            return "FORWARD_RIGHT";
        case CMD_BACKWARD:
            return "BACKWARD";
        case CMD_BACKWARD_LEFT:
            return "BACKWARD_LEFT";
        case CMD_BACKWARD_RIGHT:
            return "BACKWARD_RIGHT";
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