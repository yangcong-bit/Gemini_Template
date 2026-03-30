// eeprom_app.c
#include "eeprom_app.h"
#include "i2c_hal.h"       
#include "global_system.h" 
#include <string.h>

#define ADDR_INIT_FLAG     0x00  
#define ADDR_LOG_IDX       0x01  
#define ADDR_LOG_START     0x08  

#define MAGIC_NUM          0x55  

void EEPROM_PushLog(LogData_t new_log) {
    uint8_t next_head = (sys.log_queue.head + 1) % LOG_Q_LEN;
    if (next_head != sys.log_queue.tail) {
        sys.log_queue.buffer[sys.log_queue.head] = new_log;
        sys.log_queue.head = next_head;
    }
}

void EEPROM_Init(void) {
    uint8_t init_flag = 0;

    eeprom_read(&init_flag, ADDR_INIT_FLAG, 1);

    if (init_flag == MAGIC_NUM) {

        eeprom_read(&sys.eeprom_log_idx, ADDR_LOG_IDX, 1);
        if (sys.eeprom_log_idx >= MAX_RECORDS) {
            sys.eeprom_log_idx = 0; 
        }

        for (int i = 0; i < MAX_RECORDS; i++) {
            uint8_t addr = ADDR_LOG_START + (i * sizeof(LogData_t));
            eeprom_read((uint8_t *)&sys.eeprom_history[i], addr, sizeof(LogData_t));
            HAL_Delay(5); 
        }
    } else {

        sys.eeprom_log_idx = 0;
        eeprom_write(&sys.eeprom_log_idx, ADDR_LOG_IDX, 1);
        HAL_Delay(5);

        memset(sys.eeprom_history, 0, sizeof(sys.eeprom_history));

        init_flag = MAGIC_NUM; 
        eeprom_write(&init_flag, ADDR_INIT_FLAG, 1);
        HAL_Delay(5);
    }
}

void EEPROM_Proc(void) {
    static uint8_t  save_state = 0;    
    static uint32_t save_timer = 0;    

    static LogData_t shadow_log;       
    static uint8_t   write_offset = 0; 

    switch (save_state) {
        case 0: 
            if (sys.log_queue.head != sys.log_queue.tail) {

                shadow_log = sys.log_queue.buffer[sys.log_queue.tail];
                sys.log_queue.tail = (sys.log_queue.tail + 1) % LOG_Q_LEN;

                sys.eeprom_history[sys.eeprom_log_idx] = shadow_log;

                write_offset = 0; 
                save_state = 1;  
            }
            break;

        case 1: 
            if (HAL_GetTick() - save_timer >= 5) {

                uint8_t *ptr = (uint8_t *)&shadow_log; 

                uint8_t addr = ADDR_LOG_START + (sys.eeprom_log_idx * sizeof(LogData_t)) + write_offset;

                eeprom_write(&ptr[write_offset], addr, 1);

                write_offset++;
                save_timer = HAL_GetTick();

                if (write_offset >= sizeof(LogData_t)) {
                    save_state = 2; 
                }
            }
            break;

        case 2: 
            if (HAL_GetTick() - save_timer >= 5) {

                sys.eeprom_log_idx = (sys.eeprom_log_idx + 1) % MAX_RECORDS;
                eeprom_write(&sys.eeprom_log_idx, ADDR_LOG_IDX, 1);

                save_timer = HAL_GetTick();
                save_state = 3;
            }
            break;

        case 3: 
            if (HAL_GetTick() - save_timer >= 5) {
                save_state = 0; 
            }
            break;
    }
}
