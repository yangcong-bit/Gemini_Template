/* rtc_app.c */
#include "rtc_app.h"
#include "rtc.h" // 引入 hrtc 硬件句柄

// 定义全局的时间和日期结构体
// 其他文件如果需要读取，可以通过包含 bsp_system.h (里面写了 extern) 来直接使用
RTC_TimeTypeDef time; 
RTC_DateTypeDef date; 

/**
 * @brief  RTC 时间读取任务 (数据更新)
 * @note   建议在调度器中注册为 100ms 或 200ms 周期运行即可，不需要太快。
 * * 【?? 避坑指南】：
 * STM32 的 RTC 具有影子寄存器机制。每次读取时，必须严格遵守先读 Time、
 * 再读 Date 的顺序！即使你的 UI 界面上根本不需要显示日期，你也必须把
 * HAL_RTC_GetDate 这句话写上。否则影子寄存器被锁死，读出的时间将永远停住！
 */
void rtc_proc(void)
{
    // 1. 获取当前时间
    // 格式选用 RTC_FORMAT_BIN (纯数字格式)，这样读出来的 time.Hours 就是 12，
    // 方便直接参与加减运算和 sprintf("%d") 格式化打印。如果是 BCD 格式还得转换。
    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    
    // 2. 获取当前日期 (必须紧跟在 GetTime 之后调用，用于解锁寄存器)
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
}

/**
 * @brief  更新 RTC 时间 (供外部业务逻辑调用)
 * @param  set_hour: 设置的小时 (0~23)
 * @param  set_min:  设置的分钟 (0~59)
 * @param  set_sec:  设置的秒   (0~59)
 * * @note   比赛常考点：在参数设置界面通过按键 B2、B3 修改时和分。
 * 修改完毕后（比如按下 B4 确认退出时），调用此函数将新时间写入底层硬件。
 */
void rtc_set_time(uint8_t set_hour, uint8_t set_min, uint8_t set_sec)
{
    RTC_TimeTypeDef sTime = {0};
    
    // 填充要修改的时间参数
    sTime.Hours   = set_hour;
    sTime.Minutes = set_min;
    sTime.Seconds = set_sec;
    
    // 蓝桥杯一般不考夏令时，直接设为 NONE 关闭
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    
    // 调用底层 HAL 库将新时间写入 RTC 硬件 (同样使用 BIN 格式传入)
    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN); 
}
