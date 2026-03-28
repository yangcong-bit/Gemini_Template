/**
 * @file    lcd_app.c
 * @brief   基于 VRAM 双缓存对比机制的屏幕局部刷新实现
 */
#include "lcd_app.h"
#include "lcd.h"           
#include "key_app.h"       
#include "global_system.h" 
#include <stdio.h>
#include <string.h>

/* ==========================================
 * 虚拟显存 (VRAM) 定义区
 * ========================================== */
// 蓝桥杯板载 LCD 尺寸为 10 行，每行最多 20 个 ASCII 字符。
// 二维数组尺寸加 1 用于存放字符串结束符 '\0'
static char lcd_vram[10][21];      ///< 当前计算出的最新工作显存
static char lcd_vram_bak[10][21];  ///< 上一帧显示的备份显存 (用于比对判定局部刷新)

/**
 * @brief  UI 界面与显存开机初始化
 * @note   在 main.c 的 while(1) 之前调用
 */
void UI_Init(void) {
    LCD_Init();                 
    LCD_Clear(Black);           
    LCD_SetBackColor(Black);    
    LCD_SetTextColor(White);    
    
    memset(lcd_vram, 0, sizeof(lcd_vram));
    memset(lcd_vram_bak, 0, sizeof(lcd_vram_bak));
}

///**
// * @brief  UI 页面刷新与按键交互路由任务
// * @note   建议调度周期：100ms (10FPS 视觉流畅且不吃算力)
// */
//void UI_Proc(void) {
//    uint8_t key_val = 0;
//    char temp[32]; // 暂存数组，防 sprintf 溢出

//    /* ==========================================
//     * 1. 交互控制层：消费消息队列中的积压按键
//     * ========================================== */
//    while (Key_Get_Event(&key_val)) {
//        switch (key_val) {
//            case 1: // 【B1 短按】无缝切换页面
//                sys.current_page = (sys.current_page == PAGE_DATA) ? PAGE_PARA : PAGE_DATA;
//                // 页面切换时，强制清空备份显存，触发整屏重绘，抹除旧页面的残影
//                memset(lcd_vram_bak, 0, sizeof(lcd_vram_bak)); 
//                break;
//                
//            case 2: // 【B2 短按】参数加 (带循环限幅)
//                if (sys.current_page == PAGE_PARA) {
//                    sys.v_threshold += 0.5f;
//                    if (sys.v_threshold > 3.3f) sys.v_threshold = 0.0f;
//                }
//                break;
//                
//            case 3: // 【B3 短按】PWM 占空比加
//                if (sys.current_page == PAGE_PARA) {
//                    sys.pwm_duty += 0.1f;
//                    if (sys.pwm_duty > 1.0f) sys.pwm_duty = 0.1f;
//                }
//                break;
//                
//            case 4: // 【B4 短按】触发 EEPROM 保存请求
//                sys.eeprom_save_flag = true; 
//                break;
//                
//            case 21: // 【B1 双击】调整 MCP4017 数字电位器档位
//                sys.res_step += 10;
//                if (sys.res_step > 127) sys.res_step = 0;
//                break;
//                
//            case 11: // 【B1 长按】一键恢复出厂设定并自动保存
//                sys.v_threshold = 2.5f;
//                sys.pwm_duty = 0.5f;
//                sys.res_step = 64;
//                sys.eeprom_save_flag = true; 
//                break;
//        }
//    }
//    
//    /* ==========================================
//     * 2. VRAM 渲染层：擦除画布，填充空格
//     * @note 必须填充 20 个空格。防止长字符串变短后，尾部产生旧字符残影。
//     * ========================================== */
//    for(int i = 0; i < 10; i++) {
//        sprintf(lcd_vram[i], "                    "); 
//    }

//    /* ==========================================
//     * 3. VRAM 渲染层：将字典数据格式化并写入工作显存
//     * ========================================== */
//    if (sys.current_page == PAGE_DATA) {
//        sprintf(temp, "      DATA PAGE");
//        sprintf(lcd_vram[1], "%-20s", temp); // %-20s 强制左对齐并用空格补齐 20 格
//        
//        // 直接读取解耦后的字典 RTC 时间
//        sprintf(temp, " Time: %02d:%02d:%02d", sys.hour, sys.min, sys.sec);
//        sprintf(lcd_vram[3], "%-20s", temp);
//        
//        sprintf(temp, " V37:%.2fV V38:%.2fV", sys.r37_voltage, sys.r38_voltage);
//        sprintf(lcd_vram[5], "%-20s", temp);
//        
//        sprintf(temp, " F1:%-5d F2:%-5d", (int)sys.freq_ch1, (int)sys.freq_ch2);
//        sprintf(lcd_vram[7], "%-20s", temp);
//        
//    } else if (sys.current_page == PAGE_PARA) {
//        sprintf(temp, "      PARA PAGE");
//        sprintf(lcd_vram[1], "%-20s", temp);
//        
//        sprintf(temp, " V_Thr: %.2f V", sys.v_threshold);
//        sprintf(lcd_vram[3], "%-20s", temp);
//        
//        sprintf(temp, " PWM Duty: %3.0f%%", sys.pwm_duty * 100.0f);
//        sprintf(lcd_vram[5], "%-20s", temp);
//        
//        sprintf(temp, " Res Step: %-3d", sys.res_step);
//        sprintf(lcd_vram[7], "%-20s", temp);
//    }

//    /* ==========================================
//     * 4. 底层驱动层：对比 VRAM，执行硬件局部刷新 (防闪核心)
//     * ========================================== */
//    for(uint8_t i = 0; i < 10; i++) {
//        // 利用 strcmp 高效比对当前行是否发生变化
//        if (strcmp(lcd_vram[i], lcd_vram_bak[i]) != 0) {
//            
//            // 发生变化，才调用慢速的底层硬件 SPI 写屏幕指令 (行高为 24 像素)
//            LCD_DisplayStringLine(i * 24, (uint8_t *)lcd_vram[i]); 
//            
//            // 将工作显存同步回备份显存，为下一帧做准备
//            strcpy(lcd_vram_bak[i], lcd_vram[i]);
//        }
//    }
//}
