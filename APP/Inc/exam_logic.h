/**
 * @file    exam_logic.h
 * @brief   考场业务逻辑总览头文件
 */
#ifndef __EXAM_LOGIC_H
#define __EXAM_LOGIC_H

#include "main.h"

/* ==========================================
 * 考场业务调度任务暴露接口
 * 请将这些函数注册到 scheduler.c 的 task_list 中
 * ========================================== */

void Logic_Ctrl_Proc(void);  // 控制层：处理按键消费与业务流转
void Logic_Data_Proc(void);  // 模型层：传感器采集与全局数据计算
void Logic_LED_Proc(void);   // 视图层：LED 状态指示
void Logic_UART_Proc(void);  // 控制层：串口协议解析
void Logic_UI_Proc(void);    // 视图层：LCD 屏幕显存渲染引擎

#endif /* __EXAM_LOGIC_H */
