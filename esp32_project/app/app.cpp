#include "app_context.hpp"

extern "C" void app_main()
{
    auto& ctx = AppContext::get();

    esp_err_t memory_error;
    if (!ctx.memory_manager.init(memory_error))
    {
        return;
    }

    esp_err_t camera_error;
    if (!ctx.camera_manager.init(camera_error))
    {
        return;
    }

    esp_err_t wifi_error;
    if (!ctx.wifi_manager.init(wifi_error))
    {
        return;
    }

    esp_err_t webserver_error;
    if (!ctx.server.start_webserver(webserver_error))
    {
        return;
    }

    esp_err_t uart_manager_error;
    if (!ctx.uart_manager.init(uart_manager_error))
    {
        return;
    }

    esp_err_t task_error;
    if (!ctx.task_manager.init(task_error))
    {
        return;
    }
}
