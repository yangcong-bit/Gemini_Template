// freq_app.c
#include "freq_app.h"
#include "tim.h"
#include "global_system.h" 

#if USE_HW_PWM_CH1 == 1
    static uint32_t ic1_period = 0, ic1_high = 0;
    static uint8_t  ic1_ready  = 0; 
#else
    static uint32_t ic1_val1 = 0, ic1_val2 = 0, ic1_val3 = 0;
    static uint8_t  ic1_state  = 0; 
#endif
static uint32_t ic1_last_tick = 0; 

#if USE_HW_PWM_CH2 == 1
    static uint32_t ic2_period = 0, ic2_high = 0;
    static uint8_t  ic2_ready  = 0;
#else
    static uint32_t ic2_val1 = 0, ic2_val2 = 0, ic2_val3 = 0;
    static uint8_t  ic2_state  = 0;
#endif
static uint32_t ic2_last_tick = 0; 

void Freq_Init(void) {

    HAL_TIM_IC_Start_IT(&FREQ_CH1_HANDLE, FREQ_CH1_CH_MAIN);
#if USE_HW_PWM_CH1 == 1
    HAL_TIM_IC_Start(&FREQ_CH1_HANDLE, FREQ_CH1_CH_SUB); 
#endif

    HAL_TIM_IC_Start_IT(&FREQ_CH2_HANDLE, FREQ_CH2_CH_MAIN);
#if USE_HW_PWM_CH2 == 1
    HAL_TIM_IC_Start(&FREQ_CH2_HANDLE, FREQ_CH2_CH_SUB); 
#endif
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {

    if (htim->Instance == FREQ_CH1_INST && htim->Channel == FREQ_CH1_ACTIVE) {
        ic1_last_tick = HAL_GetTick(); 

#if USE_HW_PWM_CH1 == 1
        ic1_period = HAL_TIM_ReadCapturedValue(htim, FREQ_CH1_CH_MAIN);
        ic1_high   = HAL_TIM_ReadCapturedValue(htim, FREQ_CH1_CH_SUB);
        ic1_ready  = 1;
#else

        if (ic1_state == 0) {
            ic1_val1 = HAL_TIM_ReadCapturedValue(htim, FREQ_CH1_CH_MAIN);
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, FREQ_CH1_CH_MAIN, TIM_INPUTCHANNELPOLARITY_FALLING);
            ic1_state = 1; 
        } else if (ic1_state == 1) {
            ic1_val2 = HAL_TIM_ReadCapturedValue(htim, FREQ_CH1_CH_MAIN);
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, FREQ_CH1_CH_MAIN, TIM_INPUTCHANNELPOLARITY_RISING);
            ic1_state = 2; 
        } else if (ic1_state == 2) {
            ic1_val3 = HAL_TIM_ReadCapturedValue(htim, FREQ_CH1_CH_MAIN);
            ic1_state = 3; 
        }
#endif
    }

    if (htim->Instance == FREQ_CH2_INST && htim->Channel == FREQ_CH2_ACTIVE) {
        ic2_last_tick = HAL_GetTick(); 

#if USE_HW_PWM_CH2 == 1
        ic2_period = HAL_TIM_ReadCapturedValue(htim, FREQ_CH2_CH_MAIN);
        ic2_high   = HAL_TIM_ReadCapturedValue(htim, FREQ_CH2_CH_SUB);
        ic2_ready  = 1;
#else
        if (ic2_state == 0) {
            ic2_val1 = HAL_TIM_ReadCapturedValue(htim, FREQ_CH2_CH_MAIN);
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, FREQ_CH2_CH_MAIN, TIM_INPUTCHANNELPOLARITY_FALLING);
            ic2_state = 1; 
        } else if (ic2_state == 1) {
            ic2_val2 = HAL_TIM_ReadCapturedValue(htim, FREQ_CH2_CH_MAIN);
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, FREQ_CH2_CH_MAIN, TIM_INPUTCHANNELPOLARITY_RISING);
            ic2_state = 2; 
        } else if (ic2_state == 2) {
            ic2_val3 = HAL_TIM_ReadCapturedValue(htim, FREQ_CH2_CH_MAIN);
            ic2_state = 3; 
        }
#endif
    }
}

void Freq_Proc(void) {
    uint32_t current_tick = HAL_GetTick();

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

    if (current_tick - ic1_last_tick > 1000) {
        sys.freq_ch1 = 0; sys.period_ch1 = 0; sys.duty_ch1 = 0.0f;
#if USE_HW_PWM_CH1 == 0
        ic1_state = 0; 
        __HAL_TIM_SET_CAPTUREPOLARITY(&FREQ_CH1_HANDLE, FREQ_CH1_CH_MAIN, TIM_INPUTCHANNELPOLARITY_RISING);
#endif
    }

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

        uint32_t period_ticks = (ic2_val3 >= ic2_val1) ? (ic2_val3 - ic2_val1) : ((0xFFFFFFFF - ic2_val1) + ic2_val3 + 1);
        uint32_t high_ticks   = (ic2_val2 >= ic2_val1) ? (ic2_val2 - ic2_val1) : ((0xFFFFFFFF - ic2_val1) + ic2_val2 + 1);

        if (period_ticks > 0) {
            sys.period_ch2 = period_ticks; 
            sys.freq_ch2 = 1000000 / period_ticks;
            sys.duty_ch2 = ((float)high_ticks / (float)period_ticks) * 100.0f;
        }
        ic2_state = 0; 
    }
#endif

    if (current_tick - ic2_last_tick > 1000) {
        sys.freq_ch2 = 0; sys.period_ch2 = 0; sys.duty_ch2 = 0.0f;
#if USE_HW_PWM_CH2 == 0
        ic2_state = 0; 
        __HAL_TIM_SET_CAPTUREPOLARITY(&FREQ_CH2_HANDLE, FREQ_CH2_CH_MAIN, TIM_INPUTCHANNELPOLARITY_RISING);
#endif
    }
}
