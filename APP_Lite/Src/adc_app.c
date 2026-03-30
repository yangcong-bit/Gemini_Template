// adc_app.c
#include "adc_app.h"
#include "adc.h"           
#include "global_system.h" 

static uint16_t dma_buff[2][30];

#define ADC_SAMPLE_COUNT 5

static uint16_t ADC_Sliding_Filter(uint8_t channel_id, uint16_t adc_raw_val) 
{
    static uint16_t buf[2][ADC_SAMPLE_COUNT] = {0}; 
    static uint8_t count[2] = {0};
    uint32_t sum = 0;
    uint16_t max = 0, min = 4095;

    buf[channel_id][count[channel_id]++] = adc_raw_val;
    if(count[channel_id] >= ADC_SAMPLE_COUNT) count[channel_id] = 0;

    for(int i = 0; i < ADC_SAMPLE_COUNT; i++) {
        sum += buf[channel_id][i];
        if(buf[channel_id][i] > max) max = buf[channel_id][i];
        if(buf[channel_id][i] < min) min = buf[channel_id][i];
    }

    sum = sum - max - min;
    return (uint16_t)(sum / (ADC_SAMPLE_COUNT - 2));
}

void ADC_Init(void) {

    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
    HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED);

    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)dma_buff[0], 30);
    HAL_ADC_Start_DMA(&hadc2, (uint32_t*)dma_buff[1], 30);
}

void adc_proc(void) {
    uint32_t sum_adc1 = 0, sum_adc2 = 0;
    uint16_t max1 = 0, min1 = 4095;
    uint16_t max2 = 0, min2 = 4095;

    for(uint8_t i = 0; i < 30; i++) {

        if(dma_buff[0][i] > max1) max1 = dma_buff[0][i];
        if(dma_buff[0][i] < min1) min1 = dma_buff[0][i];
        sum_adc1 += dma_buff[0][i];

        if(dma_buff[1][i] > max2) max2 = dma_buff[1][i];
        if(dma_buff[1][i] < min2) min2 = dma_buff[1][i];
        sum_adc2 += dma_buff[1][i];
    }

    uint16_t hw_raw_r38 = (sum_adc1 - max1 - min1) / 28;
    uint16_t hw_raw_r37 = (sum_adc2 - max2 - min2) / 28;

    uint16_t final_r38 = ADC_Sliding_Filter(0, hw_raw_r38);
    uint16_t final_r37 = ADC_Sliding_Filter(1, hw_raw_r37);

    sys.r38_voltage = (final_r38 / 4096.0f) * 3.3f; 
    sys.r37_voltage = (final_r37 / 4096.0f) * 3.3f; 
}
