/**
 * @file    global_system.h
 * @brief   全局系统数据字典中心 (Single Source of Truth)
 * @note    本文件定义了整个工程唯一的数据交互中心 sys 实体。
 * 【铁血纪律】：严禁在业务模块间使用 extern 互相窃取变量！
 * 所有跨模块的数据读写、状态传递，必须通过 sys 字典进行。
 */

#ifndef __GLOBAL_SYSTEM_H
#define __GLOBAL_SYSTEM_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>


/* ==========================================================
 * 【核心配置】底层外设句柄与通道映射宏 (CubeMX 改了之后只改这里！)
 * ========================================================== */

// ---------------- [1. PWM 输出映射] ----------------
#define PWM_TIM_HANDLE    htim2             // PWM所用定时器句柄
#define PWM_TIM_CHANNEL   TIM_CHANNEL_2     // PWM所用通道

// ---------------- [2. ADC 采集映射] ----------------
#define ADC_R38_HANDLE    hadc1             // R38电位器对应的ADC句柄
#define ADC_R37_HANDLE    hadc2             // R37电位器对应的ADC句柄

// ---------------- [3. 串口通信映射] ----------------
#define UART_APP_HANDLE   huart1            // 业务串口句柄
#define UART_APP_INST     USART1            // 业务串口实例 (用于中断判定)

// ---------------- [4. 频率捕获 CH1 映射 (如 PA1)] ----------------
#define FREQ_CH1_HANDLE   htim3             // CH1所用定时器句柄
#define FREQ_CH1_INST     TIM3              // CH1所用定时器实例
#define FREQ_CH1_CH_MAIN  TIM_CHANNEL_1     // CH1测周期的主通道
#define FREQ_CH1_CH_SUB   TIM_CHANNEL_2     // CH1测高电平的副通道
#define FREQ_CH1_ACTIVE   HAL_TIM_ACTIVE_CHANNEL_1 // 中断回调判断通道

// ---------------- [5. 频率捕获 CH2 映射 (如 PA6)] ----------------
#define FREQ_CH2_HANDLE   htim3             // CH2所用定时器句柄
#define FREQ_CH2_INST     TIM3              // CH2所用定时器实例
#define FREQ_CH2_CH_MAIN  TIM_CHANNEL_1     // CH2测周期的主通道
#define FREQ_CH2_CH_SUB   TIM_CHANNEL_2     // CH2测高电平的副通道
#define FREQ_CH2_ACTIVE   HAL_TIM_ACTIVE_CHANNEL_1 // 中断回调判断通道

/* ==========================================
 * 系统常量与队列深度定义区
 * ========================================== */
#define LOG_Q_LEN      8   ///< EEPROM 异步写入缓冲队列深度
#define KEY_QUEUE_LEN  16  ///< 按键事件缓冲队列深度 (16个缓存对人工按键绰绰有余)
#define MAX_RECORDS    5   ///< EEPROM 环形栈最大记录数 (根据比赛题目要求调整)

/* ==========================================
 * 数据结构体定义区
 * ========================================== */

/**
 * @brief EEPROM 历史记录结构体
 * @note  按需修改内部成员，代表每一次需要掉电保存的一组数据帧
 */
typedef struct {
    uint8_t  hour;    ///< [0~23] 记录触发时的小时
    uint8_t  min;     ///< [0~59] 记录触发时的分钟
    uint8_t  sec;     ///< [0~59] 记录触发时的秒钟
    float    volt;    ///< [0.00~3.30 V] 记录触发时的电压状态
    uint32_t freq;    ///< [0~100000 Hz] 记录触发时的频率状态
} LogData_t;

/**
 * @brief 待写入 EEPROM 的异步缓冲队列
 * @note  采用环形队列机制。生产者(如按键)推入，消费者(EEPROM后台状态机)切片取出，物理防阻塞。
 */
typedef struct {
    LogData_t buffer[LOG_Q_LEN]; ///< 队列数据存储区
    uint8_t   head;              ///< 队头指针（写数据点）
    uint8_t   tail;              ///< 队尾指针（读数据点）
} LogQueue_t;

/**
 * @brief 跨模块按键事件环形队列
 */
