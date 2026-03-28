/* adc_app.c */
#include "adc_app.h"
#include "adc.h" // 引入 hadc1 和 hadc2 句柄

/* * ADC DMA 底层接收二维缓存数组
 * 维度 1: 代表两个通道 (0 对应 ADC1/R38, 1 对应 ADC2/R37)
 * 维度 2: 代表每个通道连续采样 30 次
 */
uint16_t dma_buff[2][30];

/* * 最终计算得出的实际电压值数组
 * adc_value[0]: R38 电位器的电压 (0 ~ 3.3V)
 * adc_value[1]: R37 电位器的电压 (0 ~ 3.3V)
 */
float adc_value[2];

/**
 * @brief ADC 与 DMA 底层启动
 * @note  必须在 main 的 while(1) 之前调用
 */
void ADC_Init(void) {
	
		HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
    HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED);
	
    // 启动 ADC1 (对应 R38)，将数据搬运到 dma_buff[0]
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)dma_buff[0], 30);
    
    // 启动 ADC2 (对应 R37)，将数据搬运到 dma_buff[1]
    HAL_ADC_Start_DMA(&hadc2, (uint32_t*)dma_buff[1], 30);
}

/**
 * @brief  ADC 数据处理任务 (带中位值平均滤波)
 */
void adc_proc(void) {
    uint32_t sum_adc1 = 0, sum_adc2 = 0;
    uint16_t max1 = 0, min1 = 4095;
    uint16_t max2 = 0, min2 = 4095;

    // 1. 遍历并找出最大最小值
    for(uint8_t i = 0; i < 30; i++) {
        // 通道 1 (R38)
        if(dma_buff[0][i] > max1) max1 = dma_buff[0][i];
        if(dma_buff[0][i] < min1) min1 = dma_buff[0][i];
        sum_adc1 += dma_buff[0][i];

        // 通道 2 (R37)
        if(dma_buff[1][i] > max2) max2 = dma_buff[1][i];
        if(dma_buff[1][i] < min2) min2 = dma_buff[1][i];
        sum_adc2 += dma_buff[1][i];
    }

    // 2. 去头去尾求平均 (30 - 2 = 28)
    sum_adc1 = sum_adc1 - max1 - min1;
    sum_adc2 = sum_adc2 - max2 - min2;

    // 3. 换算为电压 (基准 3.3V, 12位精度 4096)
    adc_value[0] = (sum_adc1 / 28.0f) * 3.3f / 4096.0f; 
    adc_value[1] = (sum_adc2 / 28.0f) * 3.3f / 4096.0f; 
}
