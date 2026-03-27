#include "freq_app.h"
#include "tim.h"

// --- 通道 1 (TIM2, 32位) 状态变量 ---
static uint32_t ic1_val1 = 0;
static uint32_t ic1_val2 = 0;
static uint8_t  ic1_state = 0; 
static uint32_t ic1_last_tick = 0; // 新增：记录最后一次捕获的时间戳

// --- 通道 2 (TIM3, 16位) 状态变量 ---
static uint32_t ic2_val1 = 0;
static uint32_t ic2_val2 = 0;
static uint8_t  ic2_state = 0;
static uint32_t ic2_last_tick = 0; // 新增：记录最后一次捕获的时间戳

void Freq_Init(void) {
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    // ---------------- 处理 TIM2 (Channel 1) ----------------
    if (htim->Instance == TIM2 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
        ic1_last_tick = HAL_GetTick(); // 更新有效时间戳
        if (ic1_state == 0) {
            ic1_val1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            ic1_state = 1; 
        } else if (ic1_state == 1) {
            ic1_val2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            ic1_state = 2; 
        }
    }
    
    // ---------------- 处理 TIM3 (Channel 1) ----------------
    if (htim->Instance == TIM3 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
        ic2_last_tick = HAL_GetTick(); // 更新有效时间戳
        if (ic2_state == 0) {
            ic2_val1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            ic2_state = 1; 
        } else if (ic2_state == 1) {
            ic2_val2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            ic2_state = 2; 
        }
    }
}

void Freq_Proc(void) {
    uint32_t current_tick = HAL_GetTick(); // 获取当前系统滴答

    // --- 计算通道 1 频率 ---
    if (ic1_state == 2) {
        uint32_t delta_time = 0;
        if (ic1_val2 >= ic1_val1) {
            delta_time = ic1_val2 - ic1_val1;
        } else {
            delta_time = (0xFFFFFFFF - ic1_val1) + ic1_val2 + 1;
        }
        
        if (delta_time > 0) sys.freq_ch1 = 1000000 / delta_time;
        ic1_state = 0; 
    }
    // 【新增】零频超时保护：超过 1000ms 没有捕获到新电平，强制归零
    if (current_tick - ic1_last_tick > 1000) {
        sys.freq_ch1 = 0;
        ic1_state = 0; // 防止状态机卡死
    }
    
    // --- 计算通道 2 频率 ---
    if (ic2_state == 2) {
        uint32_t delta_time = 0;
        if (ic2_val2 >= ic2_val1) {
            delta_time = ic2_val2 - ic2_val1;
        } else {
            delta_time = (0xFFFF - ic2_val1) + ic2_val2 + 1; 
        }
        
        if (delta_time > 0) sys.freq_ch2 = 1000000 / delta_time;
        ic2_state = 0; 
    }
    // 零频超时保护：超过 1000ms 没有捕获到新电平，强制归零
    if (current_tick - ic2_last_tick > 1000) {
        sys.freq_ch2 = 0;
        ic2_state = 0; // 防止状态机卡死
    }
}
