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

    httpd_handle_t server_handle = nullptr;

    static esp_err_t root_handler(httpd_req_t* req);
    static esp_err_t cmd_handler(httpd_req_t* req);
    static esp_err_t stream_stop_handler(httpd_req_t* req);
    static esp_err_t stream_handler(httpd_req_t* req);
    static esp_err_t ws_handler(httpd_req_t* req);

   public:
    Webserver(/* args */);
    ~Webserver();

    bool start_webserver(esp_err_t& error);
    httpd_handle_t get_server_handle();
};
