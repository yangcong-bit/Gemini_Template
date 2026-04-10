/**
 * @file    exam_logic.c
 * @brief   考场业务逻辑总览 (纯净骨架 + 例程注释版)
 * @note    在比赛时，你只需要修改这一个文件。
 * 核心区严禁修改（防闪屏/死机），业务区自由发挥。
 */

#include "exam_logic.h"
#include "global_system.h" 

/* --- 引入底层对外暴露的 API --- */
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
        
        /* --- 【业务逻辑：自由发挥区 - 按键路由】 --- */
        /* ================= [使用例程参考：按键功能路由] =================
        switch (key_val) {
            case 1: // 【B1 短按】: 切换页面 (Data -> Para -> Recd)
                sys.current_page = (sys.current_page + 1) % 3;
                break;
                
            case 2: // 【B2 短按】: 参数加 (附带上限保护防跑飞)
                if (sys.current_page == PAGE_PARA) {
                    sys.PH += 100;
                    if (sys.PH > 10000) sys.PH = 10000;
                }
                break;
                
            case 3: // 【B3 短按】: 参数减或页面模式切换
                if (sys.current_page == PAGE_PARA) {
                    sys.PH -= 100;
                    if (sys.PH < 1000) sys.PH = 1000; // 下限保护
                } else if (sys.current_page == PAGE_DATA) {
                    sys.flag = !sys.flag; // 切换数据页的显示单位
                }
                break;
                
            case 4: // 【B4 短按】: 触发 EEPROM 保存
                sys.eeprom_save_flag = true; 
                break;
                
            case 11: // 【B1 长按】: 长按触发特殊逻辑 (键值+10)
                // sys.key_data = 0; 
                break;
        }
        ================================================================ */
    }
}

/* ==========================================================
 * [模块 2]：全局数据采集与联动中枢 (Model)
 * @note   建议调度周期：10ms~50ms。负责搬运底层数据到 sys 字典，并处理核心逻辑。
 * ========================================================== */
void Logic_Data_Proc(void) {
    /* --- 【模板核心：固定资产】 --- */
    adc_proc(); // 启动底层 ADC 搬运
    rtc_proc(); // 启动底层 RTC 刷新

    /* --- 【业务逻辑：自由发挥区 - 数据换算与联动】 --- */
    /* ================= [使用例程参考：数据联动与保存] =================
    // 1. 频率预处理或传感器换算
    // sys.FA = sys.freq_ch1 - sys.PX;
    // if (sys.FA < 0) sys.FA = 0;
    
    // 2. 越限逻辑计算：检测是否超过阈值，记录次数 (防连加)
    // if (sys.FA > sys.PH) {
    //     if (!sys.Alog) { // 上升沿触发，防止一直累加
    //         sys.NHA++;   
    //         sys.Alog = true;
    //     }
    // } else {
    //     sys.Alog = false;
    // }

    // 3. 处理 EEPROM 异步存储请求 (响应按键)
    // if (sys.eeprom_save_flag) {
    //     LogData_t log = { .hour = sys.hour, .min = sys.min, .sec = sys.sec, .freq = (uint32_t)sys.FA };
    //     EEPROM_PushLog(log);          // 压入后台切片写队列，绝不卡死主循环
    //     sys.eeprom_save_flag = false; // 消费完毕，清空标志
    // }
    
    // 4. 高级：EEPROM 防抖自动保存 (参数修改后停止按键 2秒自动保存)
    // static uint32_t last_saved_PD = 0xFFFF;  
    // static uint32_t auto_save_timer = 0;    
    // if (sys.PD != last_saved_PD) {
    //     auto_save_timer++; 
    //     if (auto_save_timer >= 200) { // 稳定 2 秒不按键后保存
    //         LogData_t new_log = { .freq = sys.PD }; 
    //         EEPROM_PushLog(new_log);   
    //         last_saved_PD = sys.PD;    
    //         auto_save_timer = 0;       
    //     }
    // } else { auto_save_timer = 0; }
    ================================================================ */
}

/* ==========================================================
 * [模块 3]：LED 指示灯业务映射 (View)
 * @note   建议调度周期：20ms。纯粹只读 sys 字典，决定硬件亮灭。
 * ========================================================== */
