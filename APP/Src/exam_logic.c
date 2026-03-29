/**
 * @file    exam_logic.c
 * @brief   考场业务逻辑总览 (彻底分离 MVC 架构)
 * @note    在比赛时，你只需要修改这一个文件。
 * 包含：按键处理(Ctrl)、数据联动(Model)、屏幕/LED(View)。
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
 * [模块 1]：核心控制层 (Controller) - 按键消费与逻辑路由
 * @note   建议调度周期：50ms。只修改 sys 字典，绝不碰显存画图。
 * ========================================================== */
void Logic_Ctrl_Proc(void) {
    uint8_t key_val = 0;

    // 从消息队列中消费所有积压的按键事件
    while (Key_Get_Event(&key_val)) {
        switch (key_val) {
            case 1: // 【B1 短按】：无缝切换页面
                sys.current_page = (sys.current_page == PAGE_DATA) ? PAGE_PARA : PAGE_DATA;
                break;
                
            case 2: // 【B2 短按】：在设置页增加阈值
                if (sys.current_page == PAGE_PARA) {
                    sys.v_threshold += 0.5f;
                    if (sys.v_threshold > 3.3f) sys.v_threshold = 0.0f;
                }
                break;
                
            case 3: // 【B3 短按】：在设置页增加 PWM 占空比
                if (sys.current_page == PAGE_PARA) {
                    sys.pwm_duty += 0.1f;
                    if (sys.pwm_duty > 1.0f) sys.pwm_duty = 0.1f;
                }
                break;
                
            case 14: // 【B4 长按】：触发保存 (假设键值4 + 长按偏移10 = 14)
                sys.eeprom_save_flag = true; 
                break;

            case 11: // 【B1 长按】：一键恢复出厂设置并保存
                sys.v_threshold = 2.5f;
                sys.pwm_duty = 0.5f;
                sys.eeprom_save_flag = true; 
                break;
        }
    }
}

/* ==========================================================
 * [模块 2]：全局数据采集与联动中枢 (Model)
 * @note   建议调度周期：50ms。负责搬运底层数据到 sys 字典，并处理核心逻辑。
 * ========================================================== */
void Logic_Data_Proc(void) {
    // 1. 唤醒采集层，将最新的 ADC 电压和 RTC 时间更新到 sys 字典
    adc_proc(); 
    rtc_proc();

    // 2. 硬件映射逻辑：例如将 R38 传感器电压映射为 PWM 频率
    sys.pwm_freq = 1000 + (uint32_t)(sys.r38_voltage * 1000);

    // 3. 响应 EEPROM 异步保存请求
    if (sys.eeprom_save_flag == true) {
        LogData_t new_log = {
            .hour = sys.hour,
            .min  = sys.min,
            .sec  = sys.sec,
            .volt = sys.v_threshold,
            .freq = sys.pwm_freq 
        };
        // 压入后台异步存储队列，5ms切片慢慢写，绝不卡顿
        EEPROM_PushLog(new_log); 
        sys.eeprom_save_flag = false; // 清除请求标志
    }
}

/* ==========================================================
 * [模块 3]：LED 指示灯业务映射 (View)
 * @note   建议调度周期：20ms。纯粹只读 sys 字典，决定硬件亮灭。
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
    
    // 3. 越限报警闪烁灯逻辑 (LD8 5Hz 闪烁)
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
 * [模块 4]：串口协议解析与回复 (Controller 扩展)
 * @note   建议调度周期：10ms。处理 DMA 后台搬运完毕的不定长指令。
 * ========================================================== */
void Logic_UART_Proc(void) {
    if (sys.uart_rx_ready == true) {
        float temp_v = 0.0f;
        
        // 解析 "SET_V:2.55\r\n" 格式的字符串
        if (sscanf((char *)uart_rx_buf, "SET_V:%f", &temp_v) == 1) {
            // 参数合法性防呆校验
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
 * [模块 5]：LCD 屏幕渲染引擎 (View)
 * @note   建议调度周期：100ms (10FPS)。纯粹只读 sys 字典，双缓冲防闪。
 * ========================================================== */
static char lcd_vram[10][21];      // 当前帧工作显存
static char lcd_vram_bak[10][21];  // 历史帧备份显存 (防闪屏核心)

void Logic_UI_Proc(void) {
    char temp[32]; 

    // 1. 监测页面切换：若发现当前页面与历史页面不符，强制清空备份显存，抹除残影
    static PageState_e last_page = PAGE_DATA;
    if (sys.current_page != last_page) {
        memset(lcd_vram_bak, 0, sizeof(lcd_vram_bak));
        last_page = sys.current_page;
    }

    // 2. 擦除工作显存画布 (必须填满 20 个空格，防止上一次长字符串的尾部残留)
    for(int i = 0; i < 10; i++) {
        sprintf(lcd_vram[i], "                    "); 
    }

    // 3. 将 sys 字典变量格式化进显存
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
    }

    // 4. 对比备份显存，差异化刷新底层硬件
    for(uint8_t i = 0; i < 10; i++) {
        // strcmp 为 0 代表字符串相同，不为 0 代表该行发生了变化
        if (strcmp(lcd_vram[i], lcd_vram_bak[i]) != 0) {
            LCD_DisplayStringLine(i * 24, (uint8_t *)lcd_vram[i]); 
            // 刷新屏幕后，将工作显存同步回备份显存
            strcpy(lcd_vram_bak[i], lcd_vram[i]);
        }
    }
}
