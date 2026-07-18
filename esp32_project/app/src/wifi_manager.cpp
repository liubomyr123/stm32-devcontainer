#include "wifi_manager.hpp"

#include "app_context.hpp"

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id,
                               void* event_data)
{
    auto& ctx = AppContext::get();

    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*)event_data;
        ESP_LOGI("wifi", "Client connected, MAC: %02x:%02x:%02x:%02x:%02x:%02x", event->mac[0],
                 event->mac[1], event->mac[2], event->mac[3], event->mac[4], event->mac[5]);

        char entry[64];
        snprintf(entry, sizeof(entry), "[%lu] Client connected: %02x:%02x:%02x:%02x:%02x:%02x\n",
                 esp_log_timestamp(), event->mac[0], event->mac[1], event->mac[2], event->mac[3],
                 event->mac[4], event->mac[5]);
        ctx.memory_manager.log(entry);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*)event_data;
        ESP_LOGI("wifi", "Client disconnected, MAC: %02x:%02x:%02x:%02x:%02x:%02x", event->mac[0],
                 event->mac[1], event->mac[2], event->mac[3], event->mac[4], event->mac[5]);

        char entry[64];
        snprintf(entry, sizeof(entry), "[%lu] Client disconnected: %02x:%02x:%02x:%02x:%02x:%02x\n",
                 esp_log_timestamp(), event->mac[0], event->mac[1], event->mac[2], event->mac[3],
                 event->mac[4], event->mac[5]);
        ctx.memory_manager.log(entry);
    }
}

WifiManager::WifiManager(/* args */)
{
}

WifiManager::~WifiManager()
{
}

bool WifiManager::init(esp_err_t& error)
{
    if (initialized)
    {
        ESP_LOGW(TAG, "Already initialized");
        return true;
    }

    auto& ctx = AppContext::get();

    error = esp_netif_init();
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_netif_init: %s", esp_err_to_name(error));
        return false;
    }

    error = esp_event_loop_create_default();
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_event_loop_create_default: %s", esp_err_to_name(error));
        return false;
    }

    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT,           //
                                        ESP_EVENT_ANY_ID,     //
                                        &wifi_event_handler,  //
                                        nullptr,              //
                                        nullptr               //
    );
    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.ap.ssid, ctx.WIFI_SSID, sizeof(wifi_config.ap.ssid));
    wifi_config.ap.ssid_len = strlen(ctx.WIFI_SSID);
    strncpy((char*)wifi_config.ap.password, ctx.WIFI_PASS, sizeof(wifi_config.ap.password));
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;

    error = esp_wifi_set_mode(WIFI_MODE_AP);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_set_mode: %s", esp_err_to_name(error));
        return false;
    }

    error = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_set_config: %s", esp_err_to_name(error));
        return false;
    }

    error = esp_wifi_start();
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_start: %s", esp_err_to_name(error));
        return false;
    }

    initialized = true;
    ESP_LOGI(TAG, "WiFi AP started: %s", ctx.WIFI_SSID);

    return true;
}

esp_ip4_addr_t WifiManager::get_ap_ip()
{
    esp_netif_ip_info_t ip_info = {};
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (netif == nullptr)
    {
        ESP_LOGE(TAG, "esp_netif_get_handle_from_ifkey: netif not found");
        return ip_info.ip;
    }
    esp_err_t error = esp_netif_get_ip_info(netif, &ip_info);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_netif_get_ip_info: %s", esp_err_to_name(error));
        return ip_info.ip;
    }
    return ip_info.ip;
}