void Logic_LED_Proc(void) {
    /* --- 【模板核心：擦除上一帧】 --- */
    for(int i = 0; i < 8; i++) sys.led_ctrl[i] = 0;
    
    /* --- 【业务逻辑：自由发挥区 - 状态映射】 --- */
    /* ================= [使用例程参考：指示灯映射] =================
    // 1. 页面指示灯
    // if (sys.current_page == PAGE_DATA) sys.led_ctrl[0] = 1; // LD1 亮
    // if (sys.current_page == PAGE_PARA) sys.led_ctrl[1] = 1; // LD2 亮
    // if (sys.current_page == PAGE_RECD) sys.led_ctrl[2] = 1; // LD3 亮
    
    // 2. 超标闪烁指示 (LD8 5Hz闪烁，对 200 取余：亮 100ms 灭 100ms)
    // if (sys.FA > sys.PH && (HAL_GetTick() % 200 < 100)) {
    //     sys.led_ctrl[7] = 1;
    // }
    ================================================================ */
    
    /* --- 【模板核心：提交硬件】 --- */
    LED_Disp();
}

/* ==========================================================
 * [模块 4]：串口协议解析与回复 (Controller 扩展)
 * @note   建议调度周期：10ms。处理 DMA 后台搬运完毕的不定长指令。
 * ========================================================== */
void Logic_UART_Proc(void) {
    /* --- 【模板核心：就绪判定】 --- */
    if (sys.uart_rx_ready == true) { 
        
        /* --- 【业务逻辑：自由发挥区 - 串口解析区】 --- */
        /* ================= [使用例程参考：复杂指令解析] =================
        // char tx_msg[64];
        // int val = 0;
        // char plate[10]; 
        // 
        // // 1. strstr 匹配查询指令 (如上位机发送: "GET_FREQ")
        // if (strstr((char *)uart_rx_buf, "GET_FREQ") != NULL) {
        //     sprintf(tx_msg, "FA:%dHz FB:%dHz\r\n", (int)sys.FA, (int)sys.FB);
        //     UART_SendString(tx_msg);
        // }
        // 
        // // 2. sscanf 提取数字并限幅 (如上位机发送: "SET_PX:200")
        // else if (sscanf((char *)uart_rx_buf, "SET_PX:%d", &val) == 1) {
        //     if (val >= -1000 && val <= 1000) { // 限幅保护
        //         sys.PX = val;
        //         UART_SendString("OK\r\n");
        //     } else { UART_SendString("ERR: Out of Range\r\n"); }
        // }
        //
        // // 3. sscanf 提取带字符串的复杂指令 (如: "VNBR:A1234123015")
        // // 注意：%5s 限制提取 5 个字符防溢出，对应 plate 数组至少要是 char plate[6]
        // else if (sscanf((char *)uart_rx_buf, "VNBR:%5s", plate) == 1) {
        //     strcpy(sys.temp_v, plate); // 安全拷贝至全局字典
        //     UART_SendString("OK\r\n");
        // }
        // 
        // // 4. 兜底错误回复
        // else { 
        //     UART_SendString("Format Error\r\n"); 
        // }
        ================================================================ */

        /* --- 【模板核心：现场清理，防止死机与死锁(严禁删除)】 --- */
        sys.uart_rx_ready = false;               // 必须清除标志位，否则会死锁重复解析
        memset(uart_rx_buf, 0, UART_RX_MAX_LEN); // 清空缓冲区，防止旧指令残留干扰
        uart_rx_len = 0;
    }
}

/* ==========================================================
 * [模块 5]：LCD 屏幕渲染引擎 (View) - 终极双缓冲变色版
 * @note   建议调度周期：100ms (10FPS)。纯粹只读 sys 字典，双缓冲防闪。
 * ========================================================== */
static char lcd_vram[10][21];      // 当前帧工作显存
static char lcd_vram_bak[10][21];  // 历史帧文字备份 (防闪屏核心)

static uint16_t lcd_color[10];         // 当前帧前景色显存
static uint16_t lcd_color_bak[10];     // 历史帧前景色备份
static uint16_t lcd_bg_color[10];      // 当前帧背景色显存
static uint16_t lcd_bg_color_bak[10];  // 历史帧背景色备份

