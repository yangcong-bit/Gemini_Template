/* led_app.c */
#include "led_app.h"

/**
 * @brief  LED 底层驱动（带严格的现场保护）
 * @param  led_status: 8位状态，bit0~bit7 对应 LD1~LD8 (1亮，0灭)
 */
void LED_Disp(uint8_t led_status) {
    // 1. 保存现场：读取当前 GPIOC 的输出寄存器状态 (极其重要，保护LCD的数据线状态)
    uint32_t odr_backup = GPIOC->ODR;
    
    // 2. 准备 LED 数据：
    // 蓝桥杯板子 LED 为共阳极（或低电平驱动发光），因此对 led_status 取反 (~led_status)。
    // 逻辑运算：清除高8位(PC8-PC15，设为0)，保留低8位(PC0-PC7，原样保留)，
    // 然后将取反后的 LED 状态左移 8 位，嵌入到高 8 位中。
    GPIOC->ODR = (odr_backup & 0x00FF) | ((~led_status & 0xFF) << 8);
    
    // 3. 锁存器操作：PD2 (LE) 拉高再拉低，产生一个下降沿，
    // 将此时 PC8-PC15 的数据死死锁存到 74HC573 的输出端。
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
    
    // 4. 恢复现场：将之前保存的 GPIOC 状态原封不动写回。
    // 此时即使后续立刻进行 LCD 的刷新，LCD 也能拿到正确的 GPIOC 状态，彻底杜绝冲突。
    GPIOC->ODR = odr_backup;
}

/**
 * @brief  LED 业务逻辑处理任务
 * @note   该函数由调度器定时调用 (例如在 scheduler.c 中注册为每 20ms 执行一次)
 */
void LED_Proc(void) {
    uint8_t current_led = 0x00; 
    
    // 1. 页面指示功能 (LD1 / LD2)
    if (sys.current_page == PAGE_DATA) current_led |= 0x01; 
    if (sys.current_page == PAGE_PARA) current_led |= 0x02; 
    
    // 2. 系统心跳指示灯 (利用系统滴答，LD3 每秒闪烁一次证明程序没卡死)
    if (HAL_GetTick() % 1000 < 500) {
        current_led |= 0x04; 
    }
    
    // 3. 越限报警逻辑 (如果 R37 超过了设定的安全阈值，LD4 长亮)
    if (sys.r37_voltage > sys.v_threshold) {
        current_led |= 0x08; 
    }
    
    // 4. 后台事件指示 (正在往 EEPROM 烧录参数，或收到了串口指令未处理完时，点亮 LD8)
    if (sys.eeprom_save_flag || sys.uart_rx_ready) {
        current_led |= 0x80; 
    }
    
    // 刷新底层硬件
    LED_Disp(current_led);
    
    // 【消费控制信箱】：防止按键事件在 ctrl 信箱溢出
    if (sys.key_event_ctrl != 0) {
        // 如果题目要求按键按下伴随蜂鸣器响，可写在这里
        sys.key_event_ctrl = 0;
    }
		//提示完 LD8 后，立刻清空串口接收标志，防止 LD8 永远常亮
    if (sys.uart_rx_ready == true) {
        sys.uart_rx_ready = false;
		}
}
