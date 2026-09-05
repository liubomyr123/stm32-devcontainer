#include <array>

#include "fatfs.h"
#include "include/logger.hpp"

class SdCard
{
   public:
    bool init();
    FRESULT mount();
    bool format();
    bool dirExists(const char* path);
    bool fileExists(const char* filePath);
    bool createDir(const char* path);
    bool isDirEmpty(const char* path);
    bool isFileEmpty(const char* filePath);
    bool countFilesInDir(const char* dirPath, int& count);

    SdCard() = default;

   private:
    static constexpr const char* TAG = "SDCARD";

    FATFS fs_;
};