void Logic_UI_Proc(void) {
    char temp[32]; 
    
    /* --- 【模板核心：跨页面防残影】 --- */
    static PageState_e last_page = PAGE_DATA;
    if (sys.current_page != last_page) {
        memset(lcd_vram_bak, 0, sizeof(lcd_vram_bak)); 
        memset(lcd_color_bak, 0, sizeof(lcd_color_bak)); 
        memset(lcd_bg_color_bak, 0, sizeof(lcd_bg_color_bak)); // 切换页面时，连同颜色备份一起破坏
        last_page = sys.current_page;
    }

    /* --- 【模板核心：擦除画布填充空格，重置默认黑底白字】 --- */
    for(int i = 0; i < 10; i++) {
        sprintf(lcd_vram[i], "                    "); // 预填20个空格覆盖旧字符串
        lcd_color[i] = White;    // 默认前景白字
        lcd_bg_color[i] = Black; // 默认背景黑底
    }

    /* --- 【业务逻辑：自由发挥区 - UI 显存渲染】 --- */
    /* ================= [使用例程参考：LCD 分页与单行颜色高亮] =================
    // if(sys.current_page == PAGE_DATA) {
    //     sprintf(lcd_vram[0], "      DATA PAGE     ");
    //     
    //     // 1. 普通浮点数显示 (默认白字黑底)
    //     sprintf(temp, "  Volt:%.2fV", sys.r37_voltage);
    //     sprintf(lcd_vram[2], "%-20s", temp);     // %-20s 左对齐且用空格补齐20格
    //     
    //     // 2. 越限报警：超标时这一行变成【红底白字】
    //     sprintf(temp, "  Speed:%.1f", sys.NAME_V);
    //     sprintf(lcd_vram[4], "%-20s", temp);
    //     if(sys.NAME_V > 50.0f) {
    //         lcd_bg_color[4] = Red; // 背景变红，前景色没动所以还是白色
    //     }
    //     
    //     // 3. 极值显示：用绿色字体显示
    //     sprintf(temp, "  Max:%.1f", sys.NAME_MH);
    //     sprintf(lcd_vram[6], "%-20s", temp);
    //     lcd_color[6] = Green;
    //     
    // } else if(sys.current_page == PAGE_PARA) {
    //     sprintf(lcd_vram[0], "      PARA PAGE     ");
    //     
    //     // 4. 参数选中高亮：被选中调整的参数变成【黄底黑字】
    //     sprintf(temp, "  Thr_R:%d", sys.NAME_R);
    //     sprintf(lcd_vram[3], "%-20s", temp);
    //     if(sys.para_select == 0) { // 假设0代表选中了R
    //         lcd_color[3] = Black;    // 字体变黑
    //         lcd_bg_color[3] = Yellow;// 背景变黄
    //     }
    //
    //     sprintf(temp, "  Thr_K:%d", sys.NAME_K);
    //     sprintf(lcd_vram[5], "%-20s", temp);
    //     if(sys.para_select == 1) { // 假设1代表选中了K
    //         lcd_color[5] = Black;
    //         lcd_bg_color[5] = Yellow;
    //     }
    // } 
    ================================================================ */
    
    /* --- 【模板核心：差异化防闪屏刷新引擎 (严禁删除)】 --- */
    for(uint8_t i = 0; i < 10; i++) {
        // 利用 strcmp 比对，文字变了，或者颜色变了，才调用慢速的硬件 SPI 指令重绘
        if (strcmp(lcd_vram[i], lcd_vram_bak[i]) != 0 || 
            lcd_color[i] != lcd_color_bak[i] || 
            lcd_bg_color[i] != lcd_bg_color_bak[i]) {
            
            // 1. 设定底层色彩寄存器
            LCD_SetTextColor(lcd_color[i]);
            LCD_SetBackColor(lcd_bg_color[i]);
            
            // 2. 刷入屏幕 (行高固定 24)
            LCD_DisplayStringLine(i * 24, (uint8_t *)lcd_vram[i]); 
            
            // 3. 同步工作显存到三个备份区
            strcpy(lcd_vram_bak[i], lcd_vram[i]); 
            lcd_color_bak[i] = lcd_color[i];
            lcd_bg_color_bak[i] = lcd_bg_color[i];
        }
    }
    
    // 安全扫尾：恢复成全局标准色，防止污染主循环中其他绘制指令（画圈、清屏等）
    LCD_SetTextColor(White);
    LCD_SetBackColor(Black);
}
