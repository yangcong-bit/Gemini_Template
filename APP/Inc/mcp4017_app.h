/**
 * @file    mcp4017_app.h
 * @brief   MCP4017 可编程数字电位器控制模块
 * @note    纯消费者模块，负责监控全局字典中的 sys.res_step，并在发生变化时通过 I2C 下发指令。
 */
#ifndef __MCP4017_APP_H
#define __MCP4017_APP_H

#include "main.h"

void MCP4017_Init(void);
void MCP4017_Proc(void);

#endif /* __MCP4017_APP_H */
