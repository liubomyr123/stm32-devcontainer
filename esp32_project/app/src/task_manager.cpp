#include "task_manager.hpp"

#include "app_context.hpp"

TaskManager::TaskManager(/* args */)
{
}

TaskManager::~TaskManager()
{
}

bool TaskManager::init(esp_err_t& error)
{
    if (initialized)
    {
        ESP_LOGW(TAG, "Already initialized");
        return true;
    }

    if (xTaskCreate(uart_task, "uart", 4096, nullptr, 5, nullptr) != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create uart task");
        error = ESP_ERR_NO_MEM;
        return false;
    }

    if (xTaskCreate(camera_task, "camera", 4096, nullptr, 5, nullptr) != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create camera task");
        error = ESP_ERR_NO_MEM;
        return false;
    }

    initialized = true;
    ESP_LOGI(TAG, "Tasks started");

    return true;
}

void TaskManager::uart_task(void* arg)
{
    auto& ctx = AppContext::get();
    uint8_t buf[UART_BUF_SIZE];
    uart_event_t event;

    while (true)
    {
        if (ctx.uart_manager.queueReceived(event))
        {
            switch (event.type)
            {
                case UART_PATTERN_DET:
                {
                    ctx.uart_manager.read_patern(buf, sizeof(buf));
                    break;
                }
                case UART_DATA:
                {
                    // ctx.uart_manager.read_data(buf, sizeof(buf), event.size);
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
    }
}

void TaskManager::camera_task(void* arg)
{
    while (true)
    {
        auto& ctx = AppContext::get();
        if (!ctx.stream_active || ctx.stream_socket < 0)
        {
            vTaskDelay(pdMS_TO_TICKS(33));
            continue;
        }
        camera_fb_t* fb = esp_camera_fb_get();
        ESP_LOGI(TAG, "Frame captured: %d bytes", fb ? fb->len : 0);
        if (!fb)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        char header[128];
        snprintf(header, sizeof(header),
                 "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n", fb->len);

        int ret = httpd_socket_send(ctx.server.get_server_handle(), ctx.stream_socket, header,
                                    strlen(header), 0);
        if (ret < 0)
        {
            ctx.stream_active = false;
            ctx.stream_socket = -1;
            esp_camera_fb_return(fb);
            continue;
        }

        httpd_socket_send(ctx.server.get_server_handle(), ctx.stream_socket, (char*)fb->buf,
                          fb->len, 0);
        httpd_socket_send(ctx.server.get_server_handle(), ctx.stream_socket, "\r\n", 2, 0);
        esp_camera_fb_return(fb);
        vTaskDelay(pdMS_TO_TICKS(33));
    }
}