#include <cstdio>
#include <cstring>

#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "index.h"
#include "nvs_flash.h"

static const char* TAG = "main";

#define WIFI_SSID "MyCar"
#define WIFI_PASS "12345678"

static esp_err_t root_handler(httpd_req_t* req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static void wifi_init_ap()
{
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.ap.ssid, WIFI_SSID, sizeof(wifi_config.ap.ssid));
    wifi_config.ap.ssid_len = strlen(WIFI_SSID);
    strncpy((char*)wifi_config.ap.password, WIFI_PASS, sizeof(wifi_config.ap.password));
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "WiFi AP started: %s", WIFI_SSID);
}

static httpd_handle_t start_webserver()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    httpd_start(&server, &config);

    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = nullptr,
    };

    httpd_register_uri_handler(server, &root);

    ESP_LOGI(TAG, "Server started at http://192.168.4.1");
    return server;
}

extern "C" void app_main()
{
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();

    wifi_init_ap();
    start_webserver();
}