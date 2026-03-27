/* lcd_app.h */
#ifndef __LCD_APP_H
#define __LCD_APP_H

#include "main.h"
#include "lcd.h"           // 官方提供的 LCD 底层驱动
#include "global_system.h" // 引入全局数据字典，获取系统变量

// 暴露给外层的接口
void UI_Init(void);
void UI_Proc(void);

#endif
