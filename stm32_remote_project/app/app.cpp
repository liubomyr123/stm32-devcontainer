#include <array>

#include "cmsis_os.h"
#include "fatfs.h"
#include "include/logger.hpp"
#include "include/nrf24radio.hpp"

extern SPI_HandleTypeDef hspi1;
extern SD_HandleTypeDef hsd;

#define NRF_CSN_PORT GPIOB
#define NRF_CSN_PIN GPIO_PIN_1

#define NRF_CE_PORT GPIOB
#define NRF_CE_PIN GPIO_PIN_0

// Спільний RF-фільтр-ключ для комунікації пульт↔senses-плата (MVP, один канал)
constexpr std::array<uint8_t, 5> SHARED_RF_FILTER_KEY = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};

// Робочий канал — подалі від типових WiFi-каналів (1/6/11)
// F0 = 2400 + 100 = 2500 MHz
constexpr uint8_t SHARED_CHANNEL = 100;

extern "C" void app_main()
{
    LOG_INFO("APP", "Started!");

    HAL_SD_CardStateTypeDef cardState = HAL_SD_GetCardState(&hsd);
    LOG_INFO("SD", "HAL_SD_GetCardState = %d (expect HAL_SD_CARD_TRANSFER = 4)", cardState);

    GPIO_PinState cardDetect = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8);
    LOG_INFO("SD", "PA8 (card detect) = %d (1=inserted, 0=empty)", cardDetect);

    HAL_StatusTypeDef sdInitStatus = HAL_SD_Init(&hsd);
    LOG_INFO("SD", "HAL_SD_Init status=%d, ErrorCode=0x%08lX", sdInitStatus, hsd.ErrorCode);

    BYTE work[_MAX_SS];
    FRESULT mkfsResult = f_mkfs(SDPath, FM_FAT32, 0, work, sizeof(work));
    LOG_INFO("SD", "f_mkfs result=%d", mkfsResult);

    // FATFS-об'єкт: внутрішня структура бібліотеки, що зберігає
    // стан примонтованої файлової системи. Має existувати весь час,
    // поки диск примонтований — не можна створювати локально й губити.
    static FATFS fs;

    // f_mount montує файлову систему на логічний диск "0" (SDPath,
    // згенерований CubeMX — те саме ім'я тому, що в нас лише 1 SD-картка).
    FRESULT mountResult = f_mount(&fs, SDPath, 1);
    if (mountResult != FR_OK)
    {
        LOG_ERROR("SD", "f_mount failed: %d", mountResult);
        vTaskDelete(nullptr);
    }
    LOG_INFO("SD", "Mounted OK");

    // f_open з FA_WRITE|FA_CREATE_ALWAYS: якщо файла нема — створює,
    // якщо є — перезаписує з нуля (для першого тесту це найпростіше).
    FIL file;
    FRESULT openResult = f_open(&file, "test.txt", FA_WRITE | FA_CREATE_ALWAYS);
    if (openResult != FR_OK)
    {
        LOG_ERROR("SD", "f_open failed: %d", openResult);
        vTaskDelete(nullptr);
    }

    const char message[] = "Hello from STM32!\r\n";
    UINT bytesWritten = 0;
    FRESULT writeResult = f_write(&file, message, sizeof(message) - 1, &bytesWritten);
    LOG_INFO("SD", "f_write result=%d, bytesWritten=%u", writeResult, bytesWritten);

    f_close(&file);
    LOG_INFO("SD", "File closed, done!");

    Nrf24Radio nrf{&hspi1,                     //
                   NRF_CSN_PORT, NRF_CSN_PIN,  //
                   NRF_CE_PORT,  NRF_CE_PIN,   //
                   Direction::Tx};

    if (!nrf.init())
    {
        vTaskDelete(nullptr);
    }

    nrf.setAirDataRate(DataRate::Mbps1);
    nrf.setChannel(SHARED_CHANNEL);
    nrf.setTxRfFilterKey(SHARED_RF_FILTER_KEY);

    LOG_INFO("NRF", "State before enableTx() = %d", static_cast<int>(nrf.getCurrentState()));
    nrf.enableTx();
    RadioState txState = nrf.getCurrentState();
    LOG_INFO("NRF", "State after enableTx() = %d (expect StandbyII = %d)",
             static_cast<int>(txState), static_cast<int>(RadioState::StandbyII));

    uint32_t counter = 0;

    while (true)
    {
        uint8_t buffer[4] = {
            static_cast<uint8_t>((counter >> 24) & 0xFF),
            static_cast<uint8_t>((counter >> 16) & 0xFF),
            static_cast<uint8_t>((counter >> 8) & 0xFF),
            static_cast<uint8_t>(counter & 0xFF),
        };

        bool sent = nrf.transmit(buffer, sizeof(buffer));
        LOG_INFO("NRF", "Transmit #%lu: %s", counter, sent ? "OK" : "FAILED");

        counter++;

        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
        osDelay(1000);
    }
}