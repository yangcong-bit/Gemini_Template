/* lcd_app.c */
#include "lcd_app.h"
#include "rtc_app.h" 
#include <stdio.h>
#include <string.h>

extern RTC_TimeTypeDef time; 

// ==========================================
// 虚拟显存定义区 (双缓存机制)
// ==========================================
// 蓝桥杯LCD尺寸：10行，每行20个字符。+1用于存放字符串结束符 '\0'
char lcd_vram[10][21];      // 当前准备显示的工作显存
char lcd_vram_bak[10][21];  // 上一次显示的备份显存 (用于比对防闪屏)

/**
 * @brief  UI 界面初始化
 */
void UI_Init(void) {
    LCD_Init();                 
    LCD_Clear(Black);           
    LCD_SetBackColor(Black);    
    LCD_SetTextColor(White);    
    
    // 清空显存
    memset(lcd_vram, 0, sizeof(lcd_vram));
    memset(lcd_vram_bak, 0, sizeof(lcd_vram_bak));
}

/**
 * @brief  UI 页面刷新任务 (100ms)
 */
void UI_Proc(void) {
    // ==========================================
    // 1. 消费 UI 专属的按键信箱
    // ==========================================
    if (sys.key_event_ui != 0) {
        switch (sys.key_event_ui) {
            case 1: // 切换页面
                sys.current_page = (sys.current_page == PAGE_DATA) ? PAGE_PARA : PAGE_DATA;
                // 注意：这里不再调用 LCD_Clear()，而是直接清空备份显存。
                // 这样下一帧局部刷新机制会认为全屏都变了，自动重绘全屏，完美无闪烁。
                memset(lcd_vram_bak, 0, sizeof(lcd_vram_bak)); 
                break;
                
            case 2: // 增加电压报警阈值
                if (sys.current_page == PAGE_PARA) {
                    sys.v_threshold += 0.5f;
                    if (sys.v_threshold > 3.3f) sys.v_threshold = 0.0f;
                }
                break;
                
            case 3: // 增加 PWM 占空比
                if (sys.current_page == PAGE_PARA) {
                    sys.pwm_duty += 0.1f;
                    if (sys.pwm_duty > 1.0f) sys.pwm_duty = 0.1f;
                }
                break;
                
            case 4: // 请求保存阈值参数到 EEPROM
                sys.eeprom_save_flag = true;
                break;
                
            case 21: // 调节 MCP4017 可编程电阻档位
                sys.res_step += 10;
                if (sys.res_step > 127) sys.res_step = 0;
                break;
                
            case 11: // 一键恢复所有参数的默认值
                sys.v_threshold = 2.5f;
                sys.pwm_duty = 0.5f;
                sys.res_step = 64;
                sys.eeprom_save_flag = true; 
                break;
        }
        sys.key_event_ui = 0; 
    }
    
    // ==========================================
    // 2. 擦除当前显存工作区 (填充空格)
    // ==========================================
    // 这一步是为了防止短字符串覆盖不全旧的长字符串带来的残影
    for(int i = 0; i < 10; i++) {
        sprintf(lcd_vram[i], "                    "); 
    }

    // ==========================================
    // 3. 根据业务逻辑，将数据“画”入虚拟显存
    // ==========================================
    if (sys.current_page == PAGE_DATA) {
        sprintf(lcd_vram[1], "      DATA PAGE     ");
        sprintf(lcd_vram[3], " Time: %02d:%02d:%02d   ", time.Hours, time.Minutes, time.Seconds);
        sprintf(lcd_vram[5], " V37:%.2fV V38:%.2fV", sys.r37_voltage, sys.r38_voltage);
        sprintf(lcd_vram[7], " F1:%-5d F2:%-5d ", sys.freq_ch1, sys.freq_ch2);
    } else {
        sprintf(lcd_vram[1], "      PARA PAGE     ");
        sprintf(lcd_vram[3], " V_Thr: %.2f V      ", sys.v_threshold);
        sprintf(lcd_vram[5], " PWM Duty: %3.0f%%   ", sys.pwm_duty * 100.0f);
        sprintf(lcd_vram[7], " Res Step: %-3d      ", sys.res_step);
    }

    // ==========================================
    // 4. 对比双缓存，底层硬件局部刷新 (VRAM 核心)
    // ==========================================
    for(uint8_t i = 0; i < 10; i++) {
        // 使用 strcmp 对比该行内容是否有变化
        if (strcmp(lcd_vram[i], lcd_vram_bak[i]) != 0) {
            
            // 如果有变化，则调用底层 LCD 驱动去刷这一行
            // LCD 每一行的高度是 24 像素，所以 Line 的坐标刚好是 i * 24
            LCD_DisplayStringLine(i * 24, (uint8_t *)lcd_vram[i]); 
            
            // 将工作显存的内容同步到备份显存
            strcpy(lcd_vram_bak[i], lcd_vram[i]);
        }
    }
}
