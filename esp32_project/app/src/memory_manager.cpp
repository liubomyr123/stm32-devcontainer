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

bool MemoryManager::mountSD(esp_err_t& error)
{
    if (sd_mounted)
    {
        ESP_LOGW(TAG, "SD already mounted");
        return true;
    }

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;  // 1-bit режим

    esp_vfs_fat_sdmmc_mount_config_t mount_config{};
    mount_config.format_if_mount_failed = false;
    mount_config.max_files = 5;
    mount_config.allocation_unit_size = 16 * 1024;

    sdmmc_card_t* card;
    error = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (error != ESP_OK)
    {
        switch (error)
        {
            case ESP_ERR_NOT_FOUND:
                ESP_LOGW(TAG, "SD card not inserted");
                break;
            case ESP_ERR_TIMEOUT:
                ESP_LOGW(TAG, "SD card timeout - check connection");
                break;
            case ESP_ERR_NO_MEM:
                ESP_LOGE(TAG, "Not enough memory to mount SD");
                break;
            default:
                ESP_LOGE(TAG, "SD mount failed: %s", esp_err_to_name(error));
                break;
        }
        return false;
    }

    sd_mounted = true;
    ESP_LOGI(TAG, "SD card mounted at %s", MOUNT_POINT);
    return true;
}

bool MemoryManager::readFile(const char* path, char* buf, size_t len)
{
    FILE* f = fopen(path, "r");
    if (f == nullptr)
    {
        ESP_LOGE(TAG, "Failed to open file: %s", path);
        return false;
    }

    size_t read_len = fread(buf, 1, len - 1, f);
    fclose(f);
    f = nullptr;

    if (read_len == 0)
    {
        ESP_LOGW(TAG, "File is empty: %s", path);
        return false;
    }

    buf[read_len] = '\0';
    ESP_LOGI(TAG, "Read %d bytes from %s", read_len, path);
    return true;
}

bool MemoryManager::readFileChunked(const char* path)
{
    FILE* f = fopen(path, "r");
    if (f == nullptr)
    {
        ESP_LOGW(TAG, "File not found: %s", path);
        return false;
    }

    char chunk[512];
    size_t read_len;
    while ((read_len = fread(chunk, 1, sizeof(chunk), f)) > 0)
    {
        chunk[read_len] = '\0';
        ESP_LOGI(TAG, "%s", chunk);
    }
    fclose(f);
    return true;
}

bool MemoryManager::serveFileChunked(const char* path, httpd_req_t* req, esp_err_t& error)
{
    FILE* f = fopen(path, "r");
    if (f == nullptr)
    {
        ESP_LOGW(TAG, "File not found: %s", path);
        return false;
    }

    char chunk[512];
    size_t read_len;
    while ((read_len = fread(chunk, 1, sizeof(chunk), f)) > 0)
    {
        error = httpd_resp_send_chunk(req, chunk, read_len);
        if (error != ESP_OK)
        {
            ESP_LOGE(TAG, "httpd_resp_send_chunk: %s", esp_err_to_name(error));
        }
    }
    error = httpd_resp_send_chunk(req, nullptr, 0);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_resp_send_chunk: %s", esp_err_to_name(error));
        fclose(f);
        return false;
    }
    fclose(f);

    ESP_LOGI(TAG, "Served: %s", path);
    return true;
}

bool MemoryManager::listFiles(const char* path)
{
    DIR* dir = opendir(path);
    if (dir == nullptr)
    {
        ESP_LOGE(TAG, "Failed to open directory: %s", path);
        return false;
    }

    ESP_LOGI(TAG, "Files in %s:", path);
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        ESP_LOGI(TAG, "  - %s", entry->d_name);
    }
    closedir(dir);
    return true;
}

bool MemoryManager::listFilesRecursive(const char* path)
{
    DIR* dir = opendir(path);
    if (dir == nullptr)
    {
        ESP_LOGE(TAG, "Failed to open directory: %s", path);
        return false;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (entry->d_type == DT_DIR)
        {
            ESP_LOGI(TAG, "[DIR] %s", full_path);
            listFilesRecursive(full_path);
        }
        else
        {
            ESP_LOGI(TAG, "  - %s", full_path);
        }
    }
    closedir(dir);
    return true;
}

bool MemoryManager::writeFile(const char* path, const char* data)
{
    FILE* f = fopen(path, "w");
    if (f == nullptr)
    {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", path);
        return false;
    }

    fprintf(f, "%s", data);
    fclose(f);

    ESP_LOGI(TAG, "Written to: %s", path);
    return true;
}

bool MemoryManager::appendFile(const char* path, const char* data)
{
    FILE* f = fopen(path, "a");
    if (f == nullptr)
    {
        ESP_LOGE(TAG, "Failed to open file for appending: %s", path);
        return false;
    }

    fprintf(f, "%s", data);
    fclose(f);

    ESP_LOGI(TAG, "Appended to: %s", path);
    return true;
}

bool MemoryManager::initLogFile(esp_err_t& error)
{
    mkdir("/sdcard/logs", 0777);

    // Читаємо лічильник сесій з NVS
    uint32_t session = 0;
    nvsGetU32("session", session);
    session++;
    nvsSetU32("session", session);

    snprintf(log_path, sizeof(log_path), "/sdcard/logs/session_%lu.log", session);

    FILE* f = fopen(log_path, "w");
    if (f == nullptr)
    {
        ESP_LOGE(TAG, "Failed to create log file: %s", log_path);
        return false;
    }
    fclose(f);

    ESP_LOGI(TAG, "Log file: %s", log_path);
    return true;
}

bool MemoryManager::log(const char* message)
{
    return appendFile(log_path, message);
}

bool MemoryManager::nvsGetU32(const char* key, uint32_t& value)
{
    nvs_handle_t nvs;
    esp_err_t error = nvs_open("storage", NVS_READONLY, &nvs);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_open: %s", esp_err_to_name(error));
        return false;
    }

    error = nvs_get_u32(nvs, key, &value);
    nvs_close(nvs);

    if (error != ESP_OK)
    {
        ESP_LOGW(TAG, "nvs_get_u32 [%s]: %s", key, esp_err_to_name(error));
        return false;
    }
    return true;
}

bool MemoryManager::nvsSetU32(const char* key, uint32_t value)
{
    nvs_handle_t nvs;
    esp_err_t error = nvs_open("storage", NVS_READWRITE, &nvs);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_open: %s", esp_err_to_name(error));
        return false;
    }

    nvs_set_u32(nvs, key, value);
    nvs_commit(nvs);
    nvs_close(nvs);
    return true;
}

bool MemoryManager::nvsGetStr(const char* key, char* value, size_t len)
{
    nvs_handle_t nvs;
    esp_err_t error = nvs_open("storage", NVS_READONLY, &nvs);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_open: %s", esp_err_to_name(error));
        return false;
    }

    error = nvs_get_str(nvs, key, value, &len);
    nvs_close(nvs);

    if (error != ESP_OK)
    {
        ESP_LOGW(TAG, "nvs_get_str [%s]: %s", key, esp_err_to_name(error));
        return false;
    }
    return true;
}

bool MemoryManager::nvsSetStr(const char* key, const char* value)
{
    nvs_handle_t nvs;
    esp_err_t error = nvs_open("storage", NVS_READWRITE, &nvs);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_open: %s", esp_err_to_name(error));
        return false;
    }

    nvs_set_str(nvs, key, value);
    nvs_commit(nvs);
    nvs_close(nvs);
    return true;
}