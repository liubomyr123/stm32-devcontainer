#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <cstdarg>

#include "stm32f4xx_hal.h"

enum class LogLevel
{
    DBG,
    INFO,
    WARNING,
    ERROR
};

class Logger
{
   public:
    static Logger& instance();
    void setLevel(LogLevel level);
    void log(LogLevel level, const char* tag, const char* file, int line, const char* fmt,
             va_list args);
    void debug(const char* tag, const char* file, int line, const char* fmt, ...);
    void info(const char* tag, const char* file, int line, const char* fmt, ...);
    void warning(const char* tag, const char* file, int line, const char* fmt, ...);
    void error(const char* tag, const char* file, int line, const char* fmt, ...);

   private:
    Logger() = default;
    static const char* levelToString(LogLevel level);
    LogLevel minLevel_ = LogLevel::DBG;
};

#define LOG_DEBUG(tag, fmt, ...) \
    Logger::instance().debug(tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(tag, fmt, ...) Logger::instance().info(tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARNING(tag, fmt, ...) \
    Logger::instance().warning(tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(tag, fmt, ...) \
    Logger::instance().error(tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif