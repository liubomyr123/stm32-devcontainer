#pragma once

#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

class MemoryManager
{
   private:
    static constexpr const char* TAG = "memory";
    bool initialized = false;

   public:
    MemoryManager(/* args */);
    ~MemoryManager();

    bool init(esp_err_t& error);
};
