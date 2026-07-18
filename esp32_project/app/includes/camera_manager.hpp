#pragma once

#include <esp_camera.h>
#include <esp_err.h>
#include <esp_log.h>

#include "board_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class CameraManager
{
   private:
    static constexpr const char* TAG = "camera";
    bool initialized = false;

   public:
    CameraManager();
    ~CameraManager();

    bool init(esp_err_t& error);
    bool deinit(esp_err_t& error);
    bool reinit(esp_err_t& error);
};