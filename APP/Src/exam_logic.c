/**
 * @file    exam_logic.c
 * @brief   考场业务逻辑总览
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
    // 1. 频率校准计算 (公式: f_cal = f_meas + PX)
    sys.f_a_cal = (int32_t)sys.freq_ch1 + sys.para_px;
    sys.f_b_cal = (int32_t)sys.freq_ch2 + sys.para_px;

    // 2. 超限报警判定 (边缘触发：从正常跨越到超限的那一瞬间，次数才加1)
    static bool is_a_over = false, is_b_over = false; // 历史状态标记
    
    // 只有频率大于 0 且大于 PH 时才算超限
    if (sys.f_a_cal > 0 && sys.f_a_cal > sys.para_ph) {
        if (!is_a_over) { sys.count_nha++; is_a_over = true; } // 产生上升沿
    } else {
        is_a_over = false; // 回落
    }
    
    if (sys.f_b_cal > 0 && sys.f_b_cal > sys.para_ph) {
        if (!is_b_over) { sys.count_nhb++; is_b_over = true; }
    } else {
        is_b_over = false;
    }

    // 3. 突变报警判定 (3秒滑动窗口算法)
    // 思路：每 100ms 采样一次，3 秒只需要一个深度为 30 的数组即可。
    static int32_t win_a[30] = {0}, win_b[30] = {0};
    static uint8_t win_idx = 0;
    static uint32_t last_win_tick = 0;
    static bool a_mutated = false, b_mutated = false; // 突变冷却锁

    if (HAL_GetTick() - last_win_tick >= 100) {
        last_win_tick = HAL_GetTick();
        
        // 压入最新有效频率数据 (无效的负数按 0 算或不处理)
        win_a[win_idx] = (sys.f_a_cal > 0) ? sys.f_a_cal : 0;
        win_b[win_idx] = (sys.f_b_cal > 0) ? sys.f_b_cal : 0;
        win_idx = (win_idx + 1) % 30;

        // 寻找 30 个点中的 Max 和 Min
        int32_t max_a = 0, min_a = 9999999, max_b = 0, min_b = 9999999;
        for(int i=0; i<30; i++) {
            if (win_a[i] > max_a) max_a = win_a[i];
            if (win_a[i] < min_a) min_a = win_a[i];
            if (win_b[i] > max_b) max_b = win_b[i];
            if (win_b[i] < min_b) min_b = win_b[i];
        }

        // 判定 A 通道突变
        if ((max_a - min_a) > sys.para_pd) {
            if (!a_mutated) { sys.count_nda++; a_mutated = true; }
        } else {
            a_mutated = false; // 差值恢复正常后解锁冷却
        }

        // 判定 B 通道突变
        if ((max_b - min_b) > sys.para_pd) {
            if (!b_mutated) { sys.count_ndb++; b_mutated = true; }
        } else {
            b_mutated = false;
        }
    }
}

/* ==========================================================
 * [模块 2]：LED 指示灯业务映射
 * @note   调度周期：20ms。根据 sys 字典的状态，决定哪个灯亮。
 * ========================================================== */
