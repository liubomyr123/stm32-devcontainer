#include "webserver.hpp"

#include "app_context.hpp"
#include "cJSON.h"
#include "html/fallback.h"

Webserver::Webserver(/* args */)
{
}

Webserver::~Webserver()
{
}

esp_err_t Webserver::root_handler(httpd_req_t* req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");

    auto& ctx = AppContext::get();

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

    if (!ctx.memory_manager.serveFileChunked("/sdcard/index.html", req, error))
    {
        error = httpd_resp_send(req, fallback_html, HTTPD_RESP_USE_STRLEN);
        if (error != ESP_OK)
        {
            ESP_LOGE(TAG, "httpd_resp_send: %s", esp_err_to_name(error));
            return ESP_FAIL;
        }
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

            char entry[64];
            snprintf(entry, sizeof(entry), "[%lu] CMD: %s\n", esp_log_timestamp(), val);
            ctx.memory_manager.log(entry);
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
        if (xSemaphoreTake(ctx.camera_mutex, pdMS_TO_TICKS(500)) != pdTRUE)
        {
            ESP_LOGW(TAG, "Timeout waiting for camera mutex");
            httpd_resp_send(req, "Stream busy, try again", HTTPD_RESP_USE_STRLEN);
            return ESP_FAIL;
        }

        ctx.stream_active = false;
        ctx.stream_socket = -1;

        if (!ctx.camera_manager.deinit(error))
        {
            ESP_LOGE(TAG, "deinit: %s", esp_err_to_name(error));
            xSemaphoreGive(ctx.camera_mutex);
            return ESP_FAIL;
        }

        xSemaphoreGive(ctx.camera_mutex);

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
        httpd_resp_send(req, "Stream already active", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    int sock = httpd_req_to_sockfd(req);
    const char* headers =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    send(sock, headers, strlen(headers), 0);

    if (!ctx.camera_manager.init(error))
    {
        ESP_LOGE(TAG, "init: %s", esp_err_to_name(error));
        ctx.stream_active = false;
        ctx.stream_socket = -1;
        close(sock);
        return ESP_FAIL;
    }

    ctx.stream_socket = sock;
    ctx.stream_active = true;

    ESP_LOGI(TAG, "Stream started on socket %d", sock);
    return ESP_OK;
}

esp_err_t Webserver::ws_handler(httpd_req_t* req)
{
    Webserver* self = static_cast<Webserver*>(req->user_ctx);

    if (req->method == HTTP_GET)
    {
        ESP_LOGI(TAG, "WebSocket handshake");
        self->ws_client_fd = httpd_req_to_sockfd(req);
        return ESP_OK;
    }

    auto& ctx = AppContext::get();
    httpd_ws_frame_t ws_pkt = {};
    uint8_t buf[128] = {};
    ws_pkt.payload = buf;
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    esp_err_t error = httpd_ws_recv_frame(req, &ws_pkt, sizeof(buf));
    if (error != ESP_OK)
    {
        ESP_LOGW(TAG, "WebSocket closed or error: %s", esp_err_to_name(error));
        ctx.uart_manager.send("S");
        return error;
    }

    if (ws_pkt.type == HTTPD_WS_TYPE_CLOSE)
    {
        ESP_LOGI(TAG, "WebSocket closed by client");
        ctx.uart_manager.send("S");
        return ESP_OK;
    }

    buf[ws_pkt.len] = '\0';
    cJSON* json = cJSON_Parse((char*)buf);
    if (json)
    {
        cJSON* f = cJSON_GetObjectItem(json, "f");
        cJSON* b = cJSON_GetObjectItem(json, "b");
        cJSON* r = cJSON_GetObjectItem(json, "r");
        cJSON* l = cJSON_GetObjectItem(json, "l");
        cJSON* px = cJSON_GetObjectItem(json, "px");
        cJSON* py = cJSON_GetObjectItem(json, "py");

        if (f && b && r && l && px && py)
        {
            ESP_LOGI(TAG, "F:%d B:%d R:%d L:%d PX:%d PY:%d",  //
                     f->valueint, b->valueint, r->valueint,   //
                     l->valueint, px->valueint, py->valueint);

            char cmd[64];
            snprintf(cmd, sizeof(cmd), "F:%d B:%d R:%d L:%d PX:%d PY:%d",  //
                     f->valueint, b->valueint, r->valueint,                //
                     l->valueint, px->valueint, py->valueint);

            ctx.uart_manager.send(cmd);
            ctx.memory_manager.log(cmd);
        }

        cJSON_Delete(json);
    }

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
        .is_websocket = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol = nullptr,
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
        .is_websocket = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol = nullptr,
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
        .is_websocket = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol = nullptr,
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
        .is_websocket = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol = nullptr,
    };
    error = httpd_register_uri_handler(server_handle, &stream_stop);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_register_uri_handler/stream_stop_handler: %s", esp_err_to_name(error));
        return false;
    }

    httpd_uri_t ws = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = ws_handler,
        .user_ctx = this,
        .is_websocket = true,
        .handle_ws_control_frames = false,
        .supported_subprotocol = nullptr,
    };
    error = httpd_register_uri_handler(server_handle, &ws);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_register_uri_handler/ws_handler: %s", esp_err_to_name(error));
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

bool Webserver::send_web_socket(const char* data)
{
    if (ws_client_fd == -1)
    {
        ESP_LOGW(TAG, "ws_client_fd = -1");
        return false;
    }

    httpd_ws_frame_t ws_pkt = {};
    ws_pkt.payload = (uint8_t*)data;
    ws_pkt.len = strlen(data);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    esp_err_t error = httpd_ws_send_frame_async(server_handle, ws_client_fd, &ws_pkt);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "send_web_socket: %s", esp_err_to_name(error));
        ws_client_fd = -1;
        return false;
    }
    return true;
}
