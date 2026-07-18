#include "memory_manager.hpp"

MemoryManager::MemoryManager(/* args */)
{
}

MemoryManager::~MemoryManager()
{
}

bool MemoryManager::init(esp_err_t& error)
{
    if (initialized)
    {
        ESP_LOGW(TAG, "Already initialized");
        return true;
    }

    error = nvs_flash_init();
    if (error == ESP_ERR_NVS_NO_FREE_PAGES || error == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        error = nvs_flash_init();
    }
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_flash_init: %s", esp_err_to_name(error));
        return false;
    }

    initialized = true;
    ESP_LOGI(TAG, "Initialized OK");

    return true;
}
