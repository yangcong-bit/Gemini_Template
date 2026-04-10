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

float PI=3.14;

/* ==========================================================
 * [模块 1]：核心控制层 (Controller) - 按键消费与逻辑路由
 * @note   建议调度周期：50ms。只修改 sys 字典，绝不碰显存画图。
 * ========================================================== */
void Logic_Ctrl_Proc(void) {
    uint8_t key_val = 0;
    
    // 从消息队列中消费所有积压的按键事件
    while (Key_Get_Event(&key_val)) {
        
                 switch (key_val) {
            case 1:
            if(sys.current_page== PAGE_DATA){
            sys.current_page= PAGE_PARA;
            sys.para_select=0;
            }else if(sys.current_page== PAGE_PARA){
            sys.current_page= PAGE_RECD;
                sys.NAME_R=sys.TMP_R;
                sys.NAME_K=sys.TMP_K;
            }else if(sys.current_page== PAGE_RECD){
            sys.current_page= PAGE_DATA;
            }
                break;
                
            case 2: 
                if (sys.current_page == PAGE_PARA) {
                sys.para_select=! sys.para_select;
                }else if(sys.current_page == PAGE_DATA){
                    if(sys.M_flag==0){          //在数据界面下，用于切换选择低频或高频模式。按键按下后，5秒内不可再次触发切换功能。 
                    sys.NAME_M=!sys.NAME_M;
                    sys.NAME_N++;
                    sys.M_flag=1;
                    if(sys.NAME_M==0){
                    sys.start_freq=8000;
                    sys.target_freq=4000;
                    }else if(sys.NAME_M==1){
                    sys.start_freq=4000;
                    sys.target_freq=8000;
                        }
                    }
                    
                }
                break;
                
            case 3: // 【B3 短按】: 参数减或页面模式切换
                if (sys.current_page == PAGE_PARA) {
                    if(sys.para_select==0){
                    sys.TMP_R++;
                    if(sys.TMP_R > 10) sys.TMP_R = 1; // 超过10变回1
                    }else if(sys.para_select==1){
                    sys.TMP_K++;
                    if(sys.TMP_K > 10) sys.TMP_K = 1; // 超过10变回1
                    }
                }
                break;
                
            case 4: // 【B4 短按】: 触发 EEPROM 保存
                if (sys.current_page == PAGE_PARA) {
                    if(sys.para_select==0){
                    sys.TMP_R--;
                        if(sys.TMP_R < 1) sys.TMP_R = 10; // 小于1变回10
                    }else if(sys.para_select==1){
                    sys.TMP_K--;
                        if(sys.TMP_K < 1) sys.TMP_K = 10; // 小于1变回10
                    }
                }
                break;
                
            case 14: // 【B1 长按】: 长按触发特殊逻辑 (键值+10)
                if (sys.current_page == PAGE_DATA){
                    
                sys.is_locked =! sys.is_locked;
                    
                }
            
                break;
        }

    }
}

/* ==========================================================
 * [模块 2]：全局数据采集与联动中枢 (Model)
 * @note   建议调度周期：10ms。负责搬运底层数据到 sys 字典，并处理核心逻辑。
 * ========================================================== */
