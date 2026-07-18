#include "camera_manager.hpp"

#include "app_context.hpp"

CameraManager::CameraManager()
{
}

CameraManager::~CameraManager()
{
}

bool CameraManager::init(esp_err_t& error)
{
    if (initialized)
    {
        ESP_LOGW(TAG, "Already initialized");
        return true;
    }

    auto& ctx = AppContext::get();

    // // Вмикаємо живлення камери через GPIO32
    // gpio_set_direction(GPIO_NUM_32, GPIO_MODE_OUTPUT);
    // gpio_set_level(GPIO_NUM_32, 0);  // LOW = камера увімкнена
    // vTaskDelay(pdMS_TO_TICKS(100));  // чекаємо поки стабілізується

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

    error = esp_camera_init(&config);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_camera_init: %s", esp_err_to_name(error));
        return false;
    }

    initialized = true;
    ESP_LOGI(TAG, "Camera initialized OK");

    ctx.memory_manager.log("Stream started\n");

    return true;
}

bool CameraManager::deinit(esp_err_t& error)
{
    if (!initialized)
    {
        ESP_LOGW(TAG, "Already deinitialized");
        return true;
    }

    auto& ctx = AppContext::get();

    error = esp_camera_deinit();
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_camera_deinit: %s", esp_err_to_name(error));
        return false;
    }

    initialized = false;
    ESP_LOGI(TAG, "Camera deinitialized");

    ctx.memory_manager.log("Stream stopped\n");

    return true;
}

bool CameraManager::reinit(esp_err_t& error)
{
    if (initialized)
    {
        deinit(error);
    }

    initialized = false;
    vTaskDelay(pdMS_TO_TICKS(200));
    return init(error);
}