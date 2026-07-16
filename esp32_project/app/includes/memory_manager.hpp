#pragma once

#include <esp_err.h>

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
