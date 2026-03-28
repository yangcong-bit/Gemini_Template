/* data_app.c */
#include "data_app.h"
#include "adc_app.h" 
#include "rtc_app.h" 
#include "eeprom_app.h" // 引入 EEPROM 接口

void Data_Proc(void) {
    // 1. 同步 ADC 换算结果到系统全局字典
    adc_proc(); 
    
    // 2. 硬件联动：R38 电压 (0~3.3V) 映射为 PWM 频率 (1000~4300Hz)
    sys.pwm_freq = 1000 + (uint32_t)(sys.r38_voltage * 1000);
    
    // 3. 同步底层 RTC 时钟数据到全局
    rtc_proc();

    // 4. 【测试例程粘合】：响应 B4 按键的 EEPROM 存储请求
    if (sys.eeprom_save_flag == true) {
        // 打包当前系统状态作为一条历史记录
        LogData_t new_log = {
            .hour = sys.hour,
            .min  = sys.min,
            .sec  = sys.sec,
            .volt = sys.v_threshold,
            .freq = sys.pwm_freq
        };
        // 压入环形队列，交由底层非阻塞状态机慢慢烧录
        EEPROM_PushLog(new_log); 
        
        sys.eeprom_save_flag = false; // 消耗完毕，清空标志位
    }
}
