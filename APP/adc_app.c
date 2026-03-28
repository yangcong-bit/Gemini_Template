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
 * @brief  ADC 数据处理任务
 * @note   该函数负责将 DMA 搬运来的 30 次底层生数据进行累加平均，
 * 并将其转换为实际的物理电压值。建议由调度器定时调用（如 50ms）。
 */
void adc_proc(void)
{
    // 定义局部变量用于累加求和，防止每次进入函数时受到上一次历史值的影响
    uint32_t sum_adc1 = 0;
    uint32_t sum_adc2 = 0;
    
    // 1. 遍历 DMA 缓存，将连续采样的 30 个数据累加起来，实现软件均值滤波
    for(uint8_t i = 0; i < 30; i++)
    {
        sum_adc1 += dma_buff[0][i]; // 累加 R38 (ADC1) 的原始数据
        sum_adc2 += dma_buff[1][i]; // 累加 R37 (ADC2) 的原始数据
    }

    /* * 2. 计算平均值并转换为实际电压
     * 转换公式说明：
     * - sum_adc1 / 30.0f  : 求出 30 次采样的平均值 (范围 0 ~ 4095)
     * - * 3.3f / 4096.0f  : STM32 的 ADC 是 12 位精度，即 2^12 = 4096，
     * 基准电压为 3.3V。按比例换算得到实际电压。
     */
    adc_value[0] = (sum_adc1 / 30.0f) * 3.3f / 4096.0f; // 算出 R38 电压
    adc_value[1] = (sum_adc2 / 30.0f) * 3.3f / 4096.0f; // 算出 R37 电压
    
    // （可选）如果你的系统使用了 global_system.h 字典架构，可以在这里同步更新字典：
    // sys.r38_voltage = adc_value[0];
    // sys.r37_voltage = adc_value[1];
}
