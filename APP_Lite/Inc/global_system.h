// global_system.h
#ifndef __GLOBAL_SYSTEM_H
#define __GLOBAL_SYSTEM_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

#define LOG_Q_LEN      8   
#define KEY_QUEUE_LEN  16  
#define MAX_RECORDS    5   

typedef struct {
    uint8_t  hour;    
    uint8_t  min;     
    uint8_t  sec;     
    float    volt;    
    uint32_t freq;    
} LogData_t;

typedef struct {
    LogData_t buffer[LOG_Q_LEN]; 
    uint8_t   head;              
    uint8_t   tail;              
} LogQueue_t;

typedef struct {
    uint8_t  buffer[KEY_QUEUE_LEN]; 
    uint16_t head;                  
    uint16_t tail;                  
} KeyQueue_t;

typedef enum {
    PAGE_DATA = 0,   
    PAGE_PARA,       
    PAGE_RECD        
} PageState_e;

typedef struct {

    PageState_e current_page;  
    uint8_t     led_ctrl[8];   

    uint8_t     hour;          
    uint8_t     min;           
    uint8_t     sec;           

    float    r37_voltage;      
    float    r38_voltage;      

    uint32_t freq_ch1;         
    uint32_t period_ch1;       
    float    duty_ch1;         

    uint32_t freq_ch2;         
    uint32_t period_ch2;       
    float    duty_ch2;         

    uint8_t  res_step;         

    uint32_t pwm_freq;         
    float    pwm_duty;         

    KeyQueue_t  key_queue;     
    LogQueue_t  log_queue;     

    LogData_t   eeprom_history[MAX_RECORDS]; 
    uint8_t     eeprom_log_idx;              

    bool     eeprom_save_flag; 
    bool     uart_rx_ready;    

    uint8_t  led8_timer;       

    float FA;
    float FB;
    bool flag;
    bool Alog;
    bool Blog;

    int32_t PX;
    uint32_t PD;
    uint32_t PH;

    uint32_t NDA;
    uint32_t NDB;
    uint32_t MIN_NDA;
    uint32_t MIN_NDB;
    uint32_t MAX_NDA;
    uint32_t MAX_NDB;

    uint32_t NHA;
    uint32_t NHB;

    uint8_t key_data;

} SystemData_t;

extern SystemData_t sys;

#endif
