#pragma once

#include "cmsis_os.h"
#include "include/uart_config.h"
#include "stm32f4xx_hal.h"

class UartManager
{
   public:
    static UartManager& instance();

    void send(const uint8_t* data, size_t len);
    void sendCmd(UartCmd cmd);
    bool receiveCmd(UartCmd& cmd, uint32_t timeout = osWaitForever);
    UartCmd parseCmd(const char* buf);

   private:
    UartManager() = default;
};