#pragma once

#include <esp_camera.h>
#include <esp_err.h>
#include <esp_log.h>

class TaskManager
{
   private:
    static constexpr const char* TAG = "tasks";
    bool initialized = false;

    static void uart_task(void* arg);
    static void camera_task(void* arg);

   public:
    TaskManager(/* args */);
    ~TaskManager();

    bool init(esp_err_t& error);
};
