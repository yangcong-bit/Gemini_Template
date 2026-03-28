/**
 * @file    tim_app.c
 * @brief   防抖与防毛刺的底层 PWM 刷新驱动
 */
#include "tim_app.h"
#include "tim.h"           // 引入 htim1 句柄
#include "global_system.h" // 引入全局数据字典

/**
 * @brief  [私有] PWM 底层寄存器刷新函数 (含核心数学运算与防跑飞逻辑)
 * @param  freq 目标频率 (Hz)
 * @param  duty 占空比 (0.0 ~ 1.0)
 */
static void PWM_Set_Params(uint32_t freq, float duty) {
    if (freq == 0) return; // 除零保护
    if (duty < 0.0f) duty = 0.0f;
    if (duty > 1.0f) duty = 1.0f;

    // 1. 基于定时器时钟 1MHz 计算自动重装载寄存器(ARR)和捕获比较寄存器(CCR)
    uint32_t arr_value = (1000000 / freq) - 1; 
    uint32_t ccr_value = (uint32_t)((arr_value + 1) * duty);

    // 2. 【防毛刺核心逻辑】
    // 比赛高频 Bug：如果频率突变导致新的 ARR 小于当前计数器 CNT 的值，
    // 定时器会一直数到溢出 (0xFFFF) 才反转，导致输出几百毫秒的异常电平。
    if (__HAL_TIM_GET_COUNTER(&htim1) > arr_value) {
        __HAL_TIM_SET_COUNTER(&htim1, 0); // 强制归零，防止跑飞
    }

    // 3. 写入底层寄存器 (立即生效)
    __HAL_TIM_SET_AUTORELOAD(&htim1, arr_value);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr_value); 
}

/**
 * @brief  定时器初始化与开机赋初值
 */
void TIM_PWM_Init(void) {
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    // 使用字典中的默认值立刻初始化硬件输出
    PWM_Set_Params(sys.pwm_freq, sys.pwm_duty);
}

/**
 * @brief  PWM 业务处理与同步任务 (消费者)
 * @note   建议调度周期：20ms。对比历史状态，仅在数值变化时操作寄存器。
 */
void TIM_Proc(void) {
    static uint32_t last_freq = 0;
    static float    last_duty = -1.0f;
    
    // 检查字典中的指令参数是否被 UI 或其他逻辑修改过
    if (sys.pwm_freq != last_freq || sys.pwm_duty != last_duty) {
        PWM_Set_Params(sys.pwm_freq, sys.pwm_duty);
        last_freq = sys.pwm_freq;
        last_duty = sys.pwm_duty;
    }
}
