/* debug_log.h */
#ifndef __DEBUG_LOG_H
#define __DEBUG_LOG_H

#include "main.h"

/* ==========================================
 * 1. 日志等级宏定义
 * ========================================== */
#define LOG_LEVEL_NONE  0  // 【比赛提交模式】静默，无任何输出，代码零开销
#define LOG_LEVEL_ERROR 1  // 【严重错误】如硬件初始化失败、队列溢出
#define LOG_LEVEL_WARN  2  // 【警告信息】如数据越限、异常输入
#define LOG_LEVEL_INFO  3  // 【一般提示】如状态机切换、业务流程节点
#define LOG_LEVEL_DEBUG 4  // 【底层调试】如原始ADC值、按键抖动等海量信息

/* ==========================================
 * 2. 全局日志输出等级控制开关 (核心修改点)
 * ========================================== */
// 比赛前，将此处改为 LOG_LEVEL_NONE 即可一键抹除所有调试代码！
#define SYSTEM_LOG_LEVEL  LOG_LEVEL_NONE

/* ==========================================
 * 3. 内部打印函数声明 (业务层不要直接调用)
 * ========================================== */
void Debug_Log_Print(const char *level_str, const char *file, int line, const char *fmt, ...);

/* ==========================================
 * 4. 业务层调用的变参宏 (自动补充文件名和行号)
 * ========================================== */
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

#endif /* __DEBUG_LOG_H */
