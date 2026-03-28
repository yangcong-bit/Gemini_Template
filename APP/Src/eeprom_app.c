/**
 * @file    eeprom_app.c
 * @brief   非阻塞 EEPROM 单字节切片写入状态机实现
 */
#include "eeprom_app.h"
#include "i2c_hal.h"       // 引入底层的 I2C 读写接口
#include "global_system.h" // 引入全局数据字典
#include <string.h>

/* ==========================================
 * EEPROM 物理地址通用分配图 (AT24C02 共 256 字节)
 * ========================================== */
#define ADDR_INIT_FLAG     0x00  ///< [1 字节] 魔法数字，判断是否出厂首次上电
#define ADDR_LOG_IDX       0x01  ///< [1 字节] 记录环形栈目前写到了第几个槽位
#define ADDR_LOG_START     0x08  ///< 历史记录环形栈起点 (避开前面的系统参数区)

#define MAGIC_NUM          0x55  ///< 标志着 EEPROM 已经被格式化过的特征码

/**
 * @brief  【生产者接口】把整个结构体打包压入异步写队列
 * @param  new_log: 准备保存的最新数据块快照
 */
void EEPROM_PushLog(LogData_t new_log) {
    uint8_t next_head = (sys.log_queue.head + 1) % LOG_Q_LEN;
    if (next_head != sys.log_queue.tail) {
        sys.log_queue.buffer[sys.log_queue.head] = new_log;
        sys.log_queue.head = next_head;
    }
}

/**
 * @brief  开机初始化 (含自适应全量读取功能)
 * @note   允许阻塞。自动将 I2C 中的慢速历史记录全部搬运至全局字典内存中。
 */
void EEPROM_Init(void) {
    uint8_t init_flag = 0;
    
    // 1. 读取 0x00 地址，判断是否为出厂第一次上电
    eeprom_read(&init_flag, ADDR_INIT_FLAG, 1);
    
    if (init_flag == MAGIC_NUM) {
        // --- 已经格式化过，启动【自适应恢复】 ---
        
        eeprom_read(&sys.eeprom_log_idx, ADDR_LOG_IDX, 1);
        if (sys.eeprom_log_idx >= MAX_RECORDS) {
            sys.eeprom_log_idx = 0; // 防呆容错
        }
        
        // 批量搬运回 sys 字典。使得 UI 界面可以瞬间读取渲染，不用等 I2C。
        for (int i = 0; i < MAX_RECORDS; i++) {
            uint8_t addr = ADDR_LOG_START + (i * sizeof(LogData_t));
            eeprom_read((uint8_t *)&sys.eeprom_history[i], addr, sizeof(LogData_t));
            HAL_Delay(5); // AT24C02 连续读写时需稍微延时，防总线拥堵
        }
    } else {
        // --- 首次上电，格式化 EEPROM ---
        sys.eeprom_log_idx = 0;
        eeprom_write(&sys.eeprom_log_idx, ADDR_LOG_IDX, 1);
        HAL_Delay(5);
        
        memset(sys.eeprom_history, 0, sizeof(sys.eeprom_history));
        
        init_flag = MAGIC_NUM; // 写入魔法数字
        eeprom_write(&init_flag, ADDR_INIT_FLAG, 1);
        HAL_Delay(5);
    }
}

/**
 * @brief  万能非阻塞 EEPROM 烧录状态机 (消费者)
 * @note   建议调度周期：5ms。
 * AT24C02 每写入一页或一个字节，内部烧录需要等待 5ms。
 * 该状态机会在后台将结构体拆分成单字节，每 5ms 写入一个，彻底避免主循环卡死。
 */
void EEPROM_Proc(void) {
    static uint8_t  save_state = 0;    // 状态机主节点
    static uint32_t save_timer = 0;    // 软定时器时间戳
    
    static LogData_t shadow_log;       // 拍摄当前正在写入的数据快照
    static uint8_t   write_offset = 0; // 记录结构体切片写到了第几个字节
    
    switch (save_state) {
        case 0: // 【状态 0】：空闲等待，监控存储队列
            if (sys.log_queue.head != sys.log_queue.tail) {
                
                // 1. 提取记录并出队
                shadow_log = sys.log_queue.buffer[sys.log_queue.tail];
                sys.log_queue.tail = (sys.log_queue.tail + 1) % LOG_Q_LEN;
                
                // 2. 立即更新 RAM 镜像，使得 UI 立刻显示，无需等待慢速烧录
                sys.eeprom_history[sys.eeprom_log_idx] = shadow_log;
                
                // 3. 点亮指示灯 (赋生命值，自动递减熄灭)
                sys.led8_timer = 10; 
                
                // 4. 跳转至烧录状态
                write_offset = 0; 
                save_state = 1;  
            }
            break;
            
        case 1: // 【状态 1】：单字节切片写入核心 (每 5ms 唤醒一次)
            if (HAL_GetTick() - save_timer >= 5) {
                
                // 将结构体强制转换为单字节指针
                uint8_t *ptr = (uint8_t *)&shadow_log; 
                
                // 计算目标物理首地址
                uint8_t addr = ADDR_LOG_START + (sys.eeprom_log_idx * sizeof(LogData_t)) + write_offset;
                
                // 调用底层写入 1 个字节
                eeprom_write(&ptr[write_offset], addr, 1);
                
                write_offset++;
                save_timer = HAL_GetTick();
                
                if (write_offset >= sizeof(LogData_t)) {
                    save_state = 2; // 当前结构体全量写完，进入下一步
                }
            }
            break;
            
        case 2: // 【状态 2】：更新 EEPROM 中的环形栈索引
            if (HAL_GetTick() - save_timer >= 5) {
                
                // 槽位指针滚动: 0->1->2->3->4->0
                sys.eeprom_log_idx = (sys.eeprom_log_idx + 1) % MAX_RECORDS;
                eeprom_write(&sys.eeprom_log_idx, ADDR_LOG_IDX, 1);
                
                save_timer = HAL_GetTick();
                save_state = 3;
            }
            break;
            
        case 3: // 【状态 3】：冷却释放
            if (HAL_GetTick() - save_timer >= 5) {
                save_state = 0; 
            }
            break;
    }
}
