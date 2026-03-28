/**
 * @file    lcd_app.h
 * @brief   LCD UI 渲染模块 (防闪屏机制)
 * @note    纯消费者：所有显示数据均来自 global_system.h。
 */
#ifndef __LCD_APP_H
#define __LCD_APP_H

#include "main.h"

void UI_Init(void); 
void UI_Proc(void); 

#endif /* __LCD_APP_H */
