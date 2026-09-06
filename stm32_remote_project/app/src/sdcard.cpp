#include "sdcard.hpp"

extern RNG_HandleTypeDef hrng;

bool SdCard::init()
{
    log_mutex_ = osMutexNew(nullptr);

    FRESULT mountResult = mount();
    if (mountResult == FR_NO_FILESYSTEM)
    {
        LOG_WARNING(TAG, "No valid filesystem found, formatting...");
        if (!format())
        {
            return false;
        }
        mountResult = mount();
    }

    if (mountResult != FR_OK)
    {
        return false;
    }

    if (!dirExists("/logs"))
    {
        if (!createDir("/logs"))
        {
            return false;
        }
    }
    if (!isDirEmpty("/logs"))
    {
        int numberOfFiles = 0;
        if (!countFilesInDir("/logs", numberOfFiles))
        {
            return false;
        }
    }
    if (!initLogFile())
    {
        return false;
    }

    return true;
}

FRESULT SdCard::mount()
{
    FRESULT mountResult = f_mount(&fs_, SDPath, 1);
    if (mountResult != FR_OK)
    {
        LOG_ERROR(TAG, "f_mount failed: %s", fresultToString(mountResult));
        return mountResult;
    }

    LOG_INFO(TAG, "Mounted OK");
    return mountResult;
}

bool SdCard::format()
{
    std::array<BYTE, _MAX_SS> work = {0};

    FRESULT mkfsResult = f_mkfs(SDPath, FM_FAT32, 0, work.data(), work.size());
    if (mkfsResult != FR_OK)
    {
        LOG_ERROR(TAG, "f_mkfs failed: %s", fresultToString(mkfsResult));
        return false;
    }

    LOG_INFO(TAG, "Formatted OK");
    return true;
}

bool SdCard::dirExists(const char* dirPath)
{
    FILINFO fileInfo;
    FRESULT statResult = f_stat(dirPath, &fileInfo);

    if (statResult == FR_NO_PATH)
    {
        LOG_WARNING(TAG, "'%s' — parent directory does not exist", dirPath);
        return false;
    }

    if (statResult == FR_NO_FILE)
    {
        LOG_INFO(TAG, "'%s' does not exist", dirPath);
        return false;
    }

    if (statResult != FR_OK)
    {
        LOG_ERROR(TAG, "f_stat failed for '%s': %s", dirPath, fresultToString(statResult));
        return false;
    }

    bool isDir = (fileInfo.fattrib & AM_DIR) != 0;
    if (isDir)
    {
        LOG_INFO(TAG, "'%s' exists and is a directory", dirPath);
    }
    else
    {
        LOG_WARNING(TAG, "'%s' exists but is NOT a directory", dirPath);
    }

    return isDir;
}

bool SdCard::fileExists(const char* filePath)
{
    FILINFO fileInfo;
    FRESULT statResult = f_stat(filePath, &fileInfo);

    if (statResult == FR_NO_PATH)
    {
        LOG_WARNING(TAG, "'%s' — parent directory does not exist", filePath);
        return false;
    }

    if (statResult == FR_NO_FILE)
    {
        LOG_INFO(TAG, "'%s' — file does not exist", filePath);
        return false;
    }

    if (statResult != FR_OK)
    {
        LOG_ERROR(TAG, "f_stat failed for '%s': %s", filePath, fresultToString(statResult));
        return false;
    }

    bool isFile = (fileInfo.fattrib & AM_DIR) == 0;
    if (isFile)
    {
        LOG_INFO(TAG, "'%s' exists and is a file", filePath);
    }
    else
    {
        LOG_WARNING(TAG, "'%s' exists but is a directory, not a file", filePath);
    }

    return isFile;
}

bool SdCard::createDir(const char* dirPath)
{
    FRESULT result = f_mkdir(dirPath);

    if (result == FR_EXIST)
    {
        LOG_WARNING(TAG, "Directory '%s' already exists", dirPath);
        return true;
    }

    if (result != FR_OK)
    {
        LOG_ERROR(TAG, "f_mkdir failed for '%s': %s", dirPath, fresultToString(result));
        return false;
    }

    return true;
}

