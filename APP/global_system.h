/* global_system.h */
#ifndef __GLOBAL_SYSTEM_H
#define __GLOBAL_SYSTEM_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

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
    uint32_t ic_frequency;     // 定时器输入捕获测量的信号频率 (Hz)
    
    /* --- 用户设置参数 (通常需要掉电保存到 EEPROM) --- */
    float    v_threshold;      // 电压报警阈值
    uint32_t f_threshold;      // 频率报警阈值
    
    /* --- 跨模块事件标志位 --- */
    uint8_t  key_event;        // 按键事件 (例如：1代表B1短按，11代表B1长按)
    bool     uart_rx_ready;    // 串口指令接收完成标志
} SystemData_t;

// 外部声明，由 global_system.c 或 main.c 实例化
extern SystemData_t sys;

#endif
