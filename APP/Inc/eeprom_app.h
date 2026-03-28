/**
 * @file    eeprom_app.h
 * @brief   EEPROM 非阻塞异步存储与历史数据恢复模块
 * @note    支持结构体存取、自动寻址、以及格式化标记。
 */
#ifndef __EEPROM_APP_H
#define __EEPROM_APP_H

#include "main.h"
#include "global_system.h" // 引入 LogData_t 结构体定义

void EEPROM_Init(void);  
void EEPROM_Proc(void);  

/* 暴露给外部业务的通用入队接口 (按键/串口模块调用它来保存数据) */
void EEPROM_PushLog(LogData_t new_log);

#endif /* __EEPROM_APP_H */
