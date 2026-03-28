/* freq_app.c */
#include "freq_app.h"
#include "tim.h"

// --- 通道 1 (TIM2, 32位) 状态变量 ---
static uint32_t ic1_val1 = 0; // 第一个上升沿
static uint32_t ic1_val2 = 0; // 下降沿
static uint32_t ic1_val3 = 0; // 第二个上升沿
static uint8_t  ic1_state = 0; 
static uint32_t ic1_last_tick = 0; 

// --- 通道 2 (TIM3, 16位) 状态变量 ---
static uint32_t ic2_val1 = 0;
static uint32_t ic2_val2 = 0;
static uint32_t ic2_val3 = 0;
static uint8_t  ic2_state = 0;
static uint32_t ic2_last_tick = 0; 

void Freq_Init(void) {
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);
}

/**
 * @brief 输入捕获中断回调函数 (动态切换极性)
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    // ---------------- 处理 TIM2 (Channel 1, 32位) ----------------
    if (htim->Instance == TIM2 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
        ic1_last_tick = HAL_GetTick(); 
        
        if (ic1_state == 0) {
            ic1_val1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            // 抓到上升沿后，立刻改为捕获下降沿
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);
            ic1_state = 1; 
        } else if (ic1_state == 1) {
            ic1_val2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            // 抓到下降沿后，立刻改回捕获上升沿
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
            ic1_state = 2; 
        } else if (ic1_state == 2) {
            ic1_val3 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            // 收集完毕，挂起等待处理 (保持上升沿捕获不变)
            ic1_state = 3; 
        }
    }
    
    // ---------------- 处理 TIM3 (Channel 1, 16位) ----------------
    if (htim->Instance == TIM3 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
        ic2_last_tick = HAL_GetTick(); 
        
        if (ic2_state == 0) {
            ic2_val1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);
            ic2_state = 1; 
        } else if (ic2_state == 1) {
            ic2_val2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
            ic2_state = 2; 
        } else if (ic2_state == 2) {
            ic2_val3 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            ic2_state = 3; 
        }
    }
}

/**
 * @brief 信号处理中心：计算频率、周期、占空比
 */
void Freq_Proc(void) {
    uint32_t current_tick = HAL_GetTick(); 

    // ==========================================
    // --- 计算通道 1 (TIM2, 32位防溢出) ---
    // ==========================================
    if (ic1_state == 3) {
        uint32_t period_ticks = 0;
        uint32_t high_ticks = 0;

        // 1. 计算总周期 (T3 - T1)
        if (ic1_val3 >= ic1_val1) period_ticks = ic1_val3 - ic1_val1;
        else period_ticks = (0xFFFFFFFF - ic1_val1) + ic1_val3 + 1;
        
        // 2. 计算高电平时间 (T2 - T1)
        if (ic1_val2 >= ic1_val1) high_ticks = ic1_val2 - ic1_val1;
        else high_ticks = (0xFFFFFFFF - ic1_val1) + ic1_val2 + 1;
        
        if (period_ticks > 0) {
            // 【周期】：假设定时器时钟为 1MHz，则 1 tick = 1 us
            sys.period_ch1 = period_ticks; 
            
            // 【频率】：f = 1,000,000 / T
            sys.freq_ch1 = 1000000 / period_ticks;
            
            // 【占空比】：(高电平时间 / 周期) * 100%
            sys.duty_ch1 = ((float)high_ticks / (float)period_ticks) * 100.0f;
        }
        ic1_state = 0; // 数据消化完毕，重启捕获状态机
    }
    
    // 零频超时保护：超过 1000ms 没有捕获到完整波形，全部强制归零
    if (current_tick - ic1_last_tick > 1000) {
        sys.freq_ch1 = 0;
        sys.period_ch1 = 0;
        sys.duty_ch1 = 0.0f;
        
        // 强制重置状态和极性，防止状态机卡死在寻找下降沿的途中
        ic1_state = 0; 
        __HAL_TIM_SET_CAPTUREPOLARITY(&htim2, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
    }
    
    // ==========================================
    // --- 计算通道 2 (TIM3, 16位防溢出) ---
    // ==========================================
    if (ic2_state == 3) {
        uint32_t period_ticks = 0;
        uint32_t high_ticks = 0;

        // 1. 计算总周期 (T3 - T1) 注意是 16位溢出 0xFFFF
        if (ic2_val3 >= ic2_val1) period_ticks = ic2_val3 - ic2_val1;
        else period_ticks = (0xFFFF - ic2_val1) + ic2_val3 + 1;
        
        // 2. 计算高电平时间 (T2 - T1)
        if (ic2_val2 >= ic2_val1) high_ticks = ic2_val2 - ic2_val1;
        else high_ticks = (0xFFFF - ic2_val1) + ic2_val2 + 1;
        
        if (period_ticks > 0) {
            sys.period_ch2 = period_ticks; 
            sys.freq_ch2 = 1000000 / period_ticks;
            sys.duty_ch2 = ((float)high_ticks / (float)period_ticks) * 100.0f;
        }
        ic2_state = 0; 
    }
    
    // 零频超时保护
    if (current_tick - ic2_last_tick > 1000) {
        sys.freq_ch2 = 0;
        sys.period_ch2 = 0;
        sys.duty_ch2 = 0.0f;
        ic2_state = 0; 
        __HAL_TIM_SET_CAPTUREPOLARITY(&htim3, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
    }
}
