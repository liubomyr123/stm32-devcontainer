#include "app_context.hpp"

extern "C" void app_main()
{
    auto& ctx = AppContext::get();

    esp_err_t memory_error;
    if (!ctx.memory_manager.init(memory_error))
    {
        return;
    }

    esp_err_t sd_error;
    if (!ctx.memory_manager.mountSD(sd_error))
    {
        return;
    }

    if (!ctx.memory_manager.initLogFile(sd_error))
    {
        return;
    }

    if (!ctx.memory_manager.listFilesRecursive())
    {
        return;
    }

    esp_err_t wifi_error;
    if (!ctx.wifi_manager.init(wifi_error))
    {
        return;
    }

    esp_err_t test_error;
    bool test_connected = false;
    ctx.wifi_manager.testStaConnection("Iphone vovk_", "vovk_123", test_error, test_connected);
    ESP_LOGI("TEST", "STA test result: %s", test_connected ? "CONNECTED" : "FAILED");

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
