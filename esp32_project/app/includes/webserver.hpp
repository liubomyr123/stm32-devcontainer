#pragma once

#include <esp_http_server.h>
#include <esp_log.h>

#include "driver/uart.h"
#include "lwip/sockets.h"

class Webserver
{
   private:
    static constexpr const char* TAG = "web";
    bool initialized = false;
    int ws_client_fd = -1;
    httpd_handle_t server_handle = nullptr;

    static esp_err_t root_handler(httpd_req_t* req);
    static esp_err_t cmd_handler(httpd_req_t* req);
    static esp_err_t stream_stop_handler(httpd_req_t* req);
    static esp_err_t stream_handler(httpd_req_t* req);
    static esp_err_t ws_handler(httpd_req_t* req);

    static esp_err_t wifi_credentials_get_handler(httpd_req_t* req);
    static esp_err_t wifi_credentials_post_handler(httpd_req_t* req);
    static bool read_json_body(httpd_req_t* req, char* buf, size_t buf_len, esp_err_t& error);
    static esp_err_t wifi_ap_handler(httpd_req_t* req);
    static esp_err_t wifi_sta_handler(httpd_req_t* req);
    static esp_err_t wifi_status_handler(httpd_req_t* req);

   public:
    Webserver(/* args */);
    ~Webserver();

    bool start_webserver(esp_err_t& error);
    httpd_handle_t get_server_handle();
    bool send_web_socket(const char* data);
};
