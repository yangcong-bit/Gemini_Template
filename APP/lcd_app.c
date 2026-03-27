/* lcd_app.c */
#include "lcd_app.h"
#include "rtc_app.h" // 引入以获取全局变量 time
#include <stdio.h>

extern RTC_TimeTypeDef time; // 声明外部的 RTC 时间结构体

/**
 * @brief  UI 界面初始化
 * @note   在 main 函数进入 while(1) 之前调用
 */
void UI_Init(void) {
    LCD_Init();                 // 官方底层初始化
    LCD_Clear(Black);           // 清屏，背景色全黑
    LCD_SetBackColor(Black);    // 设置字符背景色为黑
    LCD_SetTextColor(White);    // 设置字符颜色为白
}

/**
 * @brief  UI 页面刷新任务
 * @note   由调度器定时调用 (建议 100ms 刷新一次，10FPS 对人眼已足够流畅且不占CPU)
 */
void UI_Proc(void) {
    char lcd_buf[21];

    // ==========================================
    // 1. 消费 UI 专属的按键信箱
    // ==========================================
    if (sys.key_event_ui != 0) {
        switch (sys.key_event_ui) {
            case 1: // 【B1 短按】：切换页面
                sys.current_page = (sys.current_page == PAGE_DATA) ? PAGE_PARA : PAGE_DATA;
                LCD_Clear(Black); 
                break;
                
            case 2: // 【B2 短按】：在参数页增加电压报警阈值
                if (sys.current_page == PAGE_PARA) {
                    sys.v_threshold += 0.5f;
                    if (sys.v_threshold > 3.3f) sys.v_threshold = 0.0f;
                }
                break;
                
            case 3: // 【B3 短按】：在参数页增加 PWM 占空比
                if (sys.current_page == PAGE_PARA) {
                    sys.pwm_duty += 0.1f;
                    if (sys.pwm_duty > 1.0f) sys.pwm_duty = 0.1f;
                }
                break;
                
            case 4: // 【B4 短按】：主动请求保存阈值参数到 EEPROM
                sys.eeprom_save_flag = true;
                break;
                
            case 21: // 【B1 双击】：调节 MCP4017 可编程电阻档位 (+10步进)
                sys.res_step += 10;
                if (sys.res_step > 127) sys.res_step = 0;
                break;
                
            case 11: // 【B1 长按】：一键恢复所有参数的默认值
                sys.v_threshold = 2.5f;
                sys.pwm_duty = 0.5f;
                sys.res_step = 64;
                sys.eeprom_save_flag = true; // 顺便存入芯片
                break;
        }
        
        // 安全清空 UI 信箱
        sys.key_event_ui = 0; 
    }
    
    // ==========================================
    // 2. 屏幕动态刷新逻辑 (100ms 一次，不闪屏)
    // ==========================================
    if (sys.current_page == PAGE_DATA) {
        // --- 数据监控页 ---
        sprintf(lcd_buf, "      DATA PAGE     ");
        LCD_DisplayStringLine(Line1, (uint8_t*)lcd_buf);
        
        // 显示 RTC 实时时间
        sprintf(lcd_buf, " Time: %02d:%02d:%02d   ", time.Hours, time.Minutes, time.Seconds);
        LCD_DisplayStringLine(Line3, (uint8_t*)lcd_buf);
        
        // 显示双路 ADC 电压
        sprintf(lcd_buf, " V37:%.2fV V38:%.2fV", sys.r37_voltage, sys.r38_voltage);
        LCD_DisplayStringLine(Line5, (uint8_t*)lcd_buf);
        
        // 显示双路测频 (-5d表示左对齐占5位，防止高位数字残留)
        sprintf(lcd_buf, " F1:%-5d F2:%-5d ", sys.freq_ch1, sys.freq_ch2);
        LCD_DisplayStringLine(Line7, (uint8_t*)lcd_buf);
        
    } else {
        // --- 参数设置页 ---
        sprintf(lcd_buf, "      PARA PAGE     ");
        LCD_DisplayStringLine(Line1, (uint8_t*)lcd_buf);
        
        sprintf(lcd_buf, " V_Thr: %.2f V      ", sys.v_threshold);
        LCD_DisplayStringLine(Line3, (uint8_t*)lcd_buf);
        
        sprintf(lcd_buf, " PWM Duty: %3.0f%%   ", sys.pwm_duty * 100.0f);
        LCD_DisplayStringLine(Line5, (uint8_t*)lcd_buf);
        
        sprintf(lcd_buf, " Res Step: %-3d      ", sys.res_step);
        LCD_DisplayStringLine(Line7, (uint8_t*)lcd_buf);
    }
}
