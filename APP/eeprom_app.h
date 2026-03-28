/* eeprom_app.h */
#ifndef __EEPROM_APP_H
#define __EEPROM_APP_H

#include "main.h"
#include "global_system.h"

#define MAX_RECORDS 5  // 定义环形栈深度 (题目要求存几条就写几)

// 暴露给外部的 RAM 历史记录镜像 (UI 可以直接读取它来显示)
extern LogData_t eeprom_history[MAX_RECORDS];
extern uint8_t   eeprom_log_idx; // 指示当前最新数据所在的槽位

void EEPROM_Init(void);
void EEPROM_Proc(void);

// 暴露给外部业务的通用入队接口
void EEPROM_PushLog(LogData_t new_log);

#endif
