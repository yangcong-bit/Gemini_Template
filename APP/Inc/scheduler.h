/**
 * @file    scheduler.h
 * @brief   基于时间片的协作式调度器 (微型 OS 内核)
 * @note    提供任务注册表结构与调度引擎对外 API。
 */

#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include "main.h"

/**
 * @brief 任务函数指针类型
 */
typedef void (*TaskFunc_t)(void);

/**
 * @brief 任务控制块结构体 (Task Control Block, TCB)
 */
typedef struct {
    TaskFunc_t task_func;  ///< 具体的业务状态机执行函数
    uint32_t rate_ms;      ///< 任务执行的期望周期 (单位: 毫秒)
    uint32_t last_run;     ///< 记录上次执行完毕时的系统时间戳 (基于 HAL_GetTick)
} Task_t;

/* ==========================================
 * 调度器对外 API 接口
 * ========================================== */
void Scheduler_Init(void);
void Scheduler_Run(void);

#endif /* __SCHEDULER_H */
