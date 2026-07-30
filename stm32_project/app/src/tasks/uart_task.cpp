#include <cstdlib>
#include <cstring>

#include "cmsis_os.h"
#include "include/logger.hpp"
#include "include/uart_config.h"

extern osMessageQueueId_t uartRawQueueHandle;
extern osMessageQueueId_t uartCmdQueueHandle;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
extern uint8_t uart3_rx_buf[];
extern DMA_HandleTypeDef hdma_usart3_rx;
extern UART_HandleTypeDef huart3;

extern osMutexId_t uart3_mutex;

static UartCmd parseCmd(const char* buf)
{
    UartCmd cmd{};

    if (buf[0] == 'S')
    {
        cmd.type = CMD_STOP;
        return cmd;
    }

    int f = 0;
    int b = 0;
    int r = 0;
    int l = 0;
    if (sscanf(buf, "F:%d B:%d R:%d L:%d", &f, &b, &r, &l) == 4)
    {
        cmd.f = static_cast<int8_t>(f);
        cmd.b = static_cast<int8_t>(b);
        cmd.r = static_cast<int8_t>(r);
        cmd.l = static_cast<int8_t>(l);

        if (f == 0 && b == 0 && r == 0 && l == 0)
        {
            cmd.type = CMD_STOP;
            return cmd;
        }

        if (f > 0 && l > 0)
        {
            cmd.type = CMD_FORWARD_LEFT;
        }
        else if (f > 0 && r > 0)
        {
            cmd.type = CMD_FORWARD_RIGHT;
        }
        else if (b > 0 && l > 0)
        {
            cmd.type = CMD_BACKWARD_LEFT;
        }
        else if (b > 0 && r > 0)
        {
            cmd.type = CMD_BACKWARD_RIGHT;
        }
        else if (f > 0)
        {
            cmd.type = CMD_FORWARD;
        }
        else if (b > 0)
        {
            cmd.type = CMD_BACKWARD;
        }
        else if (l > 0)
        {
            cmd.type = CMD_LEFT;
        }
        else if (r > 0)
        {
            cmd.type = CMD_RIGHT;
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
        if (osMessageQueueGet(uartRawQueueHandle, &flag, nullptr, osWaitForever) == osOK)
        {
            uint16_t len = UART_RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart3_rx);
            if (len > 0)
            {
                uart3_rx_buf[len] = '\0';
                UartCmd cmd = parseCmd((const char*)uart3_rx_buf);

                if (cmd.type == CMD_UNKNOWN)
                {
                    // skip
                }
                else
                {
                    LOG_INFO("UART", "Received: %s / cmd=%s F=%d, B=%d, L=%d, R=%d", uart3_rx_buf,
                             cmdTypeToString(cmd.type), cmd.f, cmd.b, cmd.l, cmd.r);

                    osMessageQueuePut(uartCmdQueueHandle, &cmd, 0, 0);
                }
            }

            osMutexAcquire(uart3_mutex, osWaitForever);
            HAL_UART_AbortReceive(&huart3);             // Reset DMA before read data
            memset(uart3_rx_buf, 0, UART_RX_BUF_SIZE);  // Clean buffer
            HAL_UART_Receive_DMA(&huart3, uart3_rx_buf, UART_RX_BUF_SIZE);  // Continue DMA
            osMutexRelease(uart3_mutex);
        }
    }
}
