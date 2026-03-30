// debug_log.h
#ifndef __DEBUG_LOG_H
#define __DEBUG_LOG_H

#include "main.h"

#define LOG_LEVEL_NONE  0  
#define LOG_LEVEL_ERROR 1  
#define LOG_LEVEL_WARN  2  
#define LOG_LEVEL_INFO  3  
#define LOG_LEVEL_DEBUG 4  

#define SYSTEM_LOG_LEVEL  LOG_LEVEL_NONE 

void Debug_Log_Print(const char *level_str, const char *file, int line, const char *fmt, ...);

#if (SYSTEM_LOG_LEVEL >= LOG_LEVEL_ERROR)
    #define LOG_E(fmt, ...) Debug_Log_Print("ERROR", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
    #define LOG_E(fmt, ...)
#endif

#if (SYSTEM_LOG_LEVEL >= LOG_LEVEL_WARN)
    #define LOG_W(fmt, ...) Debug_Log_Print("WARN ", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
    #define LOG_W(fmt, ...)
#endif

#if (SYSTEM_LOG_LEVEL >= LOG_LEVEL_INFO)
    #define LOG_I(fmt, ...) Debug_Log_Print("INFO ", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
    #define LOG_I(fmt, ...)
#endif

#if (SYSTEM_LOG_LEVEL >= LOG_LEVEL_DEBUG)
    #define LOG_D(fmt, ...) Debug_Log_Print("DEBUG", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
    #define LOG_D(fmt, ...)
#endif

#endif
