#include "webserver.hpp"

#include "app_context.hpp"
#include "index.h"

Webserver::Webserver(/* args */)
{
}

Webserver::~Webserver()
{
}

esp_err_t Webserver::root_handler(httpd_req_t* req)
{
    ESP_LOGI(TAG, "Client connected: %s", req->uri);

    int sock = httpd_req_to_sockfd(req);
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    getpeername(sock, (struct sockaddr*)&addr, &addr_len);
    ESP_LOGI(TAG, "Client: %s", inet_ntoa(addr.sin_addr));

    esp_err_t error;

    error = httpd_resp_set_type(req, "text/html");
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_resp_set_type: %s", esp_err_to_name(error));
        return ESP_FAIL;
    }

    error = httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_resp_send: %s", esp_err_to_name(error));
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t Webserver::cmd_handler(httpd_req_t* req)
{
    auto& ctx = AppContext::get();

    esp_err_t error;

    char cmd[32] = {};
    if (httpd_req_get_url_query_str(req, cmd, sizeof(cmd)) == ESP_OK)
    {
        char val[16] = {};
        if (httpd_query_key_value(cmd, "v", val, sizeof(val)) == ESP_OK)
        {
            ESP_LOGI(TAG, "Command: %s", val);
            ctx.uart_manager.send(val);
        }
    }

    error = httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_resp_set_type: %s", esp_err_to_name(error));
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t Webserver::stream_stop_handler(httpd_req_t* req)
{
    esp_err_t error;
    auto& ctx = AppContext::get();

    if (!ctx.stream_active)
    {
        ESP_LOGW(TAG, "Stream already stopped");
    }
    else
    {
        ctx.stream_active = false;
        ctx.stream_socket = -1;

        if (!ctx.camera_manager.deinit(error))
        {
            ESP_LOGE(TAG, "deinit: %s", esp_err_to_name(error));
            return ESP_FAIL;
        }

        ESP_LOGI(TAG, "Stream stopped");
    }

    error = httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_resp_send: %s", esp_err_to_name(error));
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t Webserver::stream_handler(httpd_req_t* req)
{
    esp_err_t error;
    auto& ctx = AppContext::get();

    if (ctx.stream_active)
    {
        ESP_LOGW(TAG, "Stream already active on socket %d", ctx.stream_socket.load());
        ctx.stream_active = false;
        ctx.stream_socket = -1;
    }

    int sock = httpd_req_to_sockfd(req);
    const char* headers =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    send(sock, headers, strlen(headers), 0);

    if (!ctx.camera_manager.reinit(error))
    {
        ESP_LOGE(TAG, "reinit: %s", esp_err_to_name(error));
        return ESP_FAIL;
    }

    ctx.stream_socket = sock;
    ctx.stream_active = true;

    ESP_LOGI(TAG, "Stream started on socket %d", sock);
    return ESP_OK;
}

bool Webserver::start_webserver(esp_err_t& error)
{
    if (initialized)
    {
        ESP_LOGW(TAG, "Already initialized");
        return true;
    }

    auto& ctx = AppContext::get();

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.task_priority = 5;
    config.stack_size = 8192;
    config.max_uri_handlers = 8;
    config.max_open_sockets = 7;

    error = httpd_start(&server_handle, &config);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_start: %s", esp_err_to_name(error));
        return false;
    }

    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = nullptr,
    };

    error = httpd_register_uri_handler(server_handle, &root);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_register_uri_handler/root_handler: %s", esp_err_to_name(error));
        return false;
    }

    httpd_uri_t cmd = {
        .uri = "/cmd",
        .method = HTTP_GET,
        .handler = cmd_handler,
        .user_ctx = nullptr,
    };
    error = httpd_register_uri_handler(server_handle, &cmd);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_register_uri_handler/cmd_handler: %s", esp_err_to_name(error));
        return false;
    }

    httpd_uri_t stream = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = nullptr,
    };
    error = httpd_register_uri_handler(server_handle, &stream);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_register_uri_handler/stream_handler: %s", esp_err_to_name(error));
        return false;
    }

    httpd_uri_t stream_stop = {
        .uri = "/stream/stop",
        .method = HTTP_GET,
        .handler = stream_stop_handler,
        .user_ctx = nullptr,
    };
    error = httpd_register_uri_handler(server_handle, &stream_stop);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_register_uri_handler/stream_stop_handler: %s", esp_err_to_name(error));
        return false;
    }

    initialized = true;
    auto ip = ctx.wifi_manager.get_ap_ip();
    ESP_LOGI(TAG, "Server started at http://" IPSTR, IP2STR(&ip));

    return true;
}

httpd_handle_t Webserver::get_server_handle()
{
    return server_handle;
}
