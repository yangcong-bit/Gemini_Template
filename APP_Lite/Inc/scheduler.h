// scheduler.h
#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include "main.h"

typedef void (*TaskFunc_t)(void);

typedef struct {
    TaskFunc_t task_func;  
    uint32_t rate_ms;      
    uint32_t last_run;     
} Task_t;

void Scheduler_Init(void);
void Scheduler_Run(void);

#endif