void Logic_Data_Proc(void) {
    /* --- 【模板核心：固定资产】 --- */
    
    static uint16_t TIME_M;
    static float M_STEP;
    
    
    adc_proc(); // 启动底层 ADC 搬运
    rtc_proc(); // 启动底层 RTC 刷新
    
    if(sys.M_flag==1){
        TIME_M++;
        M_STEP=(sys.target_freq-sys.start_freq)/500;//求步进值
        sys.pwm_freq=sys.start_freq+(TIME_M*M_STEP);//求每次运行的频率
        if(TIME_M>=500){
        sys.M_flag=0;
            TIME_M=0;
            sys.pwm_freq=sys.target_freq;
        }
    }

    sys.NAME_V=sys.freq_ch1*2*PI*sys.NAME_R/(100*sys.NAME_K);//求速度
    
    
    if(sys.is_locked==0){//锁定占空比调整功能
        if(sys.r37_voltage<=1){
        sys.pwm_duty=0.1;
        }else if(1<sys.r37_voltage && sys.r37_voltage<3){
        sys.pwm_duty=0.1+((sys.r37_voltage-1)/2)*0.75;
        }else if(sys.r37_voltage>=3){
        sys.pwm_duty=0.85;
        }//占空比调整功能
    }
    
    sys.NAME_P=sys.pwm_duty*100;
    
    /* --- 2秒速度极大值防抖统计算法 --- */
    static float candidate_speed = 0.0f; // 候选打擂台速度
    static uint16_t speed_timer = 0;     // 2秒稳定计时器

    // 1. 求出实时速度和候选速度的差值 (相当于 fabs)
    float diff = sys.NAME_V - candidate_speed;
    if(diff < 0) diff = -diff;

    // 2. 如果速度稳定 (考虑到硬件 ADC 换算的微小抖动，容忍 0.5 的误差)
    if(diff < 0.5f){
        speed_timer++;
        if(speed_timer >= 200){ // 10ms * 200次 = 2000ms = 2秒
            // 稳定了2秒，可以正式挑战极值了！
            if(sys.NAME_M == 1 && candidate_speed > sys.NAME_MH) {
                sys.NAME_MH = candidate_speed;
            }
            if(sys.NAME_M == 0 && candidate_speed > sys.NAME_ML) {
                sys.NAME_ML = candidate_speed;
            }
            speed_timer = 200; // 防止定时器无脑累加溢出
        }
    }else{
        // 3. 只要速度发生剧烈变化，立刻换人打擂，并清零计时器！
        candidate_speed = sys.NAME_V;
        speed_timer = 0;
    }
    
    
    
}

/* ==========================================================
 * [模块 3]：LED 指示灯业务映射 (View)
 * @note   建议调度周期：20ms。纯粹只读 sys 字典，决定硬件亮灭。
 * ========================================================== */
