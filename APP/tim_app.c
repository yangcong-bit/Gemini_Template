/* tim_app.c */
#include "tim_app.h"
#include "tim.h"  // 引入 htim1 等定时器句柄

/**
 * @brief  PWM 底层刷新函数 (核心数学运算与防跑飞逻辑)
 * @param  freq: 目标频率 (Hz)
 * @param  duty: 占空比 (0.0 ~ 1.0)
 */
static void PWM_Set_Params(uint32_t freq, float duty) {
    if (freq == 0) return; 
    if (duty < 0.0f) duty = 0.0f;
    if (duty > 1.0f) duty = 1.0f;

    // 1. 计算 ARR 和 CCR (基于定时器时钟 1MHz)
    uint32_t arr_value = (1000000 / freq) - 1; 
    uint32_t ccr_value = (uint32_t)((arr_value + 1) * duty);

    // 2. 防毛刺核心逻辑 (针对 TIM1)
    if (__HAL_TIM_GET_COUNTER(&htim1) > arr_value) {
        __HAL_TIM_SET_COUNTER(&htim1, 0); 
    }

    // 3. 写入底层寄存器 (立即生效)
    __HAL_TIM_SET_AUTORELOAD(&htim1, arr_value);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr_value); // 改为 TIM1 的 Channel 1
}

/**
 * @brief  定时器初始化
 */
void TIM_PWM_Init(void) {
    // 开启定时器 1 的通道 1 PWM 输出
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    
    // 初始化时立刻应用一次字典里的默认值
    PWM_Set_Params(sys.pwm_freq, sys.pwm_duty);
}

/**
 * @brief  PWM 业务处理任务
 * @note   建议调度周期：20ms
 */
void TIM_Proc(void) {
    static uint32_t last_freq = 0;
    static float    last_duty = -1.0f;
    
    if (sys.pwm_freq != last_freq || sys.pwm_duty != last_duty) {
        PWM_Set_Params(sys.pwm_freq, sys.pwm_duty);
        last_freq = sys.pwm_freq;
        last_duty = sys.pwm_duty;
    }
}
