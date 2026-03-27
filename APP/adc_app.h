/* adc_app.h */
#ifndef __ADC_APP_H__
#define __ADC_APP_H__

#include "main.h"
// 如果你在 adc_app.c 中取消了对 sys 字典的注释，这里需要包含全局字典
// #include "global_system.h" 

/* =======================================================
 * 全局变量外部声明 (实体定义在 adc_app.c 中)
 * 供底层 DMA 驱动 (如 stm32g4xx_hal_msp.c) 或其他业务模块读取
 * ======================================================= */

/** * @brief ADC DMA 底层接收二维缓存数组 
 * [0][x] 对应 ADC1_IN11 (R38电位器)
 * [1][x] 对应 ADC2_IN15 (R37电位器)
 */
extern uint16_t dma_buff[2][30];

/** * @brief 换算后的实际电压值数组 (0 ~ 3.3V)
 * [0] 对应 R38
 * [1] 对应 R37
 */
extern float adc_value[2];

void ADC_Init(void); //初始化接口

/* =======================================================
 * 调度器任务函数声明
 * ======================================================= */

/**
 * @brief ADC 数据处理任务
 * @note  负责对 DMA 搬运的原始数据进行均值滤波并换算为电压。
 * 建议由 scheduler 定期调用（如 50ms 周期）。
 */
void adc_proc(void);




#endif /* __ADC_APP_H__ */
