/* lcd_app.c */
#include "lcd_app.h"
#include "rtc_app.h" 
#include "key_app.h"
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
    uint8_t key_val = 0;
		char temp[32]; // 增加一个暂存数组

    // ==========================================
    // 1. 从消息队列中消费所有积压的按键事件
    // ==========================================
    while (Key_Get_Event(&key_val)) {
        switch (key_val) {
            case 1: // 【B1 短按】
                sys.current_page = (sys.current_page == PAGE_DATA) ? PAGE_PARA : PAGE_DATA;
                memset(lcd_vram_bak, 0, sizeof(lcd_vram_bak)); // 如果你用了双缓存VRAM
                break;
                
            case 2: // 【B2 短按】
                if (sys.current_page == PAGE_PARA) {
                    sys.v_threshold += 0.5f;
                    if (sys.v_threshold > 3.3f) sys.v_threshold = 0.0f;
                }
                break;
                
            case 3: // 【B3 短按】
                if (sys.current_page == PAGE_PARA) {
                    sys.pwm_duty += 0.1f;
                    if (sys.pwm_duty > 1.0f) sys.pwm_duty = 0.1f;
                }
                break;
                
            case 4: // 【B4 短按】
                sys.eeprom_save_flag = true;
                break;
                
            case 21: // 【B1 双击】
                sys.res_step += 10;
                if (sys.res_step > 127) sys.res_step = 0;
                break;
                
            case 11: // 【B1 长按】
                sys.v_threshold = 2.5f;
                sys.pwm_duty = 0.5f;
                sys.res_step = 64;
                sys.eeprom_save_flag = true; 
                break;
        }
        
        // 如果还需要联动蜂鸣器或LED响一下，可以在这里统一处理：
        // Beep_Tick(); 
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
        sprintf(temp, "      DATA PAGE");
        sprintf(lcd_vram[1], "%-20s", temp); // %-20s 保证必定占满 20 格
        
        sprintf(temp, " Time: %02d:%02d:%02d", time.Hours, time.Minutes, time.Seconds);
        sprintf(lcd_vram[3], "%-20s", temp);
        
        sprintf(temp, " V37:%.2fV V38:%.2fV", sys.r37_voltage, sys.r38_voltage);
        sprintf(lcd_vram[5], "%-20s", temp);
        
        sprintf(temp, " F1:%-5d F2:%-5d", sys.freq_ch1, sys.freq_ch2);
        sprintf(lcd_vram[7], "%-20s", temp);
        
    } else {
        sprintf(temp, "      PARA PAGE");
        sprintf(lcd_vram[1], "%-20s", temp);
        
        sprintf(temp, " V_Thr: %.2f V", sys.v_threshold);
        sprintf(lcd_vram[3], "%-20s", temp);
        
        sprintf(temp, " PWM Duty: %3.0f%%", sys.pwm_duty * 100.0f);
        sprintf(lcd_vram[5], "%-20s", temp);
        
        sprintf(temp, " Res Step: %-3d", sys.res_step);
        sprintf(lcd_vram[7], "%-20s", temp);
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
