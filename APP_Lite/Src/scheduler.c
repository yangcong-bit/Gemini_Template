// scheduler.c
#include "scheduler.h"
#include "global_system.h"

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

SystemData_t sys = {
    .current_page = PAGE_DATA, 
    .res_step     = 64,        
    .pwm_freq     = 1000,      
    .pwm_duty     = 0.5f,       

    .PX           =0,
    .PD           =1000,
    .PH           =5000,
    .key_data = 0,
};

static Task_t task_list[] = {

    {Key_Proc,         10,          0}, 
    {Logic_UART_Proc,  10,          0}, 
    {Logic_LED_Proc,   20,          0}, 
    {Logic_Ctrl_Proc,  50,          0}, 
    {TIM_Proc,         20,          0}, 
    {Logic_Data_Proc,  10,          0}, 
    {MCP4017_Proc,     50,          0}, 
    {Freq_Proc,        50,          0}, 
    {Logic_UI_Proc,    100,         0}, 
    {EEPROM_Proc,      5,           0}  
};

#define TASK_NUM (sizeof(task_list) / sizeof(Task_t))

void Scheduler_Init(void) {

    UI_Init();      
    UART_Init();    
    TIM_PWM_Init(); 
    Freq_Init();    

    I2CInit();      
    EEPROM_Init();  
    MCP4017_Init(); 

    ADC_Init();     
}

void Scheduler_Run(void) {
    uint32_t current_time = HAL_GetTick(); 

    for (uint8_t i = 0; i < TASK_NUM; i++) {

        if (current_time - task_list[i].last_run >= task_list[i].rate_ms) {

            task_list[i].last_run = current_time; 

            if (task_list[i].task_func != NULL) {
                task_list[i].task_func();
            }
        }
    }
}
