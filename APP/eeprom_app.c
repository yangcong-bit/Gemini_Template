/* eeprom_app.c */
#include "eeprom_app.h"
#include "i2c_hal.h"  
#include <string.h>

/* --- EEPROM 地址通用分配图 --- */
#define ADDR_INIT_FLAG     0x00  // 1 字节 (魔法数字)
#define ADDR_LOG_IDX       0x01  // 1 字节 (记录环形栈写到了第几个槽位)
#define ADDR_LOG_START     0x08  // 环形栈起点，避开前面的参数区

#define MAGIC_NUM          0x55  

// 实体定义：供全系统随时查阅的 RAM 历史记录镜像
LogData_t eeprom_history[MAX_RECORDS];
uint8_t   eeprom_log_idx = 0; 

/**
 * @brief  【万能推入接口】把整个结构体打包压入 RAM 队列
 */
void EEPROM_PushLog(LogData_t new_log) {
    uint8_t next_head = (sys.log_queue.head + 1) % LOG_Q_LEN;
    
    if (next_head != sys.log_queue.tail) {
        sys.log_queue.buffer[sys.log_queue.head] = new_log;
        sys.log_queue.head = next_head;
    }
}

/**
 * @brief  开机初始化 (含自适应读取功能)
 */
void EEPROM_Init(void) {
    uint8_t init_flag = 0;
    
    // 1. 读取是否为出厂第一次上电
    eeprom_read(&init_flag, ADDR_INIT_FLAG, 1);
    
    if (init_flag == MAGIC_NUM) {
        // --- 已经初始化过，启动【自适应读取】 ---
        
        // 读取上次掉电时的环形栈指针
        eeprom_read(&eeprom_log_idx, ADDR_LOG_IDX, 1);
        if (eeprom_log_idx >= MAX_RECORDS) eeprom_log_idx = 0; // 防呆容错
        
        // 【核心】根据 sizeof() 自动将所有历史记录块搬运回 RAM 中
        for (int i = 0; i < MAX_RECORDS; i++) {
            uint8_t addr = ADDR_LOG_START + (i * sizeof(LogData_t));
            eeprom_read((uint8_t *)&eeprom_history[i], addr, sizeof(LogData_t));
            HAL_Delay(5); // 稍微延时，防总线拥堵
        }
    } else {
        // --- 首次上电，格式化 EEPROM ---
        eeprom_log_idx = 0;
        eeprom_write(&eeprom_log_idx, ADDR_LOG_IDX, 1);
        HAL_Delay(5);
        
        memset(eeprom_history, 0, sizeof(eeprom_history));
        
        init_flag = MAGIC_NUM;
        eeprom_write(&init_flag, ADDR_INIT_FLAG, 1);
        HAL_Delay(5);
    }
}

/**
 * @brief  万能非阻塞 EEPROM 烧录状态机 (自适应大小，单字节切片写入防跨页)
 * @note   建议调度周期：5ms
 */
void EEPROM_Proc(void) {
    static uint8_t  save_state = 0;
    static uint32_t save_timer = 0;
    
    static LogData_t shadow_log;       // 拍摄当前正在写入的数据快照
    static uint8_t   write_offset = 0; // 记录结构体切片写到了第几个字节
    
    switch (save_state) {
        case 0: // 空闲等待
            if (sys.log_queue.head != sys.log_queue.tail) {
                // 出队一个最新的记录
                shadow_log = sys.log_queue.buffer[sys.log_queue.tail];
                sys.log_queue.tail = (sys.log_queue.tail + 1) % LOG_Q_LEN;
                
                // 1. 立即同步更新到 RAM 镜像中，让 UI 界面立刻显示最新数据
                eeprom_history[eeprom_log_idx] = shadow_log;
                
                // 2. 准备后台烧录 EEPROM
                write_offset = 0; 
                save_state = 1;  
            }
            break;
            
        case 1: // 【切片写入核心】：每 5ms 唤醒一次，每次只搬运 1 个字节
            if (HAL_GetTick() - save_timer >= 5) {
                
                // 强制将结构体当成单字节数组
                uint8_t *ptr = (uint8_t *)&shadow_log; 
                
                // 自动计算这个字节在 EEPROM 中的绝对地址
                uint8_t addr = ADDR_LOG_START + (eeprom_log_idx * sizeof(LogData_t)) + write_offset;
                
                // 写入这 1 个字节
                eeprom_write(&ptr[write_offset], addr, 1);
                
                // 偏移量推进
                write_offset++;
                save_timer = HAL_GetTick();
                
                // 判断整个结构体是否搬运完毕？
                if (write_offset >= sizeof(LogData_t)) {
                    save_state = 2; // 写完了，去更新系统索引
                }
            }
            break;
            
        case 2: // 更新 EEPROM 中的环形栈指针
            if (HAL_GetTick() - save_timer >= 5) {
                // 槽位指针滚动: 0->1->2->3->4->0
                eeprom_log_idx = (eeprom_log_idx + 1) % MAX_RECORDS;
                eeprom_write(&eeprom_log_idx, ADDR_LOG_IDX, 1);
                
                save_timer = HAL_GetTick();
                save_state = 3;
            }
            break;
            
        case 3: // 释放状态机
            if (HAL_GetTick() - save_timer >= 5) {
                save_state = 0; 
            }
            break;
    }
}