bool SdCard::isDirEmpty(const char* dirPath)
{
    DIR dir;
    FRESULT openResult = f_opendir(&dir, dirPath);

    if (openResult == FR_NO_PATH)
    {
        LOG_WARNING(TAG, "'%s' — parent directory does not exist", dirPath);
        return false;
    }

    if (openResult == FR_NO_FILE)
    {
        LOG_INFO(TAG, "'%s' does not exist", dirPath);
        return false;
    }

    if (openResult != FR_OK)
    {
        LOG_ERROR(TAG, "f_opendir failed for '%s': %s", dirPath, fresultToString(openResult));
        return false;
    }

    FILINFO fileInfo;
    FRESULT readResult = f_readdir(&dir, &fileInfo);
    f_closedir(&dir);

    if (readResult != FR_OK)
    {
        LOG_ERROR(TAG, "f_readdir failed for '%s': %s", dirPath, fresultToString(readResult));
        return false;
    }

    bool isEmpty = (fileInfo.fname[0] == 0);
    LOG_INFO(TAG, "'%s' is %s", dirPath, isEmpty ? "empty" : "not empty");

    return isEmpty;
}

bool SdCard::isFileEmpty(const char* filePath)
{
    FILINFO fileInfo;
    FRESULT statResult = f_stat(filePath, &fileInfo);

    if (statResult == FR_NO_PATH)
    {
        LOG_WARNING(TAG, "'%s' — parent directory does not exist", filePath);
        return false;
    }

    if (statResult == FR_NO_FILE)
    {
        LOG_INFO(TAG, "'%s' does not exist", filePath);
        return false;
    }

    if (statResult != FR_OK)
    {
        LOG_ERROR(TAG, "f_stat failed for '%s': %s", filePath, fresultToString(statResult));
        return false;
    }

    bool isEmpty = (fileInfo.fsize == 0);
    LOG_INFO(TAG, "'%s' is %s", filePath, isEmpty ? "empty" : "not empty");

    return isEmpty;
}

bool SdCard::countFilesInDir(const char* dirPath, int& count)
{
    count = 0;

    DIR dir;
    FRESULT openResult = f_opendir(&dir, dirPath);

    if (openResult == FR_NO_PATH)
    {
        LOG_WARNING(TAG, "'%s' — parent directory does not exist", dirPath);
        return false;
    }

    if (openResult == FR_NO_FILE)
    {
        LOG_INFO(TAG, "'%s' does not exist", dirPath);
        return false;
    }

    if (openResult != FR_OK)
    {
        LOG_ERROR(TAG, "f_opendir failed for '%s': %s", dirPath, fresultToString(openResult));
        return false;
    }

    FILINFO fileInfo;
    while (f_readdir(&dir, &fileInfo) == FR_OK && fileInfo.fname[0] != 0)
    {
        count++;
    }

    f_closedir(&dir);

    LOG_INFO(TAG, "'%s' contains %d entries", dirPath, count);
    return true;
}

bool SdCard::generateSessionTimestamp()
{
    current_session = 0;

    HAL_StatusTypeDef result = HAL_RNG_GenerateRandomNumber(&hrng, &current_session);
    if (result != HAL_OK)
    {
        LOG_ERROR(TAG, "Generate Random Number failed, used backup solution");

        uint32_t backupUid = *(volatile uint32_t*)(0x1FFF7A10);
        current_session = HAL_GetTick() ^ SysTick->VAL ^ backupUid;

        return false;
    }

    return true;
}

bool SdCard::getFullFilePath(std::array<char, 64>& path) const
{
    snprintf(path.data(), path.size(), "/logs/session_%" PRIu32 ".log", current_session);
    return true;
}

