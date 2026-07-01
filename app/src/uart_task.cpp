#include <cstdlib>
#include <cstring>

#include "cmsis_os.h"
#include "include/logger.hpp"
#include "include/uart_config.h"

extern osMessageQueueId_t uartRawQueueHandle;
extern osMessageQueueId_t uartCmdQueueHandle;
extern uint8_t uart3_rx_buf[];
extern DMA_HandleTypeDef hdma_usart3_rx;
extern UART_HandleTypeDef huart3;

static UartCmd parseCmd(const char* buf)
{
    UartCmd cmd = {CMD_UNKNOWN, 0};

    if (buf[0] == 'S')
    {
        cmd.type = CMD_STOP;
    }
    else if (buf[1] == ':')
    {
        cmd.value = (uint8_t)atoi(&buf[2]);
        switch (buf[0])
        {
            case 'F':
                cmd.type = CMD_FORWARD;
                break;
            case 'B':
                cmd.type = CMD_BACKWARD;
                break;
            case 'L':
                cmd.type = CMD_LEFT;
                break;
            case 'R':
                cmd.type = CMD_RIGHT;
                break;
            default:
                cmd.type = CMD_UNKNOWN;
                break;
        }
    }

    return cmd;
}

extern "C" void UartTask(void* argument)
{
    (void)argument;
    uint16_t flag;

    while (true)
    {
        if (osMessageQueueGet(uartRawQueueHandle, &flag, NULL, osWaitForever) == osOK)
        {
            uint16_t len = UART_RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart3_rx);
            if (len > 0)
            {
                uart3_rx_buf[len] = '\0';
                UartCmd cmd = parseCmd((const char*)uart3_rx_buf);
                LOG_INFO("UART", "Received: %s / cmd=%s value=%d", uart3_rx_buf,
                         cmdTypeToString(cmd.type), cmd.value);

                osMessageQueuePut(uartCmdQueueHandle, &cmd, 0, 0);
            }

            HAL_UART_AbortReceive(&huart3);             // Reset DMA before read data
            memset(uart3_rx_buf, 0, UART_RX_BUF_SIZE);  // Clean buffer
            HAL_UART_Receive_DMA(&huart3, uart3_rx_buf, UART_RX_BUF_SIZE);  // Continue DMA
        }
    }
}
