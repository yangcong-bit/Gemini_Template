/**
 * @file    adc_app.c
 * @brief   ADC DMA 底层驱动与中位值平均滤波算法实现
 */
#include "adc_app.h"
#include "adc.h"           // 引入 hadc1 和 hadc2 硬件句柄
#include "global_system.h" // 引入全局字典，用于上报最终电压

/* ==========================================
 * 私有内存区 (严禁外部 extern)
 * ========================================== */
/** * @brief ADC DMA 双缓冲底层接收数组 
 * [0][x] 对应 ADC1_IN11 (板载 R38 电位器)
 * [1][x] 对应 ADC2_IN15 (板载 R37 电位器)
 */
static uint16_t dma_buff[2][30];

/**
 * @brief  ADC 与 DMA 底层启动
 * @note   必须在 main 的 while(1) 之前调用
 */
void ADC_Init(void) {
    // 1. 硬件自校准 (极大提高精度，比赛必加)
    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
    HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED);
    
    // 2. 启动 ADC 连续 DMA 搬运 (硬件自动将数据搬运到 dma_buff)
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)dma_buff[0], 30);
    HAL_ADC_Start_DMA(&hadc2, (uint32_t*)dma_buff[1], 30);
}

/**
 * @brief  ADC 数据处理任务 (去极值平均滤波)
 * @note   建议调度周期：50ms。处理完毕后直接将计算出的物理电压上报给 sys 字典。
 */
void adc_proc(void) {
    uint32_t sum_adc1 = 0, sum_adc2 = 0;
    uint16_t max1 = 0, min1 = 4095;
    uint16_t max2 = 0, min2 = 4095;

    // 1. 遍历 30 个采样点，累加并找出最大最小值
    for(uint8_t i = 0; i < 30; i++) {
        // --- 统计通道 1 (R38) ---
        if(dma_buff[0][i] > max1) max1 = dma_buff[0][i];
        if(dma_buff[0][i] < min1) min1 = dma_buff[0][i];
        sum_adc1 += dma_buff[0][i];

        // --- 统计通道 2 (R37) ---
        if(dma_buff[1][i] > max2) max2 = dma_buff[1][i];
        if(dma_buff[1][i] < min2) min2 = dma_buff[1][i];
        sum_adc2 += dma_buff[1][i];
    }

    // 2. 算法核心：去头去尾求平均 (剥离突变噪点，余下 28 个有效点)
    sum_adc1 = sum_adc1 - max1 - min1;
    sum_adc2 = sum_adc2 - max2 - min2;

    // 3. 换算为真实电压并直接同步至全局字典
    // 计算公式: (平均采样值 / 满量程分辨率 4096.0) * 基准电压 3.3V
    sys.r38_voltage = (sum_adc1 / 28.0f) * 3.3f / 4096.0f; 
    sys.r37_voltage = (sum_adc2 / 28.0f) * 3.3f / 4096.0f; 
}
