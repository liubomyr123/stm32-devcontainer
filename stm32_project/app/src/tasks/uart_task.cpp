#include <cstring>

#include "cmsis_os.h"
#include "include/logger.hpp"
#include "include/uart_config.h"
#include "uart_manager.hpp"

extern osMessageQueueId_t uartRawQueueHandle;
extern osMessageQueueId_t uartCmdQueueHandle;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
extern uint8_t uart3_rx_buf[];
extern DMA_HandleTypeDef hdma_usart3_rx;
extern UART_HandleTypeDef huart3;

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
                UartCmd cmd = UartManager::instance().parseCmd((const char*)uart3_rx_buf);

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

            HAL_UART_AbortReceive(&huart3);             // Reset DMA before read data
            memset(uart3_rx_buf, 0, UART_RX_BUF_SIZE);  // Clean buffer
            HAL_UART_Receive_DMA(&huart3, uart3_rx_buf, UART_RX_BUF_SIZE);  // Continue DMA
        }
    }
}
