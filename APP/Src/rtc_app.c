/**
 * @file    rtc_app.c
 * @brief   RTC 影子寄存器机制处理与时间同步
 */
#include "rtc_app.h"
#include "rtc.h"           // 引入 hrtc 硬件句柄
#include "global_system.h" // 引入全局字典用于向外暴露解耦后的时间

/* 彻底将 HAL 库底层的硬件结构体私有化封装，切断外部的非法 extern 访问 */
static RTC_TimeTypeDef time; 
static RTC_DateTypeDef date; 

/**
 * @brief  RTC 时间读取与字典同步任务
 * @note   建议调度周期：100ms。
 * 【影子寄存器死锁防坑指南】：
 * STM32 的 RTC 采用影子寄存器机制以保证一致性。每次读取时，
 * 必须严格遵守【先读 Time，再读 Date】的顺序！
 * 即使你的 UI 根本不显示日期，你也必须把 HAL_RTC_GetDate() 写上，
 * 否则影子寄存器被永久锁死，读出的时间将停留在同一秒！
 */
void rtc_proc(void) {
    // 1. 获取当前时间 (选用 RTC_FORMAT_BIN 纯数字 10进制格式)
    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    
    // 2. 获取当前日期 (核心解锁步骤)
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
    
    // 3. 将剥离出来的数字，安全同步至全局字典
    sys.hour = time.Hours;
    sys.min  = time.Minutes;
    sys.sec  = time.Seconds;
}

/**
 * @brief  强行校准重置 RTC 时间 (外部业务 API)
 * @param  set_hour [0~23]
 * @param  set_min  [0~59]
 * @param  set_sec  [0~59]
 */
void rtc_set_time(uint8_t set_hour, uint8_t set_min, uint8_t set_sec) {
    RTC_TimeTypeDef sTime = {0};
    
    sTime.Hours   = set_hour;
    sTime.Minutes = set_min;
    sTime.Seconds = set_sec;
    
    // 蓝桥杯不考夏令时，关闭之，并重置备份寄存器存储操作
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    
    // 覆写进底层硬件
    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN); 
}
