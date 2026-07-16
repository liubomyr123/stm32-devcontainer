#pragma once

#include <esp_err.h>
#include <esp_netif_ip_addr.h>

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
};
