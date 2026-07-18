#pragma once

#include <dirent.h>
#include <driver/sdmmc_host.h>
#include <esp_err.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <sys/stat.h>

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

class MemoryManager
{
   private:
    static constexpr const char* TAG = "memory";
    static constexpr const char* MOUNT_POINT = "/sdcard";
    bool initialized = false;
    bool sd_mounted = false;

    char log_path[64] = {};

   public:
    MemoryManager(/* args */);
    ~MemoryManager();

    bool init(esp_err_t& error);
    bool mountSD(esp_err_t& error);
    bool readFile(const char* path, char* buf, size_t len);
    bool readFileChunked(const char* path);
    bool serveFileChunked(const char* path, httpd_req_t* req, esp_err_t& error);
    bool listFiles(const char* path = MOUNT_POINT);
    bool listFilesRecursive(const char* path = MOUNT_POINT);

    bool writeFile(const char* path, const char* data);
    bool appendFile(const char* path, const char* data);
    const char* getMountPoint()
    {
        return MOUNT_POINT;
    }

    bool initLogFile(esp_err_t& error);
    bool log(const char* message);

    bool nvsGetU32(const char* key, uint32_t& value);
    bool nvsSetU32(const char* key, uint32_t value);
    bool nvsGetStr(const char* key, char* value, size_t len);
    bool nvsSetStr(const char* key, const char* value);
};
