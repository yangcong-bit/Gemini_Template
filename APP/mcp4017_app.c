/* mcp4017_app.c */
#include "mcp4017_app.h"
#include "i2c_hal.h" // 引入我们刚刚补充了底层写函数的 I2C 库

// 用于记录上一次写入芯片的阻值档位，初始化为 0xFF 是为了强迫开机时必定刷新一次
static uint8_t last_res_step = 0xFF; 

/**
 * @brief  MCP4017 开机初始化
 * @note   在 main.c 的 while(1) 之前调用
 */
void MCP4017_Init(void) {
    // 假设开机默认档位为 64 (大概是 50kΩ 的位置)
    sys.res_step = 64;
    
    // 开机强制写入一次默认值
    mcp4017_write(sys.res_step);
    last_res_step = sys.res_step;
}

/**
 * @brief  MCP4017 动态刷新任务 (消费者)
 * @note   建议调度周期：50ms。实时监控字典，数值变化才写 I2C。
 */
void MCP4017_Proc(void) {
    // 1. 安全限幅保护：MCP4017 的有效档位是 0 到 127 (即 0x00 ~ 0x7F)
    if (sys.res_step > 127) {
        sys.res_step = 127; 
    }
    
    // 2. 状态对比：只有当全局字典里的要求值，和上一次写进去的值不一样时，才启动 I2C
    if (sys.res_step != last_res_step) {
        
        mcp4017_write(sys.res_step); // 调用底层发送
        
        last_res_step = sys.res_step; // 同步历史记录
    }
}
