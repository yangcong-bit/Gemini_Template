/**
 * @file    scheduler.c
 * @brief   时间片轮询调度引擎与系统实例化中心
 * @note    【调度精度说明】：
 * 本调度器基于 SysTick (通常为 1ms) 的 HAL_GetTick()。
 * 因为是非抢占式协作型架构，实际任务执行周期的误差 = 系统中最长单次阻塞任务的耗时。
 * 因此，业务代码中【绝对禁止】使用死循环或 HAL_Delay()！
 */

#include "scheduler.h"
#include "global_system.h"

/* --- 引入所有受支持业务模块的处理函数头文件 --- */
#include "key_app.h"
#include "led_app.h"
#include "lcd_app.h"
#include "uart_app.h"
#include "eeprom_app.h"
#include "tim_app.h"
#include "freq_app.h"
#include "mcp4017_app.h"
#include "i2c_hal.h"
#include "adc_app.h"
#include "exam_logic.h"

/* ==========================================
 * 核心数据字典物理实例化 (整个系统唯一真实的内存空间)
 * ========================================== */
SystemData_t sys = {
    .current_page = PAGE_DATA, ///< 默认开机处于数据监控界面
    .v_threshold  = 2.5f,      ///< 默认电压报警阈值 (2.5V)
    .f_threshold  = 1000,      ///< 默认频率报警阈值 (1000Hz)
    .res_step     = 64,        ///< 默认可编程电阻档位 (居中值，约50kΩ)
    .pwm_freq     = 1000,      ///< 默认 PWM 目标频率 (1kHz)
    .pwm_duty     = 0.5f       ///< 默认 PWM 目标占空比 (50%)
    // 注：其余如数组、队列索引、标志位等，C标准会自动将其初始化为 0 或 false。
};

/* ==========================================
 * 系统任务注册表 (Task Registry)
 * @note 根据项目需求，在这里静态分配任务执行周期。
 * 优先级原则：对时间敏感的(如按键、通信)周期短，对视觉反馈的(如LCD)周期长。
 * ========================================== */
static Task_t task_list[] = {
    // 任务入口函数       执行周期(ms)  时间戳初始值
    {Key_Proc,         10,          0}, ///< [交互] 按键扫描状态机，10ms 保证跟手且有效消抖
    {Logic_UART_Proc,  10,          0}, ///< [通信] 串口协议解析检测，10ms 处理 DMA 缓冲防堆积
    {Logic_LED_Proc,   20,          0}, ///< [UI]   LED 状态硬件刷新，20ms 响应足够迅速
		{Logic_Ctrl_Proc,  50,      		0}, ///< [按键] 按键消费与业务逻辑路由 (50ms，极速响应)
    {TIM_Proc,         20,          0}, ///< [外设] PWM 动态重装载任务，20ms 响应频占变化
    {Logic_Data_Proc,  50,          0}, ///< [核心] ADC、RTC 及全局数据结算大本营，50ms
    {MCP4017_Proc,     50,          0}, ///< [外设] I2C 可编程电阻动态阻值同步，50ms
    {Freq_Proc,        50,          0}, ///< [外设] 测频占空比防溢出结算状态机，50ms
    {Logic_UI_Proc,    100,         0}, ///< [UI]   LCD 局部查表重绘，100ms (10FPS 人眼流畅)
    {EEPROM_Proc,      5,           0}  ///< [极速] EEPROM I2C 后台极速单字节切片烧录机，5ms
};

// 自动计算注册表中的任务总数
#define TASK_NUM (sizeof(task_list) / sizeof(Task_t))

/* ==========================================
 * 调度器对外引擎实现
 * ========================================== */

/**
 * @brief  系统级开机硬件与外设总栈初始化
 * @note   在 main.c 的 while(1) 之前调用。必须严格遵从启动顺序依赖。
 */
void Scheduler_Init(void) {
    
    // 1. UI 与通信层初始化 (不涉及慢速总线)
    UI_Init();      // 内含 LCD 画板清屏
    UART_Init();    // 启动 IDLE 空闲中断 + DMA 接收监听
    TIM_PWM_Init(); // 启动定时器 PWM 输出引脚
    Freq_Init();    // 启动定时器输入捕获中断
    
    // 2. 总线型设备初始化 (极度依赖 GPIO 的稳态)
    I2CInit();      // 初始化 I2C 软件模拟管脚
    EEPROM_Init();  // 执行开机读取及历史数据自适应恢复机制
    MCP4017_Init(); // 强制刷入开机默认阻值
    
    // 3. 模拟采集层启动
    ADC_Init();     // 启动两路 ADC 的 DMA 底层搬运机制
}

/**
 * @brief  时间片调度器主引擎 (Main Loop)
 * @note   放置在 main 函数的 while(1) 中死循环运行。
 */
void Scheduler_Run(void) {
    uint32_t current_time = HAL_GetTick(); // 抓取底层硬件毫秒滴答数

    for (uint8_t i = 0; i < TASK_NUM; i++) {
        
        // 判断当前时间戳与上次执行的差值，是否达到了期望的执行周期
        if (current_time - task_list[i].last_run >= task_list[i].rate_ms) {
            
            // 刷新该任务的最新执行时间戳
            task_list[i].last_run = current_time; 
            
            // 安全断言：函数指针非空则执行状态机
            if (task_list[i].task_func != NULL) {
                task_list[i].task_func();
            }
        }
    }
}
