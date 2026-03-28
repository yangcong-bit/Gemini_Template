/**
 * @file    key_app.h
 * @brief   按键处理模块 (带消抖、长按、双击检测)
 * @note    采用非阻塞状态机与环形消息队列机制。
 * 底层只负责生产事件压入 sys.key_queue，外部 UI 模块负责消费。
 */
#ifndef __KEY_APP_H
#define __KEY_APP_H

#include "main.h"
#include "global_system.h" 

#define KEY1  1
#define KEY2  2
#define KEY3  3
#define KEY4  4

/* 调度器任务函数 */
void Key_Proc(void);

/* 队列出队函数（供消费者调用） */
bool Key_Get_Event(uint8_t *out_event);

#endif /* __KEY_APP_H */
