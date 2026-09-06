#include <array>
#include <cinttypes>
#include <cstring>
#include <string>

#include "fatfs.h"
#include "include/logger.hpp"
#include "include/mutex_guard.hpp"

class SdCard
{
   public:
    bool init();
    bool updateCurrentLog(const char* message);

    SdCard() = default;

   private:
    static constexpr const char* TAG = "SDCARD";

    osMutexId_t log_mutex_ = nullptr;

    FRESULT mount();
    bool format();
    bool dirExists(const char* path);
    bool fileExists(const char* filePath);
    bool createDir(const char* path);
    bool isDirEmpty(const char* path);
    bool isFileEmpty(const char* filePath);
    bool countFilesInDir(const char* dirPath, int& count);
    bool initLogFile();
    bool generateSessionTimestamp();
    bool getFullFilePath(std::array<char, 64>& path) const;
    static const char* fresultToString(FRESULT result);

    FATFS fs_{};
    std::string log_path = {0};
    uint32_t current_session = 0;
};
