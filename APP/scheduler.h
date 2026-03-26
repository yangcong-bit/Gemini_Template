/* scheduler.h */
#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include "main.h"

// 定义任务函数指针类型
typedef void (*TaskFunc_t)(void);

// 任务控制块结构体
typedef struct {
    TaskFunc_t task_func;  // 具体的任务执行函数
    uint32_t rate_ms;      // 任务执行周期 (单位: ms)
    uint32_t last_run;     // 记录上次执行的系统时间戳
} Task_t;

// 调度器对外接口
void Scheduler_Init(void);
void Scheduler_Run(void);

#endif
