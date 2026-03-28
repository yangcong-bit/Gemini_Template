/* freq_app.h */
#ifndef __FREQ_APP_H
#define __FREQ_APP_H

#include "main.h"
#include "global_system.h" // 包含 sys.freq_ch1 等

// ==========================================
// 独立探针：自动嗅探通道1和通道2的工作模式
// 依赖 CubeMX 中的 User Label (引脚标签) 自动生成的宏
// ==========================================

// --- 嗅探通道1 (TIM2) ---
#ifdef FREQ_CH1_HW_Pin
    #define USE_HW_PWM_CH1 1  // 嗅探到标签，启用 CH1 硬件模式
#else
    #define USE_HW_PWM_CH1 0  // 降级为 CH1 软件状态机模式
#endif

// --- 嗅探通道2 (TIM3) ---
#ifdef FREQ_CH2_HW_Pin
    #define USE_HW_PWM_CH2 1  // 嗅探到标签，启用 CH2 硬件模式
#else
    #define USE_HW_PWM_CH2 0  // 降级为 CH2 软件状态机模式
#endif

void Freq_Init(void);
void Freq_Proc(void);

#endif
