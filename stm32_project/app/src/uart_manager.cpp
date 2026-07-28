#include "include/uart_manager.hpp"

extern UART_HandleTypeDef huart3;
static osMutexId_t tx_mutex_ = nullptr;

UartManager& UartManager::instance()
{
    static UartManager instance;
    return instance;
}

extern "C" void uart_manager_init()
{
    tx_mutex_ = osMutexNew(nullptr);
}

void UartManager::send(const uint8_t* data, size_t len)
{
    if (tx_mutex_ != nullptr)
    {
        osMutexAcquire(tx_mutex_, osWaitForever);
    }
    HAL_UART_Transmit(&huart3, data, len, 100);
    if (tx_mutex_ != nullptr)
    {
        osMutexRelease(tx_mutex_);
    }
}
