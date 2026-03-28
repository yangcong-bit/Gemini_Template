/* freq_app.c */
#include "freq_app.h"
#include "tim.h"

// ==============================================================================
// 模式 1：硬件 PWM Input 模式 (高性能，全硬件计算)
// ==============================================================================
#if USE_HW_PWM_INPUT == 1

// 硬件模式下，直接抓取周期和高电平计数值即可，不需要状态机
static uint32_t ic1_period = 0;
static uint32_t ic1_high   = 0;
static uint32_t ic1_last_tick = 0;
static uint8_t  ic1_ready  = 0; 

static uint32_t ic2_period = 0;
static uint32_t ic2_high   = 0;
static uint32_t ic2_last_tick = 0;
static uint8_t  ic2_ready  = 0;

void Freq_Init(void) {
    // 开启通道1的捕获中断 (用于抓取周期)
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);
    
    // 开启通道2的捕获 (用于抓取占空比，无需中断，硬件自动同步)
    HAL_TIM_IC_Start(&htim2, TIM_CHANNEL_2);
    HAL_TIM_IC_Start(&htim3, TIM_CHANNEL_2);
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    // TIM2 硬件 PWM 捕获
    if (htim->Instance == TIM2 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
        // 因为是从机复位模式，CCR1里面直接就是周期，CCR2里面直接就是高电平时间
        ic1_period = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
        ic1_high   = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
        ic1_last_tick = HAL_GetTick();
        ic1_ready  = 1;
    }
    
    // TIM3 硬件 PWM 捕获
    if (htim->Instance == TIM3 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
        ic2_period = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
        ic2_high   = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
        ic2_last_tick = HAL_GetTick();
        ic2_ready  = 1;
    }
}

void Freq_Proc(void) {
    uint32_t current_tick = HAL_GetTick();

    // --- TIM2 (CH1) 数据结算 ---
    if (ic1_ready) {
        if (ic1_period != 0) {
            sys.period_ch1 = ic1_period;
            sys.freq_ch1 = 1000000 / ic1_period; // 假设定时器时钟 1MHz
            sys.duty_ch1 = ((float)ic1_high / (float)ic1_period) * 100.0f;
        }
        ic1_ready = 0;
    }
    // 零频超时保护：超过 1000ms 没有捕获到中断，强制归零
    if (current_tick - ic1_last_tick > 1000) {
        sys.freq_ch1 = 0;
        sys.period_ch1 = 0;
        sys.duty_ch1 = 0.0f;
    }

    // --- TIM3 (CH2) 数据结算 ---
    if (ic2_ready) {
        if (ic2_period != 0) {
            sys.period_ch2 = ic2_period;
            sys.freq_ch2 = 1000000 / ic2_period;
            sys.duty_ch2 = ((float)ic2_high / (float)ic2_period) * 100.0f;
        }
        ic2_ready = 0;
    }
    // 零频超时保护
    if (current_tick - ic2_last_tick > 1000) {
        sys.freq_ch2 = 0;
        sys.period_ch2 = 0;
        sys.duty_ch2 = 0.0f;
    }
}


// ==============================================================================
// 模式 0：普通输入捕获模式 (软件三态极性切换，抗锯齿能力强)
// ==============================================================================
#else

// --- 状态机变量 ---
static uint32_t ic1_val1 = 0, ic1_val2 = 0, ic1_val3 = 0;
static uint8_t  ic1_state = 0; 
static uint32_t ic1_last_tick = 0; 

static uint32_t ic2_val1 = 0, ic2_val2 = 0, ic2_val3 = 0;
static uint8_t  ic2_state = 0;
static uint32_t ic2_last_tick = 0; 

void Freq_Init(void) {
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    // ---------------- 处理 TIM2 (Channel 1, 32位) ----------------
    if (htim->Instance == TIM2 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
        ic1_last_tick = HAL_GetTick(); 
        
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
            ic1_state = 3; // 挂起等待处理
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

void Freq_Proc(void) {
    uint32_t current_tick = HAL_GetTick(); 

    // --- 计算通道 1 (TIM2, 32位防溢出) ---
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
    
    if (current_tick - ic1_last_tick > 1000) {
        sys.freq_ch1 = 0; sys.period_ch1 = 0; sys.duty_ch1 = 0.0f;
        ic1_state = 0; 
        __HAL_TIM_SET_CAPTUREPOLARITY(&htim2, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
    }
    
    // --- 计算通道 2 (TIM3, 16位防溢出) ---
    if (ic2_state == 3) {
        uint32_t period_ticks = (ic2_val3 >= ic2_val1) ? (ic2_val3 - ic2_val1) : ((0xFFFF - ic2_val1) + ic2_val3 + 1);
        uint32_t high_ticks   = (ic2_val2 >= ic2_val1) ? (ic2_val2 - ic2_val1) : ((0xFFFF - ic2_val1) + ic2_val2 + 1);
        
        if (period_ticks > 0) {
            sys.period_ch2 = period_ticks; 
            sys.freq_ch2 = 1000000 / period_ticks;
            sys.duty_ch2 = ((float)high_ticks / (float)period_ticks) * 100.0f;
        }
        ic2_state = 0; 
    }
    
    if (current_tick - ic2_last_tick > 1000) {
        sys.freq_ch2 = 0; sys.period_ch2 = 0; sys.duty_ch2 = 0.0f;
        ic2_state = 0; 
        __HAL_TIM_SET_CAPTUREPOLARITY(&htim3, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
    }
}
#endif
