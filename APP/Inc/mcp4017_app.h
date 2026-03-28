/* mcp4017_app.h */
#ifndef __MCP4017_APP_H
#define __MCP4017_APP_H

#include "main.h"
#include "global_system.h"

// 暴露给外部的初始化和调度接口
void MCP4017_Init(void);
void MCP4017_Proc(void);

#endif
