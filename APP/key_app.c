/* key_app.c */
#include "key_app.h"

// ==========================================
// 按键功能配置区：单独为每个按键配置标志位
// 数组索引对应键值 (1~4)，0号索引作废不用。
// 1 表示开启双击检测 (有判定延时)
// 0 表示关闭双击检测 (纯短按/长按，松手瞬间触发，极致跟手)
// ==========================================
const uint8_t KEY_DOUBLE_CLICK_EN[5] = {
    0, // [0] 占位符
    1, // [1] KEY1 开启双击
    0, // [2] KEY2 关闭双击
    0, // [3] KEY3 关闭双击
    0  // [4] KEY4 关闭双击
};

static uint8_t Key_Read(void) {
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET) return KEY1;
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET) return KEY2;
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_RESET) return KEY3;
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) return KEY4;
    return 0; 
}

/**
 * @brief  【生产者】按键事件入队
 */
static void Dispatch_KeyEvent(uint8_t event) {
    uint16_t next_head = (sys.key_queue.head + 1) % KEY_QUEUE_LEN;
    
    // 如果队列未满，则写入数据
    if (next_head != sys.key_queue.tail) {
        sys.key_queue.buffer[sys.key_queue.head] = event;
        sys.key_queue.head = next_head;
    }
}

/**
 * @brief  【消费者】按键事件出队 (供 UI 等外部模块提取按键)
 * @return true表示取到了按键，false表示队列为空
 */
bool Key_Get_Event(uint8_t *out_event) {
    // 队列为空
    if (sys.key_queue.head == sys.key_queue.tail) {
        return false;
    }
    
    // 取出数据并移动尾指针
    *out_event = sys.key_queue.buffer[sys.key_queue.tail];
    sys.key_queue.tail = (sys.key_queue.tail + 1) % KEY_QUEUE_LEN;
    return true;
}

/**
 * @brief  标志位驱动的 7 态按键状态机 (带防暴力揉搓测试)
 * @note   基于 10ms 调度周期
 */
void Key_Proc(void) {
    static uint8_t  key_state = 0;    
    static uint8_t  key_prev = 0;     
    static uint16_t key_time = 0;     
    
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

        case 2: // 【状态 2】确认按下，等待长按或释放
            if (key_press == key_prev) {
                key_time++;
                // 持续按下 800ms，判定为长按
                if (key_time >= 80) { 
                    Dispatch_KeyEvent(key_prev + 10); // 发送长按事件
                    key_state = 3;    
                }
            } else if (key_press == 0) {
                // 松手了
                key_time = 0; 
                
                // 【核心修改：通过标志位动态分流状态机】
                if (KEY_DOUBLE_CLICK_EN[key_prev] == 1) {
                    // 该按键配置了双击功能，进入状态4等待判定
                    key_state = 4;        
                } else {
                    // 该按键未配置双击，绕过250ms延时，直接触发短按！
                    Dispatch_KeyEvent(key_prev);
                    key_state = 0;
                }
            } else {
                //手指没松开直接滑到了另一个键
                Dispatch_KeyEvent(key_prev); // 强制立即结算上一个键的短按
                key_prev = key_press;        // 抓取新按下的键
                key_state = 1;               // 立即转入新键的消抖状态
            }
            break;

        case 3: // 【状态 3】长按触发后的等待释放 (防连发)
            if (key_press == 0) {
                key_state = 0; 
            } else if (key_press != key_prev) {
                // 长按时手指滑到了别的键
                key_prev = key_press;
                key_state = 1;
            }
            break;

        case 4: // 【状态 4】释放后，等待双击的第二下
            if (key_press == 0) {
                key_time++;
                // 超过 250ms 没有按第二下，确认为短按
                if (key_time >= 25) { 
                    Dispatch_KeyEvent(key_prev); 
                    key_state = 0;
                }
            } else if (key_press == key_prev) {
                // 在 250ms 内再次按下了同一个键
                key_state = 5; 
            } else {
                // 在等双击判定的时候，快速按了别的键
                Dispatch_KeyEvent(key_prev); // 强制结算之前的短按
                key_prev = key_press;        // 抓取新按键
                key_state = 1;               // 立即处理新键
            }
            break;

        case 5: // 【状态 5】双击的第二次按下消抖
            if (key_press == key_prev) {
                Dispatch_KeyEvent(key_prev + 20); // 发送双击事件
                key_state = 6;
            } else {
                key_state = 4; // 抖动，退回等待
            }
            break;

        case 6: // 【状态 6】双击触发后的等待释放
            if (key_press == 0) {
                key_state = 0;
            } else if (key_press != key_prev) {
                // 双击第二下按住不放并滑键
                key_prev = key_press;
                key_state = 1;
            }
            break;
    }
}
