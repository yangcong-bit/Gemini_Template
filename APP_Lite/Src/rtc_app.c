// rtc_app.c
#include "rtc_app.h"
#include "rtc.h"           
#include "global_system.h" 

static RTC_TimeTypeDef time; 
static RTC_DateTypeDef date; 

void rtc_proc(void) {

    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);

    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);

    sys.hour = time.Hours;
    sys.min  = time.Minutes;
    sys.sec  = time.Seconds;
}

void rtc_set_time(uint8_t set_hour, uint8_t set_min, uint8_t set_sec) {
    RTC_TimeTypeDef sTime = {0};

    sTime.Hours   = set_hour;
    sTime.Minutes = set_min;
    sTime.Seconds = set_sec;

    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN); 
}
