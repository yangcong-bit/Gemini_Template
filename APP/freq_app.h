/* freq_app.h */
#ifndef __FREQ_APP_H
#define __FREQ_APP_H

#include "main.h"
#include "global_system.h" // 包含 sys.freq_ch1, sys.period_ch1, sys.duty_ch1 等

// ==========================================
// 核心宏定义：底层测频驱动模式切换
// 0: 使用 普通输入捕获 (软件三态极性切换，仅需1个通道，兼容性极强)
// 1: 使用 硬件PWM Input (全硬件自动重装载计算，适合超高频，需配置2个通道)
// ==========================================
//1.PWM
#define USE_HW_PWM_INPUT  0

void Freq_Init(void);
void Freq_Proc(void);

#endif
