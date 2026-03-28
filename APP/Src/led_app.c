/**
 * @file    led_app.c
 * @brief   LED 硬件驱动与业务刷新逻辑
 */
#include "led_app.h"
#include "global_system.h" 

/**
 * @brief  [底层驱动] 将字典中的状态打包，通过 74HC573 锁存器推送到硬件
 * @note   PC8 ~ PC15 控制 8 个 LED (0亮1灭，需取反)。
 * PD2 为锁存器的 LE (Latch Enable) 门控引脚。
 */
void LED_Disp(void) {
    uint32_t odr_backup = GPIOC->ODR; // 备份当前 GPIOC 状态，防止影响其他复用引脚
    uint8_t led_status = 0;           // 打包后的 8 位硬件状态
    
    // 1. 将 sys 字典中的独立位，压缩为一个字节
    for(int i = 0; i < 8; i++) {
        if(sys.led_ctrl[i] != 0) {
            led_status |= (1 << i);   
        }
    }
    
    // 2. 硬件推挽输出：先清零 PC8~PC15，再写入取反后的状态
    GPIOC->ODR = (odr_backup & 0x00FF) | ((~led_status & 0xFF) << 8);
    
    // 3. 拨动 PD2 锁存器：开门(数据流入) -> 关门(数据锁定)
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);   
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET); 
    
    // 4. 恢复 GPIOC 原本的输出状态
    GPIOC->ODR = odr_backup;
}

/**
 * @brief  LED 业务状态映射任务 (极简 MVC 架构)
 * @note   建议调度周期：20ms。
 * 每次进入先清空所有状态，再根据最新的全局变量重新计算。
 */
void LED_Proc(void) {
    // 1. 擦除上一帧的虚拟显存
    for(int i = 0; i < 8; i++) {
        sys.led_ctrl[i] = 0;
    }
    
    // ================= 业务判定逻辑区 =================
    
    // 【常亮类指示】：UI 页面指示
    if (sys.current_page == PAGE_DATA) {
        sys.led_ctrl[0] = 1; // 处于 DATA 页面，亮 LD1
    } else if (sys.current_page == PAGE_PARA) {
        sys.led_ctrl[1] = 1; // 处于 PARA 页面，亮 LD2
    }
    
    // 【系统心跳指示】：利用系统滴答，每秒亮 500ms，灭 500ms (1Hz)
    if (HAL_GetTick() % 1000 < 500) {
        sys.led_ctrl[2] = 1; 
    }
    
    // 【越限报警指示】：R37 电压超标
    if (sys.r37_voltage > sys.v_threshold) {
        sys.led_ctrl[3] = 1; 
    }
    
    // 【软定时器瞬发指示】：EEPROM烧录/串口接收等后台动作反馈
    // 杜绝了传统 flag=true 导致灯常亮死锁的 Bug
    if (sys.led8_timer > 0) {
        sys.led_ctrl[7] = 1; // 定时器未归零，保持点亮
        sys.led8_timer--;    // 每执行一次任务(20ms)，倒计时减 1
    }
    
    // ================= 判定结束，提交底层 =================
    LED_Disp();
}