void Logic_LED_Proc(void) {
    /* --- 【模板核心：擦除上一帧】 --- */
    for(int i = 0; i < 8; i++) sys.led_ctrl[i] = 0;
    
    if (sys.current_page == PAGE_DATA) {
        sys.led_ctrl[0] = 1; // LD1 亮
    }else{
        sys.led_ctrl[0] = 0; 
    }
    
    if(sys.M_flag==1){
    
    if(HAL_GetTick()%200>100){
        sys.led_ctrl[1] = 1;
    }else{
        sys.led_ctrl[1] = 0;
        }
    }else{
        sys.led_ctrl[1] = 0;
    }
    
    if(sys.is_locked==1){
    sys.led_ctrl[2] = 1;
    }else{
    sys.led_ctrl[2] = 0;
    }
    
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
 * [模块 5]：LCD 屏幕渲染引擎 (View)
 * @note   建议调度周期：100ms (10FPS)。纯粹只读 sys 字典，双缓冲防闪。
 * ========================================================== */
static char lcd_vram[10][21];      // 当前帧工作显存
static char lcd_vram_bak[10][21];  // 历史帧备份显存 (防闪屏核心)

void Logic_UI_Proc(void) {
    char temp[32]; 
    
    /* --- 【模板核心：跨页面防残影】 --- */
    static PageState_e last_page = PAGE_DATA;
    if (sys.current_page != last_page) {
        memset(lcd_vram_bak, 0, sizeof(lcd_vram_bak)); // 切换页面时强制全刷，抹除残影
        last_page = sys.current_page;
    }

    /* --- 【模板核心：擦除画布填充空格】 --- */
    for(int i = 0; i < 10; i++) {
        sprintf(lcd_vram[i], "                    "); // 预填20个空格覆盖旧字符串
    }

    
    
         if(sys.current_page == PAGE_DATA) {
         sprintf(temp, "        DATA");
         sprintf(lcd_vram[1], "%-20s", temp);     // %-20s 左对齐且用空格补齐20格
             
         // 字符串显示
         sprintf(temp, "     M=%d", sys.NAME_M);
         sprintf(lcd_vram[3], "%-20s", temp);     // %-20s 左对齐且用空格补齐20格
        
        // 浮点数显示
         sprintf(temp, "     P=%d%%", (int)sys.NAME_P);
         sprintf(lcd_vram[4], "%-20s", temp);
         
         // 格式化时间显示 (补零)
         sprintf(temp, "     V=%.1f", sys.NAME_V);
         sprintf(lcd_vram[5], "%-20s", temp);
         
        } 
         else if(sys.current_page == PAGE_PARA) {
         sprintf(temp, "        PARA     ");
         sprintf(lcd_vram[1], "%-20s", temp);     // %-20s 左对齐且用空格补齐20格
           
         sprintf(temp, "     R=%d", sys.TMP_R);
         sprintf(lcd_vram[3], "%-20s", temp);
             
         sprintf(temp, "     R=%d", sys.TMP_K);
         sprintf(lcd_vram[4], "%-20s", temp);
         
        } else if(sys.current_page == PAGE_RECD) {
         sprintf(temp, "        RECD");
         sprintf(lcd_vram[1], "%-20s", temp);     // %-20s 左对齐且用空格补齐20格
            
         sprintf(temp, "     N=%d", sys.NAME_N);
         sprintf(lcd_vram[3], "%-20s", temp);
            
         sprintf(temp, "     MH=%.1f", sys.NAME_MH);
         sprintf(lcd_vram[4], "%-20s", temp);
            
         sprintf(temp, "     ML=%.1f", sys.NAME_ML);
         sprintf(lcd_vram[5], "%-20s", temp);

        }
    
    
    
    
    
    
    /* --- 【业务逻辑：自由发挥区 - UI 显存渲染】 --- */
    /* ================= [使用例程参考：LCD 分页动态渲染] =================
    // if(sys.current_page == PAGE_DATA) {
    //     sprintf(lcd_vram[0], "      DATA PAGE     ");
    //     
    //     // 字符串显示
    //     sprintf(temp, "  Plate:%s", sys.temp_v); 
    //     sprintf(lcd_vram[2], "%-20s", temp);     // %-20s 左对齐且用空格补齐20格
    //     
    //     // 浮点数显示
    //     sprintf(temp, "  Volt:%.2fV", sys.r37_voltage);
    //     sprintf(lcd_vram[4], "%-20s", temp);
    //     
    //     // 格式化时间显示 (补零)
    //     sprintf(temp, "  Time:%02d:%02d:%02d", sys.hour, sys.min, sys.sec);
    //     sprintf(lcd_vram[6], "%-20s", temp);
    //     
    // } else if(sys.current_page == PAGE_PARA) {
    //     sprintf(lcd_vram[0], "      PARA PAGE     ");
    //     
    //     // 整数显示
    //     sprintf(temp, "  Thr_PH:%dHz", sys.PH);
    //     sprintf(lcd_vram[3], "%-20s", temp);
    //     
    // } else if(sys.current_page == PAGE_RECD) {
    //     sprintf(lcd_vram[0], "      RECD PAGE     ");
    //     sprintf(temp, "  OverCount:%d", sys.NHA);
    //     sprintf(lcd_vram[4], "%-20s", temp);
    // }
    ================================================================ */
    
    /* --- 【模板核心：差异化防闪屏刷新 (严禁删除)】 --- */
    for(uint8_t i = 0; i < 10; i++) {
        // 利用 strcmp 比对，只有当这一行内容变了，才调用慢速的硬件 SPI 指令
        if (strcmp(lcd_vram[i], lcd_vram_bak[i]) != 0) {
            LCD_DisplayStringLine(i * 24, (uint8_t *)lcd_vram[i]);
            strcpy(lcd_vram_bak[i], lcd_vram[i]); // 同步工作显存到备份显存
        }
    }
}
