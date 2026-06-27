#ifndef LOGGER_H
#define LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

void log_debug(const char* tag, const char* file, int line, const char* fmt, ...);
void log_info(const char* tag, const char* file, int line, const char* fmt, ...);
void log_warning(const char* tag, const char* file, int line, const char* fmt, ...);
void log_error(const char* tag, const char* file, int line, const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#define LOG_DEBUG(tag, fmt, ...)   log_debug(tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(tag, fmt, ...)    log_info(tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARNING(tag, fmt, ...) log_warning(tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(tag, fmt, ...)   log_error(tag, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif
