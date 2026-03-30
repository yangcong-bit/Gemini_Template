// debug_log.c
#include "debug_log.h"
#include "usart.h" 
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define LOG_BUFFER_SIZE 256  

void Debug_Log_Print(const char *level_str, const char *file, int line, const char *fmt, ...) {
    #if (SYSTEM_LOG_LEVEL > LOG_LEVEL_NONE)

    char log_buf[LOG_BUFFER_SIZE];
    char msg_buf[LOG_BUFFER_SIZE];

    va_list args;
    va_start(args, fmt);
    vsnprintf(msg_buf, sizeof(msg_buf), fmt, args);
    va_end(args);

    const char *file_name = strrchr(file, '/');
    if (!file_name) {
        file_name = strrchr(file, '\\'); 
    }
    if (file_name) {
        file_name++; 
    } else {
        file_name = file;
    }

    snprintf(log_buf, sizeof(log_buf), "[%08lu][%s](%s:%d) %s\r\n", 
             HAL_GetTick(), level_str, file_name, line, msg_buf);

    HAL_UART_Transmit(&huart1, (uint8_t *)log_buf, strlen(log_buf), HAL_MAX_DELAY);

    #endif
}
