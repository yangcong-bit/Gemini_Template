/**
 * @file    exam_logic.c
 * @brief   【考场唯一需要修改的文件】业务逻辑总览
 * @note    本文件将原本分散的 UI、LED、数据联动、串口解析集中于一处。
 * 考试时，只需盯着这个文件改，不要去碰底层驱动代码！
 */

#include "exam_logic.h"
#include "global_system.h" 

// --- 引入各底层模块对外暴露的 API ---
#include "key_app.h"       // 提供 Key_Get_Event
#include "led_app.h"       // 提供 LED_Disp
#include "lcd.h"           // 提供 LCD_DisplayStringLine 等底层画图
#include "uart_app.h"      // 提供 UART_SendString 和 uart_rx_buf
#include "eeprom_app.h"    // 提供 EEPROM_PushLog
#include "adc_app.h"       // 提供 adc_proc
#include "rtc_app.h"       // 提供 rtc_proc
#include <stdio.h>
#include <string.h>

/* ==========================================================
 * [模块 1]：全局数据处理与联动中枢
 * 考点：传感器数据转物理量、恒压/恒流控制算法、触发EEPROM保存
 * 调度周期：50ms
 * ========================================================== */
void Logic_Data_Proc(void) {
    // 1. 唤醒采集层，结果自动存入 sys 字典
    adc_proc(); 
    rtc_proc();

    // 2. 硬件联动：R38 电压 (0~3.3V) 映射为 PWM 频率 (1000~4300Hz)
    sys.pwm_freq = 1000 + (uint32_t)(sys.r38_voltage * 1000);

    // 3. 响应 B4 按键触发的 EEPROM 保存请求
    if (sys.eeprom_save_flag == true) {
        LogData_t new_log = {
            .hour = sys.hour,
            .min  = sys.min,
            .sec  = sys.sec,
            .volt = sys.v_threshold,
            .freq = sys.pwm_freq
        };
        EEPROM_PushLog(new_log); 
        sys.eeprom_save_flag = false; // 消费完毕必须清零
    }
}

/* ==========================================================
 * [模块 2]：LED 指示灯业务映射
 * 考点：页面指示、参数越限报警、系统心跳
 * 调度周期：20ms
 * ========================================================== */
void Logic_LED_Proc(void) {
    // 1. 每次进入先清空所有状态 (擦除上一帧)
    for(int i = 0; i < 8; i++) sys.led_ctrl[i] = 0;
    
    // 2. 页面指示灯逻辑
    if (sys.current_page == PAGE_DATA) sys.led_ctrl[0] = 1; 
    else if (sys.current_page == PAGE_PARA) sys.led_ctrl[1] = 1; 
    
    // 3. 闪烁心跳灯逻辑 (1Hz)
    if (HAL_GetTick() % 1000 < 500) sys.led_ctrl[2] = 1; 
    
    // 4. 越限报警灯逻辑
    if (sys.r37_voltage > sys.v_threshold) sys.led_ctrl[3] = 1; 
    
    // 5. 软定时器后台瞬发事件灯 (杜绝死锁)
    if (sys.led8_timer > 0) {
        sys.led_ctrl[7] = 1; 
        sys.led8_timer--;   
    }
    
    // 6. 提交到底层 74HC573 锁存器
    LED_Disp();
}

/* ==========================================================
 * [模块 3]：串口协议解析与回复
 * 考点：sscanf 解析参数、字符串截取、非法指令过滤
 * 调度周期：10ms
 * ========================================================== */
void Logic_UART_Proc(void) {
    // 收到底层 DMA 搬运完毕的一帧完整数据
    if (sys.uart_rx_ready == true) {
        float temp_v = 0.0f;
        
        // 考题范例：解析 "SET_V:2.55\r\n"
        if (sscanf((char *)uart_rx_buf, "SET_V:%f", &temp_v) == 1) {
            if (temp_v >= 0.0f && temp_v <= 3.3f) {
                sys.v_threshold = temp_v;
                UART_SendString("OK\r\n");
            } else {
                UART_SendString("ERR: Out of Range\r\n");
            }
        } else {
            UART_SendString("ERR: Format Error\r\n");
        }
        
        sys.uart_rx_ready = false; // 消费完毕必须清零
    }
}

