/* data_app.c */
#include "data_app.h"
#include "adc_app.h" 
#include "rtc_app.h" // 引入 RTC 头文件以调用 rtc_proc

/**
 * @brief  数据中心处理任务 (50ms 周期)
 */
void Data_Proc(void) {
    // 1. 同步 ADC 换算结果到系统全局字典
		adc_proc(); // <-- 必须加这句，先计算再赋值
    sys.r38_voltage = adc_value[0];
    sys.r37_voltage = adc_value[1];
    
    // 2. 硬件联动：用 R38 电位器的电压动态控制 PWM 输出频率
    // 当 R38 从 0V 拧到 3.3V 时，PWM 频率将在 1000Hz 到 4300Hz 之间无级变化
    sys.pwm_freq = 1000 + (uint32_t)(sys.r38_voltage * 1000);
    
    // 3. 同步底层 RTC 时钟数据到全局 (供 LCD 显示)
    rtc_proc();
}
