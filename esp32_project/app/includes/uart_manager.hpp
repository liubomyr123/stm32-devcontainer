#pragma once

#include <esp_log.h>

#include <algorithm>
#include <cstring>

#include "driver/uart.h"

#define UART_PORT UART_NUM_2
#define UART_TX_PIN 12
#define UART_RX_PIN 13
#define UART_BAUD 115200
#define UART_BUF_SIZE 256

struct GyroPacket
{
    float pitch;
    float roll;
};

class UartManager
{
   private:
    static constexpr const char* TAG = "uart";
    bool initialized = false;
    QueueHandle_t uart_queue = nullptr;

   public:
    UartManager(/* args */);
    ~UartManager();

    bool init(esp_err_t& error);
    bool send(const char* cmd);
    bool read_patern(uint8_t* buf, size_t len);
    bool read_data(uint8_t* buf, size_t len, size_t size);
    bool queue_received(uart_event_t& event);
    bool read_gyro(GyroPacket& pkt);
};
