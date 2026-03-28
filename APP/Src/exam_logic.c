/**
 * @file    exam_logic.c
 * @brief   考场业务逻辑总览 (智能环境监测与报警系统)
 */

#include "exam_logic.h"
#include "global_system.h" 

// --- 引入底层对外暴露的 API ---
#include "key_app.h"
#include "led_app.h"
#include "lcd.h"
#include "uart_app.h"
#include "eeprom_app.h"
#include "adc_app.h"
#include "rtc_app.h"
#include <stdio.h>
#include <string.h>

/* ==========================================================
 * [模块 1]：全局数据采集与联动中枢
 * @note   调度周期：50ms。负责搬运底层数据到 sys 字典，并处理核心逻辑。
 * ========================================================== */
void Logic_Data_Proc(void) {
    // 1. 唤醒采集层，将最新的 ADC 电压和 RTC 时间更新到 sys 字典
    adc_proc(); 
    rtc_proc();

    // 2. 响应 B4 长按触发的 EEPROM 保存请求
    if (sys.eeprom_save_flag == true) {
        LogData_t new_log = {
            .hour = sys.hour,
            .min  = sys.min,
            .sec  = sys.sec,
            .volt = sys.v_threshold, // 保存当前的报警阈值
        };
        // 压入后台异步存储队列，5ms切片慢慢写，绝不卡顿
        EEPROM_PushLog(new_log); 
        sys.eeprom_save_flag = false; // 清除请求标志
    }
}

/* ==========================================================
 * [模块 2]：LED 指示灯业务映射
 * @note   调度周期：20ms。根据 sys 字典的状态，决定哪个灯亮。
 * ========================================================== */
void Logic_LED_Proc(void) {
    // 1. 擦除上一帧状态
    for(int i = 0; i < 8; i++) sys.led_ctrl[i] = 0;
    
    // 2. 页面指示灯逻辑 (LD1/LD2)
    if (sys.current_page == PAGE_DATA) {
        sys.led_ctrl[0] = 1; 
    } else if (sys.current_page == PAGE_PARA) {
        sys.led_ctrl[1] = 1; 
    }
    
    // 3. 越限报警灯逻辑 (LD8 5Hz 闪烁)
    // 算法：利用系统滴答，对 200 取余。前 100ms 亮，后 100ms 灭
    if (sys.r37_voltage > sys.v_threshold) {
        if (HAL_GetTick() % 200 < 100) {
            sys.led_ctrl[7] = 1; 
        }
    }
    
    // 4. 提交到底层锁存器刷新硬件
    LED_Disp();
}

/* ==========================================================
 * [模块 3]：串口协议解析 (本题未要求，留空即可)
 * @note   调度周期：10ms
 * ========================================================== */
void Logic_UART_Proc(void) {
    if (sys.uart_rx_ready == true) {
        // 如果题目要求串口控制，在这里写 sscanf 解析逻辑
        sys.uart_rx_ready = false; 
    }
}

/* ==========================================================
 * [模块 4]：LCD 屏幕渲染与按键交互
 * @note   调度周期：100ms (10FPS)
 * ========================================================== */
static char lcd_vram[10][21];      // 当前帧工作显存
static char lcd_vram_bak[10][21];  // 历史帧备份显存 (防闪屏核心)

void Logic_UI_Proc(void) {
    uint8_t key_val = 0;
    char temp[32]; 

    /* ----------------------------------
     * 环节 A：按键交互控制 (改变 sys 变量)
     * ---------------------------------- */
    while (Key_Get_Event(&key_val)) { // 从队列把积压的按键全部消费完
        switch (key_val) {
            case 1: // 【B1 短按】：切换页面
                sys.current_page = (sys.current_page == PAGE_DATA) ? PAGE_PARA : PAGE_DATA;
                memset(lcd_vram_bak, 0, sizeof(lcd_vram_bak)); // 换页必须清空备份以触发全刷
                break;
                
            case 2: // 【B2 短按】：在设置页增加阈值
                if (sys.current_page == PAGE_PARA) {
                    sys.v_threshold += 0.5f;
                    if (sys.v_threshold > 3.3f) sys.v_threshold = 0.0f;
                }
                break;
                
            case 14: // 【B4 长按】：触发保存 (键值 4 + 长按偏移 10 = 14)
                sys.eeprom_save_flag = true; 
                break;
        }
    }
    
    /* ----------------------------------
     * 环节 B：擦除画布，准备绘制新的一帧
     * ---------------------------------- */
    for(int i = 0; i < 10; i++) {
        sprintf(lcd_vram[i], "                    "); 
    }

    /* ----------------------------------
     * 环节 C：将 sys 字典变量格式化进显存
     * ---------------------------------- */
    if (sys.current_page == PAGE_DATA) {
        sprintf(temp, "   MONITOR PAGE");
        sprintf(lcd_vram[1], "%-20s", temp); 
        
        sprintf(temp, " Time: %02d:%02d:%02d", sys.hour, sys.min, sys.sec);
        sprintf(lcd_vram[3], "%-20s", temp);
        
        sprintf(temp, " Volt: %.2f V", sys.r37_voltage);
        sprintf(lcd_vram[5], "%-20s", temp);
        
        sprintf(temp, " Alarm Thr: %.1f V", sys.v_threshold);
        sprintf(lcd_vram[7], "%-20s", temp);
        
    } else if (sys.current_page == PAGE_PARA) {
        sprintf(temp, "   PARA SETTING");
        sprintf(lcd_vram[1], "%-20s", temp);
        
        sprintf(temp, " V_Thr: %.1f V", sys.v_threshold);
        sprintf(lcd_vram[4], "%-20s", temp);
    }else if(sys.current_page == PAGE_LED){
			
			sprintf(temp, "   PARA SETTING");
        sprintf(lcd_vram[1], "%-20s", temp);
        
        sprintf(temp, " V_Thr: %.1f V", sys.v_threshold);
        sprintf(lcd_vram[4], "%-20s", temp);
		
		
		}

    /* ----------------------------------
     * 环节 D：对比备份显存，差异化刷新底层硬件
     * ---------------------------------- */
    for(uint8_t i = 0; i < 10; i++) {
        if (strcmp(lcd_vram[i], lcd_vram_bak[i]) != 0) {
            LCD_DisplayStringLine(i * 24, (uint8_t *)lcd_vram[i]); 
            strcpy(lcd_vram_bak[i], lcd_vram[i]);
        }
    }
}
