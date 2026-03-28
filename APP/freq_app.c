/* freq_app.c */
#include "freq_app.h"
#include "tim.h"

// ==========================================================
// 1. 通道 1 (TIM2 - 32位) 变量定义
// ==========================================================
#if USE_HW_PWM_CH1 == 1
    // CH1 硬件模式专属变量
    static uint32_t ic1_period = 0, ic1_high = 0;
    static uint8_t  ic1_ready  = 0; 
#else
    // CH1 软件模式专属变量
    static uint32_t ic1_val1 = 0, ic1_val2 = 0, ic1_val3 = 0;
    static uint8_t  ic1_state  = 0; 
#endif
static uint32_t ic1_last_tick = 0; 

// ==========================================================
// 2. 通道 2 (TIM3 - 16位) 变量定义
// ==========================================================
#if USE_HW_PWM_CH2 == 1
    // CH2 硬件模式专属变量
    static uint32_t ic2_period = 0, ic2_high = 0;
    static uint8_t  ic2_ready  = 0;
#else
    // CH2 软件模式专属变量
    static uint32_t ic2_val1 = 0, ic2_val2 = 0, ic2_val3 = 0;
    static uint8_t  ic2_state  = 0;
#endif
static uint32_t ic2_last_tick = 0; 

// ==========================================================
// 初始化函数 (独立启动各通道)
// ==========================================================
void Freq_Init(void) {
    // --- 启动 CH1 (TIM2) ---
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
#if USE_HW_PWM_CH1 == 1
    HAL_TIM_IC_Start(&htim2, TIM_CHANNEL_2); // 硬件模式需额外开启副通道抓占空比
#endif

    // --- 启动 CH2 (TIM3) ---
    HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);
#if USE_HW_PWM_CH2 == 1
    HAL_TIM_IC_Start(&htim3, TIM_CHANNEL_2); 
#endif
}

// ==========================================================
// 中断回调函数 (独立处理硬件/软件逻辑)
// ==========================================================
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    
    // ---------------- 处理 TIM2 (Channel 1) ----------------
    if (htim->Instance == TIM2 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
        ic1_last_tick = HAL_GetTick(); 
        
#if USE_HW_PWM_CH1 == 1
        // 【硬件模式】：直接读数
        ic1_period = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
        ic1_high   = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
        ic1_ready  = 1;
#else
        // 【软件模式】：极性切换三步曲
        if (ic1_state == 0) {
            ic1_val1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);
            ic1_state = 1; 
        } else if (ic1_state == 1) {
            ic1_val2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
            ic1_state = 2; 
        } else if (ic1_state == 2) {
            ic1_val3 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            ic1_state = 3; 
        }
#endif
    }
    
    // ---------------- 处理 TIM3 (Channel 1) ----------------
    if (htim->Instance == TIM3 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
        ic2_last_tick = HAL_GetTick(); 
        
#if USE_HW_PWM_CH2 == 1
        ic2_period = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
        ic2_high   = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
        ic2_ready  = 1;
#else
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
#endif
    }
}

// ==========================================================
// 业务结算函数 (独立计算防溢出算法)
// ==========================================================
void Freq_Proc(void) {
    uint32_t current_tick = HAL_GetTick();

    // ================= CH1 (TIM2 - 32位) 结算 =================
#if USE_HW_PWM_CH1 == 1
    if (ic1_ready) {
        if (ic1_period != 0) {
            sys.period_ch1 = ic1_period;
            sys.freq_ch1 = 1000000 / ic1_period; 
            sys.duty_ch1 = ((float)ic1_high / (float)ic1_period) * 100.0f;
        }
        ic1_ready = 0;
    }
#else
    if (ic1_state == 3) {
        uint32_t period_ticks = (ic1_val3 >= ic1_val1) ? (ic1_val3 - ic1_val1) : ((0xFFFFFFFF - ic1_val1) + ic1_val3 + 1);
        uint32_t high_ticks   = (ic1_val2 >= ic1_val1) ? (ic1_val2 - ic1_val1) : ((0xFFFFFFFF - ic1_val1) + ic1_val2 + 1);
        if (period_ticks > 0) {
            sys.period_ch1 = period_ticks; 
            sys.freq_ch1 = 1000000 / period_ticks;
            sys.duty_ch1 = ((float)high_ticks / (float)period_ticks) * 100.0f;
        }
        ic1_state = 0; 
    }
#endif
    
    // 零频保护
    if (current_tick - ic1_last_tick > 1000) {
        sys.freq_ch1 = 0; sys.period_ch1 = 0; sys.duty_ch1 = 0.0f;
#if USE_HW_PWM_CH1 == 0
        ic1_state = 0; 
        __HAL_TIM_SET_CAPTUREPOLARITY(&htim2, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
#endif
    }

    // ================= CH2 (TIM3 - 16位) 结算 =================
#if USE_HW_PWM_CH2 == 1
    if (ic2_ready) {
        if (ic2_period != 0) {
            sys.period_ch2 = ic2_period;
            sys.freq_ch2 = 1000000 / ic2_period;
            sys.duty_ch2 = ((float)ic2_high / (float)ic2_period) * 100.0f;
        }
        ic2_ready = 0;
    }
#else
    if (ic2_state == 3) {
        // 注意：TIM3 是 16位计数器，溢出值是 0xFFFF
        uint32_t period_ticks = (ic2_val3 >= ic2_val1) ? (ic2_val3 - ic2_val1) : ((0xFFFF - ic2_val1) + ic2_val3 + 1);
        uint32_t high_ticks   = (ic2_val2 >= ic2_val1) ? (ic2_val2 - ic2_val1) : ((0xFFFF - ic2_val1) + ic2_val2 + 1);
        if (period_ticks > 0) {
            sys.period_ch2 = period_ticks; 
            sys.freq_ch2 = 1000000 / period_ticks;
            sys.duty_ch2 = ((float)high_ticks / (float)period_ticks) * 100.0f;
        }
        ic2_state = 0; 
    }
#endif

    // 零频保护
    if (current_tick - ic2_last_tick > 1000) {
        sys.freq_ch2 = 0; sys.period_ch2 = 0; sys.duty_ch2 = 0.0f;
#if USE_HW_PWM_CH2 == 0
        ic2_state = 0; 
        __HAL_TIM_SET_CAPTUREPOLARITY(&htim3, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
#endif
    }
}
