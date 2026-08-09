#include "wifi_manager.hpp"

#include "app_context.hpp"

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id,
                               void* event_data)
{
    if (event_base != WIFI_EVENT)
    {
        return;
    }

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
    else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGW("wifi", "STA disconnected / failed to connect");

        char entry[64];
        snprintf(entry, sizeof(entry), "[%lu] STA test: disconnected/failed\n",
                 esp_log_timestamp());
        ctx.memory_manager.log(entry);

        if (ctx.wifi_manager.wifi_event_group_ != nullptr)
        {
            xEventGroupSetBits(ctx.wifi_manager.wifi_event_group_,
                               WifiManager::WIFI_STA_FAILED_BIT);
        }
    }
}

static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id,
                             void* event_data)
{
    if (event_base != IP_EVENT)
    {
        return;
    }

    auto& ctx = AppContext::get();

    if (event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI("wifi", "STA got IP: " IPSTR, IP2STR(&event->ip_info.ip));

        char entry[64];
        snprintf(entry, sizeof(entry), "[%lu] STA test: got IP " IPSTR "\n", esp_log_timestamp(),
                 IP2STR(&event->ip_info.ip));
        ctx.memory_manager.log(entry);

        if (ctx.wifi_manager.wifi_event_group_ != nullptr)
        {
            xEventGroupSetBits(ctx.wifi_manager.wifi_event_group_,
                               WifiManager::WIFI_STA_CONNECTED_BIT);
        }
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

    esp_netif_t* ap_netif = esp_netif_create_default_wifi_ap();
    if (ap_netif == nullptr)
    {
        ESP_LOGE(TAG, "esp_netif_create_default_wifi_ap: failed");
        return false;
    }
    ESP_LOGI(TAG, "AP netif created: %s", esp_netif_get_desc(ap_netif));

    esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
    if (sta_netif == nullptr)
    {
        ESP_LOGE(TAG, "esp_netif_create_default_wifi_sta: failed");
        return false;
    }
    ESP_LOGI(TAG, "STA netif created: %s", esp_netif_get_desc(sta_netif));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    error = esp_wifi_init(&cfg);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_init: %s", esp_err_to_name(error));
        return false;
    }

    esp_event_handler_instance_register(WIFI_EVENT,           //
                                        ESP_EVENT_ANY_ID,     //
                                        &wifi_event_handler,  //
                                        nullptr,              //
                                        nullptr               //
    );

    wifi_event_group_ = xEventGroupCreate();
    if (wifi_event_group_ == nullptr)
    {
        ESP_LOGE(TAG, "Failed to create event group");
        return false;
    }

    esp_event_handler_instance_register(IP_EVENT,             //
                                        IP_EVENT_STA_GOT_IP,  //
                                        &ip_event_handler,    //
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

    esp_ip4_addr_t ap_ip = get_ap_ip();
    ESP_LOGI(TAG, "WiFi AP IP: " IPSTR, IP2STR(&ap_ip));

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

esp_ip4_addr_t WifiManager::get_sta_ip()
{
    esp_netif_ip_info_t ip_info = {};
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif == nullptr)
    {
        ESP_LOGE(TAG, "esp_netif_get_handle_from_ifkey: STA netif not found");
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

bool WifiManager::isStaActive()
{
    wifi_mode_t current_mode;
    if (esp_wifi_get_mode(&current_mode) != ESP_OK)
    {
        return false;
    }
    return current_mode == WIFI_MODE_STA || current_mode == WIFI_MODE_APSTA;
}

bool WifiManager::testStaConnection(const char* ssid, const char* password, esp_err_t& error,
                                    bool& out_connected)
{
    out_connected = false;
    if (!initialized)
    {
        ESP_LOGE(TAG, "WifiManager not initialized");
        return false;
    }

    xEventGroupClearBits(wifi_event_group_, WIFI_STA_CONNECTED_BIT | WIFI_STA_FAILED_BIT);

    error = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_set_mode(APSTA): %s", esp_err_to_name(error));
        return false;
    }

    wifi_config_t sta_config = {};
    strncpy((char*)sta_config.sta.ssid, ssid, sizeof(sta_config.sta.ssid) - 1);
    strncpy((char*)sta_config.sta.password, password, sizeof(sta_config.sta.password) - 1);

    error = esp_wifi_set_config(WIFI_IF_STA, &sta_config);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_set_config(STA): %s", esp_err_to_name(error));
        esp_wifi_set_mode(WIFI_MODE_AP);
        return false;
    }

    error = esp_wifi_connect();
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_connect: %s", esp_err_to_name(error));
        esp_wifi_set_mode(WIFI_MODE_AP);
        return false;
    }

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group_,                             //
                                           WIFI_STA_CONNECTED_BIT | WIFI_STA_FAILED_BIT,  //
                                           pdFALSE,                                       //
                                           pdFALSE,                                       //
                                           pdMS_TO_TICKS(10000));

    if ((bits & (WIFI_STA_CONNECTED_BIT | WIFI_STA_FAILED_BIT)) == 0)
    {
        ESP_LOGW(TAG, "Test connection to '%s': timeout, no event received", ssid);
    }

    out_connected = (bits & WIFI_STA_CONNECTED_BIT) != 0;

    error = esp_wifi_disconnect();
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_disconnect: %s", esp_err_to_name(error));
        esp_wifi_set_mode(WIFI_MODE_AP);
        return false;
    }

    error = esp_wifi_set_mode(WIFI_MODE_AP);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_set_mode(WIFI_MODE_AP): %s", esp_err_to_name(error));
        return false;
    }

    ESP_LOGI(TAG, "Test connection to '%s': %s", ssid, out_connected ? "SUCCESS" : "FAILED");

    return true;
}