void Logic_LED_Proc(void) {
    for(int i = 0; i < 8; i++) sys.led_ctrl[i] = 0; // 擦除
    
    // 1) LD1: 监控界面点亮
    if (sys.current_page == PAGE_DATA) sys.led_ctrl[0] = 1;
    
    // 2) LD2: A通道频率 > PH 点亮
    if (sys.f_a_cal > sys.para_ph) sys.led_ctrl[1] = 1;
    
    // 3) LD3: B通道频率 > PH 点亮
    if (sys.f_b_cal > sys.para_ph) sys.led_ctrl[2] = 1;
    
    // 4) LD8: 任意通道突变次数 >= 3 次点亮
    if (sys.count_nda >= 3 || sys.count_ndb >= 3) sys.led_ctrl[7] = 1;
    
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
		static uint8_t para_focus = 0; // 0:PD, 1:PH, 2:PX
    char temp[32]; 

    /* ----------------------------------
     * A：按键交互控制 (极其反常的键位，按要求连线即可)
     * B4: 切页面。B3: 切参数/切模式。B1: 加。B2: 减
     * ---------------------------------- */
    while (Key_Get_Event(&key_val)) {
        switch (key_val) {
            case 4: // 【B4短按】切页面：DATA -> PARA -> RECD
                sys.current_page = (sys.current_page + 1) % 3;
                para_focus = 0; // 回到参数页重置焦点
                memset(lcd_vram_bak, 0, sizeof(lcd_vram_bak)); // 清屏防残影
								
								
                break;
                
            case 3: // 【B3短按】不同页面的功能不同
                if (sys.current_page == PAGE_DATA) {
                    sys.data_mode = !sys.data_mode; // 切换 频率/周期
                } else if (sys.current_page == PAGE_PARA) {
                    para_focus = (para_focus + 1) % 3; // 切换 PD/PH/PX
                }
                memset(lcd_vram_bak, 0, sizeof(lcd_vram_bak)); // 清屏
                break;
                
            case 13: // 【B3长按】清零记录 (键值3 + 10偏移)
                if (sys.current_page == PAGE_RECD) {
                    sys.count_nda = sys.count_ndb = 0;
                    sys.count_nha = sys.count_nhb = 0;
                    memset(lcd_vram_bak, 0, sizeof(lcd_vram_bak));
                }
                break;
                
            case 1: // 【B1短按】加参数
            case 2: // 【B2短按】减参数
								if (sys.current_page == PAGE_PARA){
                    int sign = (key_val == 1) ? 1 : -1;
                    if (para_focus == 0) { // 调节 PD
                        sys.para_pd += sign * 100;
                        if (sys.para_pd < 100 || sys.para_pd > 1000) sys.para_pd -= sign * 100;
                    } else if (para_focus == 1) { // 调节 PH
                        sys.para_ph += sign * 100;
                        if (sys.para_ph < 1000 || sys.para_ph > 10000) sys.para_ph -= sign * 100;
                    } else if (para_focus == 2) { // 调节 PX
                        sys.para_px += sign * 100;
                        if (sys.para_px < -1000 || sys.para_px > 1000) sys.para_px -= sign * 100;
                    }
                }
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
    /* ----------------------------------
     * C：画屏 (重点看格式化单位的智能切换)
     * ---------------------------------- */
    if (sys.current_page == PAGE_DATA) {
        sprintf(temp, "        DATA");
        sprintf(lcd_vram[1], "%-20s", temp); 
        
        if (sys.data_mode == 0) {
            // --- 频率显示模式 ---
            // A 通道
            if (sys.f_a_cal < 0) {
                sprintf(temp, "     A=NULL");
            } else if (sys.f_a_cal > 1000) {
                sprintf(temp, "     A=%.2fKHz", sys.f_a_cal / 1000.0f);
            } else {
                sprintf(temp, "     A=%dHz", (int)sys.f_a_cal);
            }
            sprintf(lcd_vram[3], "%-20s", temp);
					} else {
            // --- 周期显示模式 ---
            // 公式 T = 1/F 秒 = 1000000/F uS
            if (sys.f_a_cal <= 0) {
                sprintf(temp, "     A=NULL");
            } else {
                uint32_t period_us = 1000000 / sys.f_a_cal;
                if (period_us > 1000) {
                    sprintf(temp, "     A=%.2fmS", period_us / 1000.0f);
                } else {
                    sprintf(temp, "     A=%duS", (int)period_us);
                }
            }
            sprintf(lcd_vram[3], "%-20s", temp);
					}
				}else if (sys.current_page == PAGE_PARA) {
        //
        sprintf(temp, "        PARA");
        sprintf(lcd_vram[1], "%-20s", temp);
        sprintf(temp, "     PD=%dHz", (int)sys.para_pd);
        sprintf(lcd_vram[3], "%-20s", temp);
        sprintf(temp, "     PH=%dHz", (int)sys.para_ph);
        sprintf(lcd_vram[5], "%-20s", temp);
        sprintf(temp, "     PX=%dHz", (int)sys.para_px);
        sprintf(lcd_vram[7], "%-20s", temp);
        
			} else if (sys.current_page == PAGE_RECD) {
        //
        sprintf(temp, "        RECD");
        sprintf(lcd_vram[1], "%-20s", temp);
        sprintf(temp, "     NDA=%d", (int)sys.count_nda);
        sprintf(lcd_vram[3], "%-20s", temp);
        sprintf(temp, "     NDB=%d", (int)sys.count_ndb);
        sprintf(lcd_vram[5], "%-20s", temp);
        sprintf(temp, "     NHA=%d", (int)sys.count_nha);
        sprintf(lcd_vram[7], "%-20s", temp);
        sprintf(temp, "     NHB=%d", (int)sys.count_nhb);
        sprintf(lcd_vram[9], "%-20s", temp);
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
