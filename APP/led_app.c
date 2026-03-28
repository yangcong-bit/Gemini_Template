/* led_app.c */
#include "led_app.h"

// 实体定义：LED状态数组
uint8_t led_ctrl[8] = {0};

/**
 * @brief  LED 底层驱动：将数组状态打包成硬件需要的 8 位数据
 */
void LED_Disp(void) {
    uint32_t odr_backup = GPIOC->ODR;
    uint8_t led_status = 0;
    
    // 将数组的值打包成 1 个字节 (位压缩)
    for(int i = 0; i < 8; i++) {
        if(led_ctrl[i] != 0) {
            led_status |= (1 << i);
        }
    }
    
    // 底层硬件逻辑不变
    GPIOC->ODR = (odr_backup & 0x00FF) | ((~led_status & 0xFF) << 8);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
    GPIOC->ODR = odr_backup;
}

/**
 * @brief  LED 业务逻辑处理任务 (20ms)
 */
void LED_Proc(void) {
    // 每次进入先清空数组状态，相当于清除上一帧的 LED 显示
    for(int i = 0; i < 8; i++) {
        led_ctrl[i] = 0;
    }
    
    // 1. 页面指示功能 (LD1 / LD2)
    if (sys.current_page == PAGE_DATA) led_ctrl[0] = 1; // 数组索引0对应LD1
    if (sys.current_page == PAGE_PARA) led_ctrl[1] = 1; // 数组索引1对应LD2
    
    // 2. 系统心跳指示灯 (利用系统滴答，LD3 每秒闪烁一次证明程序没卡死)
    if (HAL_GetTick() % 1000 < 500) {
        led_ctrl[2] = 1; 
    }
    
    // 3. 越限报警逻辑 (如果 R37 超过了设定的安全阈值，LD4 长亮)
    if (sys.r37_voltage > sys.v_threshold) {
        led_ctrl[3] = 1; 
    }
    
    // 4. 后台事件指示 (正在往 EEPROM 烧录参数，或收到了串口指令未处理完时，点亮 LD8)
    if (sys.eeprom_save_flag || sys.uart_rx_ready) {
        led_ctrl[7] = 1; 
    }
    
    // 刷新到底层硬件
    LED_Disp();
    
    // 【消费控制信箱】
    if (sys.key_event_ctrl != 0) {
        sys.key_event_ctrl = 0;
    }
    if (sys.uart_rx_ready == true) {
        sys.uart_rx_ready = false;
    }
}
