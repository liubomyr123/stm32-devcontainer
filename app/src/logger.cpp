#include "include/logger.hpp"
#include <stdio.h>
#include <stdarg.h>

extern UART_HandleTypeDef huart1;

extern "C" int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::setLevel(LogLevel level) {
    minLevel_ = level;
}

void Logger::debug(const char* tag, const char* file, int line, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log(LogLevel::DBG, tag, file, line, fmt, args);
    va_end(args);
}

void Logger::info(const char* tag, const char* file, int line, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log(LogLevel::INFO, tag, file, line, fmt, args);
    va_end(args);
}

void Logger::warning(const char* tag, const char* file, int line, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log(LogLevel::WARNING, tag, file, line, fmt, args);
    va_end(args);
}

void Logger::error(const char* tag, const char* file, int line, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log(LogLevel::ERROR, tag, file, line, fmt, args);
    va_end(args);
}

void Logger::log(LogLevel level, const char* tag, const char* file, int line, const char* fmt, va_list args) {
    if (level < minLevel_) return;

    // Форматований час
    uint32_t tick = HAL_GetTick();
    uint32_t ms   = tick % 1000;
    uint32_t sec  = (tick / 1000) % 60;
    uint32_t min  = (tick / 60000) % 60;
    uint32_t hour = tick / 3600000;

    // Тільки ім'я файлу без повного шляху
    const char* filename = file;
    for (const char* p = file; *p; p++) {
        if (*p == '/' || *p == '\\') filename = p + 1;
    }

    printf("[%02lu:%02lu:%02lu.%03lu][%s][%s][%s:%d]: ",
           hour, min, sec, ms,
           levelToString(level), tag, filename, line);
    vprintf(fmt, args);
    printf("\r\n");
}

const char* Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DBG:     return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        default:                return "UNKNOWN";
    }
}

extern "C" void log_debug(const char* tag, const char* file, int line, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Logger::instance().log(LogLevel::DBG, tag, file, line, fmt, args);
    va_end(args);
}

extern "C" void log_info(const char* tag, const char* file, int line, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Logger::instance().log(LogLevel::INFO, tag, file, line, fmt, args);
    va_end(args);
}

extern "C" void log_warning(const char* tag, const char* file, int line, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Logger::instance().log(LogLevel::WARNING, tag, file, line, fmt, args);
    va_end(args);
}

extern "C" void log_error(const char* tag, const char* file, int line, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Logger::instance().log(LogLevel::ERROR, tag, file, line, fmt, args);
    va_end(args);
}