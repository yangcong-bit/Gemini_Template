/* global_system.h */
#ifndef __GLOBAL_SYSTEM_H
#define __GLOBAL_SYSTEM_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include <stdint.h>
#include <stdbool.h>

#define KEY_QUEUE_LEN  16  // 定义按键队列深度，16 个缓存对人工按键绰绰有余

/* 定义简单的环形队列结构体 */
typedef struct {
    uint8_t  buffer[KEY_QUEUE_LEN]; // 消息体
    uint16_t head;                  // 队头指针（写）
    uint16_t tail;                  // 队尾指针（读）
} KeyQueue_t;

/* 1. 定义 UI 页面枚举（状态机思想） */
typedef enum {
    PAGE_DATA = 0,   // 数据实时监控界面
    PAGE_PARA,       // 参数设置界面
} PageState_e;

/* 2. 核心数据交互结构体 */
typedef struct {
    /* --- 系统运行与 UI 状态 --- */
    PageState_e current_page;  // 当前屏幕所处页面
    
    /* --- 传感器/硬件采集数据 --- */
    float    r37_voltage;      // 蓝桥杯板载电位器 R37 的转换电压
		float    r38_voltage;      // 蓝桥杯板载电位器 R38 的转换电压
    
    /* --- 用户设置参数 (通常需要掉电保存到 EEPROM) --- */
    float    v_threshold;      // 电压报警阈值
    uint32_t f_threshold;      // 频率报警阈值
    
    /* --- 跨模块事件标志位 --- */
		// --- 按键 控制标志 ---
    KeyQueue_t key_queue;      // 按键 FIFO 消息队列
	
		// --- EEPROM 控制标志 ---
    bool eeprom_save_flag;  // 请求保存参数到 EEPROM 的标志位
	
		/* --- I2C 外设控制参数 --- */
    uint8_t  res_step;         // MCP4017 可编程电阻步进值 (范围 0 ~ 127)
	
		// --- 串口通信交互标志 ---
		bool     uart_rx_ready;    // 收到完整指令帧的标志
    // (如果需要保存解析后的特定字符串，也可以加在这里)
		
		// --- PWM 输出控制参数 ---
    uint32_t pwm_freq;   // 目标输出频率 (Hz)
    float    pwm_duty;   // 目标占空比 (0.0f ~ 1.0f，例如 0.5 代表 50%)
		
		// --- 双通道测频数据 ---
    uint32_t freq_ch1;  // Signal 1 频率 (PA15 / TIM2)
    uint32_t freq_ch2;  // Signal 2 频率 (PB4 / TIM3)
} SystemData_t;

// 外部声明，由 global_system.c 或 main.c 实例化
extern SystemData_t sys;

#endif
