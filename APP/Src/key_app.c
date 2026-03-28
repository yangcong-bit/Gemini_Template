/**
 * @file    key_app.c
 * @brief   按键底层状态机实现
 */
#include "key_app.h"

/* ==========================================
 * 按键功能配置区：为每个按键独立配置特性
 * 数组索引对应键值 (1~4)，0号索引作废不用。
 * [填 1]：开启双击检测 (牺牲约 250ms 的短按判定延时，用于等待第二下)
 * [填 0]：关闭双击检测 (纯短按/长按，松手瞬间极速触发，极致跟手)
 * ========================================== */
const uint8_t KEY_DOUBLE_CLICK_EN[5] = {
    0, // [0] 占位符作废
    1, // [1] KEY1 开启双击 (用于快速调整阻值档位)
    0, // [2] KEY2 关闭双击 (只做短按增加电压)
    0, // [3] KEY3 关闭双击 (只做短按增加PWM)
    0  // [4] KEY4 关闭双击 (只做短按保存EEPROM)
};

/**
 * @brief  [私有] 读取底层 GPIO 硬件状态
 * @return 当前按下的物理键值 (1-4)，无按键返回 0
 */
static uint8_t Key_Read(void) {
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET) return KEY1;
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET) return KEY2;
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_RESET) return KEY3;
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) return KEY4;
    return 0; 
}

/**
 * @brief  [生产者] 按键事件打包入队
 * @param  event 具体的按键事件码 (如: 1短按, 11长按, 21双击)
 */
static void Dispatch_KeyEvent(uint8_t event) {
    uint16_t next_head = (sys.key_queue.head + 1) % KEY_QUEUE_LEN;
    
    // 如果队列未满，则推入新数据并移动头指针
    if (next_head != sys.key_queue.tail) {
        sys.key_queue.buffer[sys.key_queue.head] = event;
        sys.key_queue.head = next_head;
    }
}

/**
 * @brief  [消费者] 从队列提取最新按键事件 (外部 API)
 * @param  out_event 存放取出的键值
 * @return true: 取到了按键; false: 队列为空
 */
bool Key_Get_Event(uint8_t *out_event) {
    if (sys.key_queue.head == sys.key_queue.tail) {
        return false; // 队列为空
    }
    *out_event = sys.key_queue.buffer[sys.key_queue.tail];
    sys.key_queue.tail = (sys.key_queue.tail + 1) % KEY_QUEUE_LEN;
    return true;
}

/**
 * @brief  标志位驱动的 7 态按键状态机 (带防暴力揉搓与滑键保护)
 * @note   建议调度周期：10ms
 */
void Key_Proc(void) {
    static uint8_t  key_state = 0;    // 状态机主节点
    static uint8_t  key_prev = 0;     // 记录上一次被确认按下的键值
    static uint16_t key_time = 0;     // 按下时长计时器 (以 10ms 为单位)
    
    uint8_t key_press = Key_Read();   

    switch (key_state) {
        case 0: // 【状态 0】空闲等待
            if (key_press != 0) {
                key_prev = key_press; 
                key_state = 1;        
            }
            break;

        case 1: // 【状态 1】按下初次消抖
            if (key_press == key_prev) {
                key_state = 2;
                key_time = 0;         
            } else {
                key_state = 0; // 抖动，退回
            }
            break;

        case 2: // 【状态 2】确认按下，甄别长按或释放
            if (key_press == key_prev) {
                key_time++;
                if (key_time >= 80) { // 持续按下 800ms，判定为长按
                    Dispatch_KeyEvent(key_prev + 10); 
                    key_state = 3;    
                }
            } else if (key_press == 0) {
                key_time = 0; // 松手了
                
                // 【核心分流】：检查该按键是否配置了双击
                if (KEY_DOUBLE_CLICK_EN[key_prev] == 1) {
                    key_state = 4; // 进入状态4等待双击判定      
                } else {
                    Dispatch_KeyEvent(key_prev); // 绕过延时，极速触发短按！
                    key_state = 0;
                }
            } else {
                // 手指滑到了另一个键：强制结算上一个键的短按，抓取新键
                Dispatch_KeyEvent(key_prev); 
                key_prev = key_press;        
                key_state = 1;               
            }
            break;

        case 3: // 【状态 3】长按触发后的等待释放 (防连发)
            if (key_press == 0) {
                key_state = 0; 
            } else if (key_press != key_prev) {
                key_prev = key_press;
                key_state = 1;
            }
            break;

        case 4: // 【状态 4】首次释放后，等待双击的第二下
            if (key_press == 0) {
                key_time++;
                if (key_time >= 25) { // 超过 250ms 没有按第二下，降级结算为短按
                    Dispatch_KeyEvent(key_prev); 
                    key_state = 0;
                }
            } else if (key_press == key_prev) {
                key_state = 5; // 在 250ms 内再次按下了同一个键
            } else {
                Dispatch_KeyEvent(key_prev); // 等待期间按了别的键：结算旧键，抓新键
                key_prev = key_press;        
                key_state = 1;               
            }
            break;

        case 5: // 【状态 5】双击的第二次按下消抖
            if (key_press == key_prev) {
                Dispatch_KeyEvent(key_prev + 20); // 正式发送双击事件 (+20)
                key_state = 6;
            } else {
                key_state = 4; // 抖动，退回等待
            }
            break;

        case 6: // 【状态 6】双击触发后的等待释放
            if (key_press == 0) {
                key_state = 0;
            } else if (key_press != key_prev) {
                key_prev = key_press;
                key_state = 1;
            }
            break;
    }
}
