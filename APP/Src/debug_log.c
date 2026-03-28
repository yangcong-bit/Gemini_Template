/* debug_log.c */
#include "debug_log.h"
#include "usart.h" // 引入底层串口句柄 huart1
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define LOG_BUFFER_SIZE 256  // 单条日志最大长度

/**
 * @brief  系统底层日志格式化与发送引擎
 * @note   只有在宏定义允许的等级下，才会被编译和调用
 */
void Debug_Log_Print(const char *level_str, const char *file, int line, const char *fmt, ...) {
    // 双保险：如果等级为 NONE，直接跳过 (其实头文件宏里已经拦截了)
    #if (SYSTEM_LOG_LEVEL > LOG_LEVEL_NONE)
    
    char log_buf[LOG_BUFFER_SIZE];
    char msg_buf[LOG_BUFFER_SIZE];
    
    // 1. 处理不定长参数，格式化用户核心消息 (类似 printf)
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg_buf, sizeof(msg_buf), fmt, args);
    va_end(args);
    
    // 2. 自动裁剪绝对路径，仅保留纯文件名
    // MDK/Keil 的 __FILE__ 宏往往是带盘符的绝对路径，太长会浪费串口带宽
    const char *file_name = strrchr(file, '/');
    if (!file_name) {
        file_name = strrchr(file, '\\'); // 兼容 Windows 反斜杠
    }
    if (file_name) {
        file_name++; // 跳过最后的斜杠
    } else {
        file_name = file; // 如果都没找到，说明本身就是纯文件名
    }
    
    // 3. 组装终极格式: [时间戳][等级](文件名:行号) 消息\r\n
    // 例如: [00015432][INFO ](key_app.c:64) Key 1 short pressed.
    snprintf(log_buf, sizeof(log_buf), "[%08lu][%s](%s:%d) %s\r\n", 
             HAL_GetTick(), level_str, file_name, line, msg_buf);
             
    // 4. 调用底层串口阻塞发送 
    // 【重要提示】：调试期必须用阻塞发送 (HAL_MAX_DELAY)，保证日志完整不丢失，不被 DMA 覆盖
    HAL_UART_Transmit(&huart1, (uint8_t *)log_buf, strlen(log_buf), HAL_MAX_DELAY);
    
    #endif
}
