#pragma once

#include <esp_err.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_netif_ip_addr.h>
#include <esp_wifi.h>
#include <esp_wifi_default.h>

#include <cstring>

#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

class WifiManager
{
   private:
    static constexpr const char* TAG = "wifi";
    bool initialized = false;

   public:
    WifiManager(/* args */);
    ~WifiManager();

    bool init(esp_err_t& error);
    esp_ip4_addr_t get_ap_ip();
    bool testStaConnection(const char* ssid, const char* password, esp_err_t& error,
                           bool& out_connected);

    EventGroupHandle_t wifi_event_group_ = nullptr;

    static constexpr int WIFI_STA_CONNECTED_BIT = BIT0;
    static constexpr int WIFI_STA_FAILED_BIT = BIT1;
};
