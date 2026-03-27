/* scheduler.c */
#include "scheduler.h"
#include "global_system.h"

// 引入各个业务模块的处理函数头文件
#include "key_app.h"
#include "led_app.h"
#include "data_app.h"
#include "lcd_app.h"
#include "uart_app.h"
#include "eeprom_app.h"
#include "tim_app.h"
#include "freq_app.h"
#include "mcp4017_app.h"
#include "i2c_hal.h"
#include "adc_app.h"

// 实例化全局数据字典（整个系统的数据都在这里）
SystemData_t sys = {
    .current_page = PAGE_DATA,
    .v_threshold = 2.5f,
    .f_threshold = 1000,
		.key_event_ui = 0,    // 替换为 UI 信箱
    .key_event_ctrl = 0   // 新增控制信箱
};

/* * 任务注册表
 * 说明：根据比赛/项目需求，动态调整执行频率
 */
static Task_t task_list[] = {
    {Key_Proc,    10,  0},   // 按键扫描，10ms一次，保证灵敏且能有效消抖
    {LED_Proc,    20,  0},   // LED状态刷新，20ms一次
    {Data_Proc,   50,  0},   // ADC及测频等数据处理，50ms一次
		{TIM_Proc,    20,  0},   // PWM 动态刷新任务，20ms 一次响应足够迅速
		{MCP4017_Proc, 50,  0},  // 新增：MCP4017 阻值动态监控，50ms一次
    {UI_Proc,     100, 0},   // 屏幕刷新，100ms一次（10FPS足够，且肉眼不觉闪烁）
		{EEPROM_Proc, 10,  0},   // 请求保存参数到 EEPROM 的标志位
		{Freq_Proc,   50,  0},   // 新增：测频数据处理任务，50ms 算一次
    {UART_Proc,   10,  0}    // 串口协议解析，10ms一次
};

// 计算注册表中的任务总数
#define TASK_NUM (sizeof(task_list)/sizeof(Task_t))

/**
 * @brief  系统级初始化
 * @note   在 main 函数的 while(1) 之前调用
 */
void Scheduler_Init(void) {
    // 此处可调用各个底层模块的初始化函数
    //LCD_Init();
    LCD_Clear(Black);
    LCD_SetBackColor(Black);
    LCD_SetTextColor(White);
    
    UI_Init();
    UART_Init();
    TIM_PWM_Init();
    Freq_Init(); 
    
    //在此处调用官方 I2C 的 GPIO 初始化，必须在操作 EEPROM/MCP4017 之前
    I2CInit(); 
    EEPROM_Init();
    MCP4017_Init();
    
    //在此处开启 ADC 的 DMA 硬件搬运机制
    ADC_Init();
}

/**
 * @brief  时间片调度器主引擎
 * @note   放在 main 函数的 while(1) 中不断轮询
 */
void Scheduler_Run(void) {
    uint32_t current_time = HAL_GetTick(); // 获取系统毫秒滴答数

    for (uint8_t i = 0; i < TASK_NUM; i++) {
        // 判断是否到达任务指定的执行周期
        if (current_time - task_list[i].last_run >= task_list[i].rate_ms) {
            task_list[i].last_run = current_time; // 更新时间戳
            
            // 安全调用任务函数
            if (task_list[i].task_func != NULL) {
                task_list[i].task_func();
            }
        }
    }
}
