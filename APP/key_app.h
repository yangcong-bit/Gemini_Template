/* key_app.h */
#ifndef __KEY_APP_H
#define __KEY_APP_H

#include "main.h"
#include "global_system.h" // 引入全局数据字典，用于写入按键事件

// 按键定义 (对应蓝桥杯 CT117E-M4 上的 B1~B4)
#define KEY1  1
#define KEY2  2
#define KEY3  3
#define KEY4  4

// 导出调度器任务函数
void Key_Proc(void);

#endif
