/* rtc_app.h */
#ifndef __RTC_APP_H__
#define __RTC_APP_H__

#include "main.h"

// 暴露给外部的读取任务和设置接口
void rtc_proc(void);
void rtc_set_time(uint8_t set_hour, uint8_t set_min, uint8_t set_sec);

#endif /* __RTC_APP_H__ */