bool SdCard::initLogFile()
{
    if (!dirExists("/logs"))
    {
        return false;
    }

    if (!generateSessionTimestamp())
    {
        LOG_WARNING(TAG, "Session timestamp generation failed, using fallback value");
    }

    std::array<char, 64> fullFilePath = {0};
    if (!getFullFilePath(fullFilePath))
    {
        //
    }

    if (fileExists(fullFilePath.data()))
    {
        LOG_WARNING(TAG, "Log file already exists");
        return false;
    }

    FIL file;
    FRESULT openResult = f_open(&file, fullFilePath.data(), FA_CREATE_NEW);
    if (openResult != FR_OK)
    {
        LOG_ERROR(TAG, "f_open failed: %s", fresultToString(openResult));
        return false;
    }

    f_close(&file);
    LOG_INFO(TAG, "File closed, done!");

    return true;
}

bool SdCard::updateCurrentLog(const char* message)
{
    MutexGuard guard(log_mutex_);

    std::array<char, 64> fullFilePath = {0};
    if (!getFullFilePath(fullFilePath))
    {
        //
    }

    if (!fileExists(fullFilePath.data()))
    {
        return false;
    }

    FIL file;
    FRESULT openResult = f_open(&file, fullFilePath.data(), FA_WRITE | FA_OPEN_APPEND);
    if (openResult != FR_OK)
    {
        LOG_ERROR(TAG, "f_open failed: %s", fresultToString(openResult));
        return false;
    }

    uint32_t tick = HAL_GetTick();
    uint32_t ms = tick % 1000;
    uint32_t sec = (tick / 1000) % 60;
    uint32_t min = (tick / 60000) % 60;
    uint32_t hour = tick / 3600000;

    std::array<char, 128> buffer{};
    snprintf(buffer.data(),                                                       //
             buffer.size(),                                                       //
             "[%02" PRIu32 ":%02" PRIu32 ":%02" PRIu32 ".%03" PRIu32 "] %s\r\n",  //
             hour,                                                                //
             min,                                                                 //
             sec,                                                                 //
             ms,                                                                  //
             message                                                              //
    );

    UINT bytesWritten = 0;
    FRESULT writeResult = f_write(&file, buffer.data(), strlen(buffer.data()), &bytesWritten);
    LOG_INFO(TAG, "f_write result=%s, bytesWritten=%u", fresultToString(writeResult), bytesWritten);

    f_close(&file);
    LOG_INFO(TAG, "File closed, done!");

    return true;
}

const char* SdCard::fresultToString(FRESULT result)
{
    switch (result)
    {
        case FR_OK:
            return "FR_OK";
        case FR_DISK_ERR:
            return "FR_DISK_ERR";
        case FR_INT_ERR:
            return "FR_INT_ERR";
        case FR_NOT_READY:
            return "FR_NOT_READY";
        case FR_NO_FILE:
            return "FR_NO_FILE";
        case FR_NO_PATH:
            return "FR_NO_PATH";
        case FR_INVALID_NAME:
            return "FR_INVALID_NAME";
        case FR_DENIED:
            return "FR_DENIED";
        case FR_EXIST:
            return "FR_EXIST";
        case FR_INVALID_OBJECT:
            return "FR_INVALID_OBJECT";
        case FR_WRITE_PROTECTED:
            return "FR_WRITE_PROTECTED";
        case FR_INVALID_DRIVE:
            return "FR_INVALID_DRIVE";
        case FR_NOT_ENABLED:
            return "FR_NOT_ENABLED";
        case FR_NO_FILESYSTEM:
            return "FR_NO_FILESYSTEM";
        case FR_MKFS_ABORTED:
            return "FR_MKFS_ABORTED";
        case FR_TIMEOUT:
            return "FR_TIMEOUT";
        case FR_LOCKED:
            return "FR_LOCKED";
        case FR_NOT_ENOUGH_CORE:
            return "FR_NOT_ENOUGH_CORE";
        case FR_TOO_MANY_OPEN_FILES:
            return "FR_TOO_MANY_OPEN_FILES";
        case FR_INVALID_PARAMETER:
            return "FR_INVALID_PARAMETER";
        default:
            return "UNKNOWN";
    }
}