bool WifiManager::saveStaCredentials(const char* ssid, const char* password)
{
    if (strlen(ssid) >= SSID_MAX_LEN)
    {
        ESP_LOGE(TAG, "SSID too long: %d chars (max %d)", strlen(ssid), SSID_MAX_LEN - 1);
        return false;
    }

    if (strlen(password) >= PASSWORD_MAX_LEN)
    {
        ESP_LOGE(TAG, "Password too long: %d chars (max %d)", strlen(password),
                 PASSWORD_MAX_LEN - 1);
        return false;
    }

    auto& ctx = AppContext::get();

    if (!ctx.memory_manager.nvsSetStr("sta_ssid", ssid))
    {
        ESP_LOGE(TAG, "Failed to save SSID to NVS");
        return false;
    }

    if (!ctx.memory_manager.nvsSetStr("sta_pass", password))
    {
        ESP_LOGE(TAG, "Failed to save password to NVS");
        return false;
    }

    ESP_LOGI(TAG, "STA credentials saved to NVS - ssid: %s / password: %s", ssid, password);
    return true;
}

bool WifiManager::loadStaCredentials(char* ssid_out, size_t ssid_len, char* password_out,
                                     size_t password_len)
{
    if (ssid_len < SSID_MAX_LEN || password_len < PASSWORD_MAX_LEN)
    {
        ESP_LOGE(TAG, "Buffer too small for STA credentials");
        return false;
    }

    auto& ctx = AppContext::get();

    bool has_ssid = ctx.memory_manager.nvsGetStr("sta_ssid", ssid_out, ssid_len);
    bool has_pass = ctx.memory_manager.nvsGetStr("sta_pass", password_out, password_len);

    if (!has_ssid || !has_pass)
    {
        ESP_LOGI(TAG, "No saved STA credentials found");
        return false;
    }

    return true;
}

