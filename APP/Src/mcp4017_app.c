/**
 * @file    mcp4017_app.c
 * @brief   MCP4017 状态机与防冗余刷新逻辑实现
 */
#include "mcp4017_app.h"
#include "i2c_hal.h"       // 引入底层的 I2C 发送接口
#include "global_system.h" // 引入全局数据字典

/* 私有状态变量：记录上一次写入芯片的阻值档位 */
// 初始化为 0xFF (超出合法范围 0~127)，强迫开机时必定触发一次真实刷新
static uint8_t last_res_step = 0xFF; 

/**
 * @brief  MCP4017 开机初始化与默认值装载
 * @note   在 main.c 的 while(1) 之前调用
 */
void MCP4017_Init(void) {
    // 强制写入开机默认档位 (例如 64 大约对应 50kΩ)
    mcp4017_write(sys.res_step);
    last_res_step = sys.res_step;
}

/**
 * @brief  MCP4017 动态阻值刷新任务 (消费者)
 * @note   建议调度周期：50ms。
 * 核心逻辑：只在字典数值发生变化时才调用慢速的 I2C 总线，节省 CPU 算力。
 */
void MCP4017_Proc(void) {
    // 1. 安全限幅保护：MCP4017 的有效档位严格限制在 0 ~ 127 (即 0x00 ~ 0x7F)
    if (sys.res_step > 127) {
        sys.res_step = 127; 
    }
    
    // 2. 状态防抖对比：全局字典的目标值 != 历史记录值时，才启动 I2C 通信
    if (sys.res_step != last_res_step) {
        
        mcp4017_write(sys.res_step); // 调用底层发送
        last_res_step = sys.res_step; // 同步历史记录
    }
}