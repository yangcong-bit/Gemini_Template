/* led_app.h */
#ifndef __LED_APP_H
#define __LED_APP_H

#include "main.h"
#include "global_system.h" 

// 暴露给外部的 LED 控制数组，0 灭，1 亮
// [0] 对应 LD1, [7] 对应 LD8
extern uint8_t led_ctrl[8];

void LED_Disp(void);
void LED_Proc(void);

#endif