bool WifiManager::applySTA(const char* ssid, const char* password, esp_err_t& error,
                           bool& out_connected)
{
    out_connected = false;
    if (!initialized)
    {
        ESP_LOGE(TAG, "WifiManager not initialized");
        return false;
    }

    wifi_mode_t current_mode;
    error = esp_wifi_get_mode(&current_mode);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_get_mode: %s", esp_err_to_name(error));
        return false;
    }

    if (current_mode == WIFI_MODE_STA || current_mode == WIFI_MODE_APSTA)
    {
        wifi_ap_record_t ap_info;
        esp_err_t ap_info_error = esp_wifi_sta_get_ap_info(&ap_info);

        switch (ap_info_error)
        {
            case ESP_OK:
            {
                if (strcmp((const char*)ap_info.ssid, ssid) == 0)
                {
                    ESP_LOGI(TAG, "applySTA: already connected to '%s'", ssid);
                    out_connected = true;
                    error = ESP_OK;
                    return true;
                }

                ESP_LOGI(TAG, "applySTA: connected to different SSID '%s', switching to '%s'",
                         ap_info.ssid, ssid);

                error = esp_wifi_disconnect();
                if (error != ESP_OK)
                {
                    ESP_LOGW(TAG, "esp_wifi_disconnect: %s", esp_err_to_name(error));
                }
                break;
            }
            case ESP_ERR_WIFI_NOT_CONNECT:
            {
                ESP_LOGI(TAG, "applySTA: STA interface initialized but not connected, proceeding");
                break;
            }
            case ESP_ERR_WIFI_CONN:
            {
                ESP_LOGW(TAG, "applySTA: STA interface not initialized, proceeding anyway");
                break;
            }
            default:
            {
                ESP_LOGW(TAG, "esp_wifi_sta_get_ap_info: unexpected error %s",
                         esp_err_to_name(ap_info_error));
                break;
            }
        }
    }

    xEventGroupClearBits(wifi_event_group_, WIFI_STA_CONNECTED_BIT | WIFI_STA_FAILED_BIT);

    error = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_set_mode(APSTA): %s", esp_err_to_name(error));
        return false;
    }

    wifi_config_t sta_config = {};
    strncpy((char*)sta_config.sta.ssid, ssid, sizeof(sta_config.sta.ssid) - 1);
    strncpy((char*)sta_config.sta.password, password, sizeof(sta_config.sta.password) - 1);

    error = esp_wifi_set_config(WIFI_IF_STA, &sta_config);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_set_config(STA): %s", esp_err_to_name(error));
        return false;
    }

    error = esp_wifi_connect();
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_connect: %s", esp_err_to_name(error));
        return false;
    }

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group_,                             //
                                           WIFI_STA_CONNECTED_BIT | WIFI_STA_FAILED_BIT,  //
                                           pdFALSE,                                       //
                                           pdFALSE,                                       //
                                           pdMS_TO_TICKS(10000));

    if ((bits & (WIFI_STA_CONNECTED_BIT | WIFI_STA_FAILED_BIT)) == 0)
    {
        ESP_LOGW(TAG, "applySTA to '%s': timeout, no event received", ssid);
    }

    out_connected = (bits & WIFI_STA_CONNECTED_BIT) != 0;

    if (!out_connected)
    {
        ESP_LOGW(TAG, "applySTA to '%s': FAILED, reverting to AP-only", ssid);
        error = esp_wifi_set_mode(WIFI_MODE_AP);
        if (error != ESP_OK)
        {
            ESP_LOGE(TAG, "esp_wifi_set_mode(AP) revert: %s", esp_err_to_name(error));
            return false;
        }
        return true;
    }

    ESP_LOGI(TAG, "applySTA to '%s': SUCCESS, staying connected (APSTA)", ssid);
    return true;
}

bool WifiManager::applyAP(esp_err_t& error)
{
    if (!initialized)
    {
        ESP_LOGE(TAG, "WifiManager not initialized");
        return false;
    }

    wifi_mode_t current_mode;
    error = esp_wifi_get_mode(&current_mode);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_get_mode: %s", esp_err_to_name(error));
        return false;
    }

    if (current_mode == WIFI_MODE_AP)
    {
        ESP_LOGI(TAG, "applyAP: already in AP-only mode");
        return true;
    }

    if (current_mode == WIFI_MODE_APSTA || current_mode == WIFI_MODE_STA)
    {
        error = esp_wifi_disconnect();
        if (error != ESP_OK)
        {
            ESP_LOGW(TAG, "esp_wifi_disconnect: %s (continuing anyway)", esp_err_to_name(error));
        }
    }

    error = esp_wifi_set_mode(WIFI_MODE_AP);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_set_mode(AP): %s", esp_err_to_name(error));
        return false;
    }

    ESP_LOGI(TAG, "applyAP: switched to AP-only mode");
    return true;
}
