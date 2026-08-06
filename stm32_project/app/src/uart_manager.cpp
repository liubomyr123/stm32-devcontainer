#include "include/uart_manager.hpp"

#include <cstdio>

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

UartCmd UartManager::parseCmd(const char* buf)
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
    int px = 0;
    int py = 0;

    int parsed = std::sscanf(buf, "F:%d B:%d R:%d L:%d PX:%d PY:%d", &f, &b, &r, &l, &px, &py);
    if (parsed >= 4)
    {
        cmd.f = static_cast<int8_t>(f);
        cmd.b = static_cast<int8_t>(b);
        cmd.r = static_cast<int8_t>(r);
        cmd.l = static_cast<int8_t>(l);

        if (parsed == 6)
        {
            cmd.px = static_cast<int8_t>(px);
            cmd.py = static_cast<int8_t>(py);
        }

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
