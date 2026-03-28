/* key_app.h */
#ifndef __KEY_APP_H
#define __KEY_APP_H

#include "main.h"
#include "global_system.h" 

#define KEY1  1
#define KEY2  2
#define KEY3  3
#define KEY4  4

// 导出调度器任务函数
void Key_Proc(void);

// 导出队列出队函数（供消费者调用）
bool Key_Get_Event(uint8_t *out_event);

#endif
