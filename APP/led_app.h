/* led_app.h */
#ifndef __LED_APP_H
#define __LED_APP_H

#include "main.h"
#include "global_system.h" // 引入全局数据字典，用于获取业务状态

// 硬件底层操作接口
void LED_Disp(uint8_t led_status);

// 调度器任务函数
void LED_Proc(void);

#endif
