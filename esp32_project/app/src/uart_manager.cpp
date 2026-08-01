#include "uart_manager.hpp"

UartManager::UartManager(/* args */)
{
}

UartManager::~UartManager()
{
}

bool UartManager::init(esp_err_t& error)
{
    if (initialized)
    {
        ESP_LOGW(TAG, "Already initialized");
        return true;
    }

    uart_config_t uart_config = {};
    uart_config.baud_rate = UART_BAUD;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;

    error = uart_param_config(UART_PORT, &uart_config);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_param_config: %s", esp_err_to_name(error));
        return false;
    }
    error = uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN, -1, -1);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_set_pin: %s", esp_err_to_name(error));
        return false;
    }
    error = uart_driver_install(UART_PORT, UART_BUF_SIZE, UART_BUF_SIZE, 10, &uart_queue, 0);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_driver_install: %s", esp_err_to_name(error));
        return false;
    }

    // Pattern detection на '\n'
    error = uart_enable_pattern_det_baud_intr(UART_PORT, '\n', 1, 9, 0, 0);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_enable_pattern_det_baud_intr: %s", esp_err_to_name(error));
        return false;
    }
    error = uart_pattern_queue_reset(UART_PORT, 20);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_pattern_queue_reset: %s", esp_err_to_name(error));
        return false;
    }

    initialized = true;
    ESP_LOGI(TAG, "UART ready");

    return true;
}

bool UartManager::send(const char* cmd)
{
    char msg[32];
    snprintf(msg, sizeof(msg), "%s\n", cmd);
    uart_write_bytes(UART_NUM_2, msg, strlen(msg));
    return true;
}

bool UartManager::read_patern(uint8_t* buf, size_t len)
{
    esp_err_t error;
    int pos = uart_pattern_pop_pos(UART_PORT);
    if (pos == -1)
    {
        ESP_LOGW(TAG, "Pattern position not found, flushing buffer");
        error = uart_flush_input(UART_PORT);
        if (error != ESP_OK)
        {
            ESP_LOGE(TAG, "uart_flush_input: %s", esp_err_to_name(error));
        }
        return false;
    }
    int read_len = uart_read_bytes(UART_PORT,                        //
                                   buf,                              //
                                   std::min((int)len - 1, pos + 1),  //
                                   pdMS_TO_TICKS(10));
    if (read_len == 0)
    {
        ESP_LOGW(TAG, "uart_read_bytes: timeout or empty buffer");
        return false;
    }
    if (read_len < 0)
    {
        ESP_LOGE(TAG, "uart_read_bytes: error %d", read_len);
        return false;
    }

    buf[read_len] = '\0';
    ESP_LOGI(TAG, "UART RX: %s", buf);

    return true;
}

bool UartManager::read_data(uint8_t* buf, size_t len, size_t size)
{
    int read_len = uart_read_bytes(UART_PORT,                //
                                   buf,                      //
                                   std::min(len - 1, size),  //
                                   pdMS_TO_TICKS(10));
    if (read_len == 0)
    {
        ESP_LOGW(TAG, "uart_read_bytes: timeout or empty buffer");
        return false;
    }
    if (read_len < 0)
    {
        ESP_LOGE(TAG, "uart_read_bytes: error %d", read_len);
        return false;
    }

    buf[read_len] = '\0';
    ESP_LOGI(TAG, "Raw RX: %s", buf);
    return true;
}

bool UartManager::queue_received(uart_event_t& event)
{
    return xQueueReceive(uart_queue, &event, portMAX_DELAY);
}

bool UartManager::read_telemetry(TelemetryPacket& pkt)
{
    int read_len = uart_read_bytes(UART_PORT,                //
                                   (uint8_t*)&pkt,           //
                                   sizeof(TelemetryPacket),  //
                                   pdMS_TO_TICKS(10));
    if (read_len == 0)
    {
        ESP_LOGW(TAG, "read_telemetry: timeout or empty buffer");
        return false;
    }
    if (read_len < 0)
    {
        ESP_LOGE(TAG, "read_telemetry: error %d", read_len);
        return false;
    }
    if (read_len != sizeof(TelemetryPacket))
    {
        return false;
    }

    ESP_LOGI(TAG, "Gyro RX: pitch=%.1f roll=%.1f", pkt.pitch, pkt.roll);
    return true;
}