/* ==========================================================
 * [模块 4]：LCD 屏幕渲染与按键交互
 * 考点：菜单切换、光标反白、长短按键处理
 * 调度周期：100ms
 * ========================================================== */
// 虚拟显存 (由于只有在这个函数里画图，因此放在这即可)
static char lcd_vram[10][21];      
static char lcd_vram_bak[10][21];  

void Logic_UI_Proc(void) {
    uint8_t key_val = 0;
    char temp[32]; 

    // --- 第一部分：消费按键并改变变量 ---
    while (Key_Get_Event(&key_val)) {
        switch (key_val) {
            case 1: // 【B1 短按】无缝切换页面
                sys.current_page = (sys.current_page == PAGE_DATA) ? PAGE_PARA : PAGE_DATA;
                memset(lcd_vram_bak, 0, sizeof(lcd_vram_bak)); // 切换清屏防残影
                break;
                
            case 2: // 【B2 短按】增加电压阈值
                if (sys.current_page == PAGE_PARA) {
                    sys.v_threshold += 0.5f;
                    if (sys.v_threshold > 3.3f) sys.v_threshold = 0.0f;
                }
                break;
                
            case 3: // 【B3 短按】增加 PWM 占空比
                if (sys.current_page == PAGE_PARA) {
                    sys.pwm_duty += 0.1f;
                    if (sys.pwm_duty > 1.0f) sys.pwm_duty = 0.1f;
                }
                break;
                
            case 4: // 【B4 短按】触发 EEPROM 保存
                sys.eeprom_save_flag = true; 
                break;
                
            case 11: // 【B1 长按】一键恢复出厂设定
                sys.v_threshold = 2.5f;
                sys.pwm_duty = 0.5f;
                sys.eeprom_save_flag = true; 
                break;
        }
    }
    
    // --- 第二部分：擦除显存画布 ---
    for(int i = 0; i < 10; i++) {
        sprintf(lcd_vram[i], "                    "); 
    }

    // --- 第三部分：将字典数据格式化并写入工作显存 ---
    if (sys.current_page == PAGE_DATA) {
        sprintf(temp, "      DATA PAGE");
        sprintf(lcd_vram[1], "%-20s", temp); 
        
        sprintf(temp, " Time: %02d:%02d:%02d", sys.hour, sys.min, sys.sec);
        sprintf(lcd_vram[3], "%-20s", temp);
        
        sprintf(temp, " V37:%.2fV V38:%.2fV", sys.r37_voltage, sys.r38_voltage);
        sprintf(lcd_vram[5], "%-20s", temp);
        
        sprintf(temp, " F1:%-5d F2:%-5d", (int)sys.freq_ch1, (int)sys.freq_ch2);
        sprintf(lcd_vram[7], "%-20s", temp);
        
    } else if (sys.current_page == PAGE_PARA) {
        sprintf(temp, "      PARA PAGE");
        sprintf(lcd_vram[1], "%-20s", temp);
        
        sprintf(temp, " V_Thr: %.2f V", sys.v_threshold);
        sprintf(lcd_vram[3], "%-20s", temp);
        
        sprintf(temp, " PWM Duty: %3.0f%%", sys.pwm_duty * 100.0f);
        sprintf(lcd_vram[5], "%-20s", temp);
        
        sprintf(temp, " Res Step: %-3d", sys.res_step);
        sprintf(lcd_vram[7], "%-20s", temp);
    }

    // --- 第四部分：防闪屏双缓冲底层刷新 ---
    for(uint8_t i = 0; i < 10; i++) {
        if (strcmp(lcd_vram[i], lcd_vram_bak[i]) != 0) {
            LCD_DisplayStringLine(i * 24, (uint8_t *)lcd_vram[i]); 
            strcpy(lcd_vram_bak[i], lcd_vram[i]);
        }
    }
}
