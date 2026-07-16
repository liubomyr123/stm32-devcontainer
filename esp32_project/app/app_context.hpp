#pragma once

#include <atomic>

#include "camera_manager.hpp"
#include "memory_manager.hpp"
#include "task_manager.hpp"
#include "webserver.hpp"
#include "wifi_manager.hpp"

class AppContext
{
   private:
    AppContext() = default;

   public:
    static AppContext& get()
    {
        static AppContext instance;
        return instance;
    }

    AppContext(const AppContext&) = delete;
    AppContext& operator=(const AppContext&) = delete;

    MemoryManager memory_manager;
    WifiManager wifi_manager;
    CameraManager camera_manager;
    Webserver server;
    TaskManager task_manager;

    static constexpr const char* WIFI_SSID = "MyCar";
    static constexpr const char* WIFI_PASS = "12345678";
    std::atomic<bool> stream_active{false};
    std::atomic<int> stream_socket{-1};
};
