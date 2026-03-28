/**
 * @file    led_app.h
 * @brief   LED 控制模块 (状态机映射与软定时防死锁)
 * @note    业务模块如果需要点灯，请直接操作全局字典：sys.led_ctrl[x]。
 */
#ifndef __LED_APP_H
#define __LED_APP_H

#include "main.h"

void LED_Disp(void); 
void LED_Proc(void); 

#endif /* __LED_APP_H */
