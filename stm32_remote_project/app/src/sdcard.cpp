#include "sdcard.hpp"

bool SdCard::init()
{
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

    return true;
}

FRESULT SdCard::mount()
{
    FRESULT mountResult = f_mount(&fs_, SDPath, 1);
    if (mountResult != FR_OK)
    {
        LOG_ERROR(TAG, "f_mount failed: %d", mountResult);
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
        LOG_ERROR(TAG, "f_mkfs failed: %d", mkfsResult);
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
        LOG_ERROR(TAG, "f_stat failed for '%s': %d", dirPath, statResult);
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
        LOG_ERROR(TAG, "f_stat failed for '%s': %d", filePath, statResult);
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
        LOG_ERROR(TAG, "f_mkdir failed for '%s': %d", dirPath, result);
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
        LOG_ERROR(TAG, "f_opendir failed for '%s': %d", dirPath, openResult);
        return false;
    }

    FILINFO fileInfo;
    FRESULT readResult = f_readdir(&dir, &fileInfo);
    f_closedir(&dir);

    if (readResult != FR_OK)
    {
        LOG_ERROR(TAG, "f_readdir failed for '%s': %d", dirPath, readResult);
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
        LOG_ERROR(TAG, "f_stat failed for '%s': %d", filePath, statResult);
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
        LOG_ERROR(TAG, "f_opendir failed for '%s': %d", dirPath, openResult);
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
