#include <cstdio>
#include <cstring>

#include "board_config.h"
#include "driver/gpio.h"
#include "esp_camera.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "index.h"
#include "lwip/sockets.h"
#include "nvs_flash.h"

static const char* TAG = "main";

#define WIFI_SSID "MyCar"
#define WIFI_PASS "12345678"

static httpd_handle_t server_handle = nullptr;
static bool stream_active = false;
static int stream_socket = -1;

static esp_err_t root_handler(httpd_req_t* req);
static void wifi_init_ap();
static httpd_handle_t start_webserver();

static esp_err_t camera_init()
{
    // Вмикаємо живлення камери через GPIO32
    gpio_set_direction(GPIO_NUM_32, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_32, 0);  // LOW = камера увімкнена
    vTaskDelay(pdMS_TO_TICKS(100));  // чекаємо поки стабілізується

    camera_config_t config = {};
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_QVGA;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.jpeg_quality = 15;
    config.fb_count = 1;

    return esp_camera_init(&config);
}

static void uart_task(void* arg)
{
    while (true)
    {
        // TODO: надсилати команди на STM32
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void camera_task(void* arg)
{
    while (true)
    {
        if (!stream_active || stream_socket < 0)
        {
            vTaskDelay(pdMS_TO_TICKS(33));
            continue;
        }
        camera_fb_t* fb = esp_camera_fb_get();
        if (!fb)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        char header[128];
        snprintf(header, sizeof(header),
                 "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n", fb->len);

        int ret = httpd_socket_send(server_handle, stream_socket, header, strlen(header), 0);
        if (ret < 0)
        {
            stream_active = false;
            stream_socket = -1;
            esp_camera_fb_return(fb);
            continue;
        }

        httpd_socket_send(server_handle, stream_socket, (char*)fb->buf, fb->len, 0);
        httpd_socket_send(server_handle, stream_socket, "\r\n", 2, 0);
        esp_camera_fb_return(fb);
        vTaskDelay(pdMS_TO_TICKS(33));
    }
}

static esp_err_t root_handler(httpd_req_t* req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t cmd_handler(httpd_req_t* req)
{
    char cmd[32] = {};

    // Отримуємо параметр ?v=F:80
    if (httpd_req_get_url_query_str(req, cmd, sizeof(cmd)) == ESP_OK)
    {
        char val[16] = {};
        if (httpd_query_key_value(cmd, "v", val, sizeof(val)) == ESP_OK)
        {
            ESP_LOGI(TAG, "Command: %s", val);
            // TODO: надіслати на STM32 через UART
        }
    }

    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t stream_handler(httpd_req_t* req)
{
    int sock = httpd_req_to_sockfd(req);
    const char* headers =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    send(sock, headers, strlen(headers), 0);
    stream_socket = sock;
    stream_active = true;

    // Повертаємось одразу — camera_task надсилає кадри
    return ESP_OK;
}

static esp_err_t stream_stop_handler(httpd_req_t* req)
{
    stream_active = false;
    stream_socket = -1;
    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
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
    config.task_priority = 5;
    config.stack_size = 8192;
    config.max_uri_handlers = 8;
    config.max_open_sockets = 7;

    httpd_start(&server_handle, &config);

    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = nullptr,
    };

    httpd_register_uri_handler(server_handle, &root);

    httpd_uri_t cmd = {
        .uri = "/cmd",
        .method = HTTP_GET,
        .handler = cmd_handler,
        .user_ctx = nullptr,
    };
    httpd_register_uri_handler(server_handle, &cmd);

    httpd_uri_t stream = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = nullptr,
    };
    httpd_register_uri_handler(server_handle, &stream);

    httpd_uri_t stream_stop = {
        .uri = "/stream/stop",
        .method = HTTP_GET,
        .handler = stream_stop_handler,
        .user_ctx = nullptr,
    };
    httpd_register_uri_handler(server_handle, &stream_stop);

    ESP_LOGI(TAG, "Server started at http://192.168.4.1");
    return server_handle;
}

extern "C" void app_main()
{
    esp_err_t ret;
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    ESP_LOGI(TAG, "nvs_flash_init: %s", esp_err_to_name(ret));

    ret = esp_netif_init();
    ESP_LOGI(TAG, "esp_netif_init: %s", esp_err_to_name(ret));

    ret = esp_event_loop_create_default();
    ESP_LOGI(TAG, "esp_event_loop_create_default: %s", esp_err_to_name(ret));

    ret = camera_init();
    ESP_LOGI(TAG, "camera_init: %s", esp_err_to_name(ret));

    wifi_init_ap();
    start_webserver();

    xTaskCreate(uart_task, "uart", 2048, nullptr, 5, nullptr);
    xTaskCreate(camera_task, "camera", 4096, nullptr, 5, nullptr);
}

// extern "C" void app_main()
// {
//     printf("Hello!\n");
//     while (true)
//     {
//         printf("Running...\n");
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }