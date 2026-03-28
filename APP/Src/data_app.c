/**
 * @file    data_app.c
 * @brief   数据更新与硬件联动映射的具体实现
 */
#include "data_app.h"
#include "adc_app.h"       // 提供 adc_proc() 接口
#include "rtc_app.h"       // 提供 rtc_proc() 接口
#include "eeprom_app.h"    // 提供 EEPROM_PushLog() 接口
#include "global_system.h" // 核心数据字典

/**
 * @brief  核心数据处理大本营任务
 * @note   建议调度周期：50ms。
 */
void Data_Proc(void) {
    /* ==========================================
     * 1. 采集层唤醒与数据入库
     * ========================================== */
    // 唤醒 ADC 滤波状态机，内部会自动将换算后的电压抛入 sys 字典
    adc_proc(); 
    
    // 唤醒 RTC 状态机，内部会自动将时间同步至 sys 字典
    rtc_proc();

    /* ==========================================
     * 2. 业务联动层：物理量映射
     * ========================================== */
    // 【考点范例】：R38 传感器电压 (0~3.3V) 线性映射为 PWM 频率 (1000~4300Hz)
    // 因为 adc_proc 已经将电压推送到字典，这里直接使用 sys.r38_voltage，杜绝了 extern 非法访问
    sys.pwm_freq = 1000 + (uint32_t)(sys.r38_voltage * 1000);

    /* ==========================================
     * 3. 异步事件层：响应持久化请求
     * ========================================== */
    // 检测 B4 按键触发的 EEPROM 保存请求标志位
    if (sys.eeprom_save_flag == true) {
        
        // 打包当前系统状态为一条快照日志
        LogData_t new_log = {
            .hour = sys.hour,
            .min  = sys.min,
            .sec  = sys.sec,
            .volt = sys.v_threshold,
            .freq = sys.pwm_freq
        };
        
        // 压入环形队列，交由底层非阻塞状态机(EEPROM_Proc)慢慢烧录
        EEPROM_PushLog(new_log); 
        
        sys.eeprom_save_flag = false; // 请求消耗完毕，必须清空标志位防死锁
    }
}
