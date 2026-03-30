// tim_app.c
#include "tim_app.h"
#include "tim.h"           
#include "global_system.h" 

static void PWM_Set_Params(uint32_t freq, float duty) {
    if (freq == 0) return; 
    if (duty < 0.0f) duty = 0.0f;
    if (duty > 1.0f) duty = 1.0f;

    uint32_t arr_value = (1000000 / freq) - 1; 
    uint32_t ccr_value = (uint32_t)((arr_value + 1) * duty);

    if (__HAL_TIM_GET_COUNTER(&htim1) > arr_value) {
        __HAL_TIM_SET_COUNTER(&htim1, 0); 
    }

    __HAL_TIM_SET_AUTORELOAD(&htim1, arr_value);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr_value); 
}

void TIM_PWM_Init(void) {
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

    PWM_Set_Params(sys.pwm_freq, sys.pwm_duty);
}

void TIM_Proc(void) {
    static uint32_t last_freq = 0;
    static float    last_duty = -1.0f;

    if (sys.pwm_freq != last_freq || sys.pwm_duty != last_duty) {
        PWM_Set_Params(sys.pwm_freq, sys.pwm_duty);
        last_freq = sys.pwm_freq;
        last_duty = sys.pwm_duty;
    }
}