typedef struct {
    uint8_t  buffer[KEY_QUEUE_LEN]; ///< 存放键值 (例如: 1为短按, 11为长按, 21为双击)
    uint16_t head;                  ///< 队头指针（写）
    uint16_t tail;                  ///< 队尾指针（读）
} KeyQueue_t;

/**
 * @brief UI 页面枚举（状态机流转节点）
 */
typedef enum {
    PAGE_DATA = 0,   ///< 数据实时监控界面
    PAGE_PARA,       ///< 参数设置界面
    PAGE_RECD        ///< 统计界面 
} PageState_e;

/* ==========================================
 * 核心全局数据字典 (System Data Dictionary)
 * ========================================== */
/**
 * @brief 统管全系统运行状态的核心大脑
 */
typedef struct {
    /* --- 1. 系统运行与 UI 状态 --- */
    PageState_e current_page;  ///< 当前 LCD 屏幕所处的页面状态
    uint8_t     led_ctrl[8];   ///< [0或1] LED 控制映射数组 (0:灭, 1:亮，[0]对应硬件LD1)
    
    // RTC 时间镜像 (彻底解耦底层 RTC_TimeTypeDef，防止影子寄存器死锁)
    uint8_t     hour;          ///< [0~23] 硬件实时小时
    uint8_t     min;           ///< [0~59] 硬件实时分钟
    uint8_t     sec;           ///< [0~59] 硬件实时秒钟

    /* --- 2. 传感器/硬件实时采集数据 (只读，由底层采集模块循环更新) --- */
    float    r37_voltage;      ///< [0.00~3.30 V] R37 蓝桥杯板载电位器的实时电压
    float    r38_voltage;      ///< [0.00~3.30 V] R38 蓝桥杯板载电位器的实时电压
    
    // 双通道输入捕获数据
    uint32_t freq_ch1;         ///< [0~100000 Hz] 通道1 实时输入频率
    uint32_t period_ch1;       ///< [us]          通道1 实时信号周期
    float    duty_ch1;         ///< [0.0~100.0 %] 通道1 实时高电平占空比
    
    uint32_t freq_ch2;         ///< [0~100000 Hz] 通道2 实时输入频率
    uint32_t period_ch2;       ///< [us]          通道2 实时信号周期
    float    duty_ch2;         ///< [0.0~100.0 %] 通道2 实时高电平占空比

    /* --- 3. 用户设置参数与外设控制指令 (读写) --- */
    uint8_t  res_step;         ///< [0~127]       MCP4017 可编程电阻步进值 (0~100kΩ)
    
    uint32_t pwm_freq;         ///< [Hz]          PWM 目标输出频率指令
    float    pwm_duty;         ///< [0.0~1.0]     PWM 目标占空比指令 (例如 0.5 代表 50%)
        
    /* --- 4. 跨模块事件队列与 EEPROM 历史记录缓冲区 --- */
    KeyQueue_t  key_queue;     ///< 按键消息队列 (缓冲用户操作，防丢失)
    LogQueue_t  log_queue;     ///< 等待异步切片写入 EEPROM 的后台日志队列
    
    // 内存数据镜像：供 UI 页面直接秒读显示，无需等待低速 I2C 总线
    LogData_t   eeprom_history[MAX_RECORDS]; ///< 历史记录全量 RAM 镜像
    uint8_t     eeprom_log_idx;              ///< [0~(MAX_RECORDS-1)] 指示当前最新数据所在的环形槽位

    /* --- 5. 系统行为控制标志与软定时器 --- */
    bool     eeprom_save_flag; ///< [触发型] 请求将当前参数打包压入 EEPROM 队列的标志位
    bool     uart_rx_ready;    ///< [触发型] 串口收到一帧完整 DMA 数据的就绪标志位
    
    float NAME_V;
    bool NAME_M;
    float NAME_P;
    float NAME_MH;
    float NAME_ML;

    
    float    start_freq;       // 过渡起点频率
    float    target_freq;      // 过渡终点频率
    
         
    bool LED;
    bool para_select;
    bool is_locked;
    bool M_flag;
    
    uint8_t NAME_R;
    uint8_t TMP_R;
    uint8_t NAME_K;
    uint8_t TMP_K;
    
    uint32_t NAME_N;
        

} SystemData_t;

/* 外部声明，整个工程仅在 scheduler.c 中实例化唯一的一份物理内存 */
extern SystemData_t sys;

#endif /* __GLOBAL_SYSTEM_H */
