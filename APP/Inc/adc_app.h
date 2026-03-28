/**
 * @file    adc_app.h
 * @brief   双通道 ADC 采集与滤波模块
 * @note    所有 DMA 缓存与计算过程彻底私有化，外部只需调用初始化与处理函数，
 * 最终电压结果会自动同步至 global_system.h 的 sys 字典中。
 */
#ifndef __ADC_APP_H__
#define __ADC_APP_H__

#include "main.h"

void ADC_Init(void);
void adc_proc(void);

#endif /* __ADC_APP_H__ */
