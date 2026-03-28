/**
 * @file    freq_app.h
 * @brief   双通道输入捕获 (测频/周期/占空比)
 * @note    支持 CubeMX 标签自适应嗅探，硬件模式(主从定时器)与软件模式(极性反转)无缝切换。
 */
#ifndef __FREQ_APP_H
#define __FREQ_APP_H

#include "main.h"

// --- 嗅探通道1 (TIM2) 宏配置 ---
#ifdef FREQ_CH1_HW_Pin
    #define USE_HW_PWM_CH1 1  ///< 嗅探到标签，启用 CH1 纯硬件模式 (零 CPU 开销)
#else
    #define USE_HW_PWM_CH1 0  ///< 降级为 CH1 软件中断极性翻转模式
#endif

// --- 嗅探通道2 (TIM3) 宏配置 ---
#ifdef FREQ_CH2_HW_Pin
    #define USE_HW_PWM_CH2 1  
#else
    #define USE_HW_PWM_CH2 0  
#endif

void Freq_Init(void);
void Freq_Proc(void);

#endif /* __FREQ_APP_H */
