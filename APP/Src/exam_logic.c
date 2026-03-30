/**
 * @file    exam_logic.c
 * @brief   第十二届省赛真题：智能停车场计费系统 (基于 MVC 模板)
 */

#include "exam_logic.h"
#include "global_system.h" 
#include "key_app.h"
#include "led_app.h"
#include "lcd.h"
#include "uart_app.h"
#include <stdio.h>
#include <string.h>

/* ==========================================================
 * [私有工具函数]：计算两个时间的差值（小时），不足1小时进1
 * ========================================================== */
static int Get_Hours_Diff(int y1, int m1, int d1, int h1, int min1, int s1,
                          int y2, int m2, int d2, int h2, int min2, int s2) {
    // 简易闰年判定与每月天数 (2000年起算)
    const int days[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    long days1 = y1 * 365 + (y1 + 3) / 4; 
    for(int i = 1; i < m1; i++) days1 += days[i];
    if(m1 > 2 && (y1 % 4 == 0)) days1++;
    days1 += d1;

    long days2 = y2 * 365 + (y2 + 3) / 4;
    for(int i = 1; i < m2; i++) days2 += days[i];
    if(m2 > 2 && (y2 % 4 == 0)) days2++;
    days2 += d2;

    long sec1 = days1 * 86400L + h1 * 3600L + min1 * 60L + s1;
    long sec2 = days2 * 86400L + h2 * 3600L + min2 * 60L + s2;

    long diff = sec2 - sec1;
    if(diff < 0) return 0; // 逻辑错误：出库时间早于入库时间

    int hours = diff / 3600;
    if (diff % 3600 > 0) hours++; // 不足1小时按1小时计
    
    return hours;
}

/* ==========================================================
 * [模块 1]：核心控制层 (Controller) - 按键路由
 * ========================================================== */
void Logic_Ctrl_Proc(void) {
    uint8_t key_val = 0;
    
    while (Key_Get_Event(&key_val)) {
        switch (key_val) {
            case 1: // 【B1】：切换页面 (Data <-> Para)
                sys.current_page = (sys.current_page == PAGE_DATA) ? PAGE_PARA : PAGE_DATA;
                break;
                
            case 2: // 【B2】：CNBR和VNBR费率增加 0.5 (仅在Para页面有效)
                if (sys.current_page == PAGE_PARA) {
                    sys.c_rate += 0.5f;
                    sys.v_rate += 0.5f;
                }
                break;
                
            case 3: // 【B3】：CNBR和VNBR费率减少 0.5 (仅在Para页面有效)
                if (sys.current_page == PAGE_PARA) {
                    sys.c_rate -= 0.5f;
                    sys.v_rate -= 0.5f;
                    if (sys.c_rate < 0) sys.c_rate = 0.0f; // 边界保护
                    if (sys.v_rate < 0) sys.v_rate = 0.0f;
                }
                break;
                
            case 4: // 【B4】：切换 PA7 输出模式
                sys.pa7_mode = !sys.pa7_mode;
                break;
        }
    }
}

/* ==========================================================
 * [模块 2]：全局数据联动中枢 (Model)
 * ========================================================== */
void Logic_Data_Proc(void) {
    // 1. 实时统计各种车辆的数量
    int c_cnt = 0, v_cnt = 0;
    for (int i = 0; i < 8; i++) {
        if (sys.cars[i].is_occupied) {
            if (strcmp(sys.cars[i].type, "CNBR") == 0) c_cnt++;
            if (strcmp(sys.cars[i].type, "VNBR") == 0) v_cnt++;
        }
    }
    sys.cnbr_cnt = c_cnt;
    sys.vnbr_cnt = v_cnt;
    sys.idle_cnt = 8 - c_cnt - v_cnt;

    // 2. 根据 PA7_mode 控制底层 PWM (复用模板的 pwm 变量即可控制硬件)
    if (sys.pa7_mode == 1) {
        sys.pwm_freq = 2000;   // 2KHz
        sys.pwm_duty = 0.2f;   // 20% 占空比
    } else {
        sys.pwm_freq = 2000;
        sys.pwm_duty = 0.0f;   // 0% 占空比 = 持续低电平
    }
}

/* ==========================================================
 * [模块 3]：LED 状态控制 (View)
 * ========================================================== */
void Logic_LED_Proc(void) {
    for(int i = 0; i < 8; i++) sys.led_ctrl[i] = 0;
    
    // LD1：若停车场内存在空闲车位，点亮
    if (sys.idle_cnt > 0) sys.led_ctrl[0] = 1; 
    
    // LD2：若 PA7 处于 2KHz 20% 脉冲模式，点亮
    if (sys.pa7_mode == 1) sys.led_ctrl[1] = 1;
    
    LED_Disp();
}

/* ==========================================================
 * [模块 4]：串口不定长协议解析 (Controller)
 * ========================================================== */
void Logic_UART_Proc(void) {
    if (sys.uart_rx_ready == true) { 
        
        char type[10], id[10];
        int y, m, d, h, min, s;
        char tx_buf[64];
        
        // 尝试匹配标准的出入库格式: "CNBR:A392:200202120000"
        // 注意：%4[^:] 表示读取前4个字符直到遇到冒号，这能完美切割 Type 和 ID
        if (sscanf((char *)uart_rx_buf, "%4[^:]:%4[^:]:%2d%2d%2d%2d%2d%2d", 
                   type, id, &y, &m, &d, &h, &min, &s) == 8) {
            
            // 数据逻辑校验：判断车辆是否已经在库内
            int found_idx = -1;
            for (int i = 0; i < 8; i++) {
                if (sys.cars[i].is_occupied && strcmp(sys.cars[i].id, id) == 0) {
                    found_idx = i;
                    break;
                }
            }

            // --- 驶出逻辑 ---
            if (found_idx != -1) {
                // 车牌匹配成功，说明是出库
                if (strcmp(sys.cars[found_idx].type, type) == 0) {
                    // 计算停车时长
                    int park_hours = Get_Hours_Diff(
                        sys.cars[found_idx].y, sys.cars[found_idx].m, sys.cars[found_idx].d,
                        sys.cars[found_idx].h, sys.cars[found_idx].min, sys.cars[found_idx].s,
                        y, m, d, h, min, s);
                    
                    // 计算费用
                    float fee = 0.0f;
                    if (strcmp(type, "CNBR") == 0) fee = park_hours * sys.c_rate;
                    if (strcmp(type, "VNBR") == 0) fee = park_hours * sys.v_rate;
                    
                    // 回复计费信息
                    sprintf(tx_buf, "%s:%s:%d:%.2f\r\n", type, id, park_hours, fee);
                    UART_SendString(tx_buf);
                    
                    // 释放车位
                    sys.cars[found_idx].is_occupied = false;
                } else {
                    UART_SendString("Error\r\n"); // 车牌存在但类型不符
                }
            } 
            // --- 驶入逻辑 ---
            else {
                if (sys.idle_cnt > 0) {
                    // 有空位，寻找第一个空位存入
                    for (int i = 0; i < 8; i++) {
                        if (!sys.cars[i].is_occupied) {
                            strcpy(sys.cars[i].type, type);
                            strcpy(sys.cars[i].id, id);
                            sys.cars[i].y = y; sys.cars[i].m = m; sys.cars[i].d = d;
                            sys.cars[i].h = h; sys.cars[i].min = min; sys.cars[i].s = s;
                            sys.cars[i].is_occupied = true;
                            break;
                        }
                    }
                    // 收到入场信息不需要回复 (真题要求)
                } else {
                    // 车位已满
                    UART_SendString("Error\r\n"); 
                }
            }
        } 
        else {
            // 格式完全不匹配
            UART_SendString("Error\r\n");
        }

        // 扫尾清理
        sys.uart_rx_ready = false;          
        memset(uart_rx_buf, 0, UART_RX_MAX_LEN); 
        uart_rx_len = 0;
    }
}

/* ==========================================================
 * [模块 5]：LCD 屏幕渲染 (View)
 * ========================================================== */
static char lcd_vram[10][21];      
static char lcd_vram_bak[10][21];  

void Logic_UI_Proc(void) {
    char temp[32]; 
    
    static PageState_e last_page = PAGE_DATA;
    if (sys.current_page != last_page) {
        memset(lcd_vram_bak, 0, sizeof(lcd_vram_bak)); 
        last_page = sys.current_page;
    }

    for(int i = 0; i < 10; i++) {
        sprintf(lcd_vram[i], "                    "); 
    }

    // --- 界面渲染逻辑 ---
    if(sys.current_page == PAGE_DATA) {
        sprintf(lcd_vram[1], "       Data         ");
        
        sprintf(temp, "   CNBR:%d", sys.cnbr_cnt);
        sprintf(lcd_vram[3], "%-20s", temp);     
        
        sprintf(temp, "   VNBR:%d", sys.vnbr_cnt);
        sprintf(lcd_vram[5], "%-20s", temp);
        
        sprintf(temp, "   IDLE:%d", sys.idle_cnt);
        sprintf(lcd_vram[7], "%-20s", temp);
        
    } else if(sys.current_page == PAGE_PARA) {
        sprintf(lcd_vram[1], "       Para         ");
        
        sprintf(temp, "   CNBR:%.2f", sys.c_rate);
        sprintf(lcd_vram[3], "%-20s", temp);
        
        sprintf(temp, "   VNBR:%.2f", sys.v_rate);
        sprintf(lcd_vram[5], "%-20s", temp);
    }
    
    // --- 差异化局部刷新 (防闪屏) ---
    for(uint8_t i = 0; i < 10; i++) {
        if (strcmp(lcd_vram[i], lcd_vram_bak[i]) != 0) {
            LCD_DisplayStringLine(i * 24, (uint8_t *)lcd_vram[i]); 
            strcpy(lcd_vram_bak[i], lcd_vram[i]); 
        }
    }
}
