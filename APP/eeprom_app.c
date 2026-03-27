/* eeprom_app.c */
#include "eeprom_app.h"
#include "i2c_hal.h"  // 引入你模板里自带的底层 I2C/EEPROM 驱动

/* * 内存地址分配 (严格遵循 8 字节对齐，防止 Page Write 跨页回卷错乱)
 * AT24C02 容量为 256 字节 (0x00 ~ 0xFF)
 */
#define ADDR_INIT_FLAG  0x00  // 占用 1 字节 (标志位：判断芯片是否是出厂第一次上电)
#define ADDR_V_THR      0x08  // 占用 4 字节 (存放 float v_threshold)
#define ADDR_F_THR      0x10  // 占用 4 字节 (存放 uint32_t f_threshold)

#define MAGIC_NUM       0x55  // “魔法数字”，如果 0x00 地址读出来是它，说明参数已经初始化过

/**
 * @brief  开机初始化读取 EEPROM
 * @note   在 main.c 的 while(1) 之前调用一次
 */
void EEPROM_Init(void) {
    uint8_t init_flag = 0;
    
    // 1. 读取标志位
    eeprom_read(&init_flag, ADDR_INIT_FLAG, 1);
    
    // 2. 判断是否是首次上电 (或者芯片被清空过)
    if (init_flag == MAGIC_NUM) {
        // --- 已经初始化过，直接读取历史参数覆盖 sys 字典 ---
        // 利用指针强转，将 EEPROM 里连续的 4 个字节直接塞给 float 和 uint32_t 变量
        eeprom_read((uint8_t *)&sys.v_threshold, ADDR_V_THR, 4);
        HAL_Delay(5);
        eeprom_read((uint8_t *)&sys.f_threshold, ADDR_F_THR, 4);
    } else {
        // --- 第一次上电，写入默认的安全参数 ---
        sys.v_threshold = 2.5f;
        sys.f_threshold = 1000;
        
        // 保存默认值到 EEPROM
        eeprom_write((uint8_t *)&sys.v_threshold, ADDR_V_THR, 4);
        HAL_Delay(5); // 初始化阶段用阻塞延时没关系，此时系统还没起跑
        eeprom_write((uint8_t *)&sys.f_threshold, ADDR_F_THR, 4);
        HAL_Delay(5);
        
        // 写入魔法数字，以后开机就会走上面的读取逻辑了
        init_flag = MAGIC_NUM;
        eeprom_write(&init_flag, ADDR_INIT_FLAG, 1);
    }
}

/**
 * @brief  非阻塞 EEPROM 保存任务
 * @note   由调度器定时调用 (建议 10ms)，平时不占用资源，检测到标志位才启动状态机
 */
void EEPROM_Proc(void) {
    static uint8_t  save_state = 0;
    static uint32_t save_timer = 0;
    
    // 影子缓存区，用于冻结保存瞬间的数据
    static float    shadow_v_threshold = 0.0f;
    static uint32_t shadow_f_threshold = 0;
    
    // 收到保存请求，且当前空闲，则启动状态机
    if (sys.eeprom_save_flag == true && save_state == 0) {
        
        // 拍摄数据快照，隔离外部字典的动态变化
        shadow_v_threshold = sys.v_threshold;
        shadow_f_threshold = sys.f_threshold;
        
        save_state = 1;               // 步入状态 1
        sys.eeprom_save_flag = false; // 消费掉事件
    }
    
    // 异步状态机：分步保存，中间留足时间供芯片烧写
    switch (save_state) {
        case 1:
            // 注意：这里传入的是影分身的地址，而不是 sys 字典的地址
            eeprom_write((uint8_t *)&shadow_v_threshold, ADDR_V_THR, 4);
            save_timer = HAL_GetTick(); 
            save_state = 2;
            break;
            
        case 2:
            if (HAL_GetTick() - save_timer >= 10) {
                // 同样从影分身写入第二部分数据
                eeprom_write((uint8_t *)&shadow_f_threshold, ADDR_F_THR, 4);
                save_timer = HAL_GetTick();
                save_state = 3;
            }
            break;
            
        case 3:
            if (HAL_GetTick() - save_timer >= 10) {
                save_state = 0; // 归零，释放状态机
            }
            break;
            
        default:
            save_state = 0;
            break;
    }
}
