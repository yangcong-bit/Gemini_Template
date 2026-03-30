// freq_app.h
#ifndef __FREQ_APP_H
#define __FREQ_APP_H

#include "main.h"

#ifdef FREQ_CH1_HW_Pin
    #define USE_HW_PWM_CH1 1  
#else
    #define USE_HW_PWM_CH1 0  
#endif

#ifdef FREQ_CH2_HW_Pin
    #define USE_HW_PWM_CH2 1  
#else
    #define USE_HW_PWM_CH2 0  
#endif

void Freq_Init(void);
void Freq_Proc(void);

#endif
