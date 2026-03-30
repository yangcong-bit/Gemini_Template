// key_app.c
#include "key_app.h"

const uint8_t KEY_DOUBLE_CLICK_EN[5] = {
    0, 
    0, 
    0, 
    0, 
    0  
};

static uint8_t Key_Read(void) {
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET) return KEY1;
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET) return KEY2;
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_RESET) return KEY3;
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) return KEY4;
    return 0; 
}

static void Dispatch_KeyEvent(uint8_t event) {
    uint16_t next_head = (sys.key_queue.head + 1) % KEY_QUEUE_LEN;

    if (next_head != sys.key_queue.tail) {
        sys.key_queue.buffer[sys.key_queue.head] = event;
        sys.key_queue.head = next_head;
    }
}

bool Key_Get_Event(uint8_t *out_event) {
    if (sys.key_queue.head == sys.key_queue.tail) {
        return false; 
    }
    *out_event = sys.key_queue.buffer[sys.key_queue.tail];
    sys.key_queue.tail = (sys.key_queue.tail + 1) % KEY_QUEUE_LEN;
    return true;
}

void Key_Proc(void) {
    static uint8_t  key_state = 0;    
    static uint8_t  key_prev = 0;     
    static uint16_t key_time = 0;     

    uint8_t key_press = Key_Read();   

    switch (key_state) {
        case 0: 
            if (key_press != 0) {
                key_prev = key_press; 
                key_state = 1;        
            }
            break;

        case 1: 
            if (key_press == key_prev) {
                key_state = 2;
                key_time = 0;         
            } else {
                key_state = 0; 
            }
            break;

        case 2: 
            if (key_press == key_prev) {
                key_time++;
                if (key_time >= 80) { 
                    Dispatch_KeyEvent(key_prev + 10); 
                    key_state = 3;    
                }
            } else if (key_press == 0) {
                key_time = 0; 

                if (KEY_DOUBLE_CLICK_EN[key_prev] == 1) {
                    key_state = 4; 
                } else {
                    Dispatch_KeyEvent(key_prev); 
                    key_state = 0;
                }
            } else {

                Dispatch_KeyEvent(key_prev); 
                key_prev = key_press;        
                key_state = 1;               
            }
            break;

        case 3: 
            if (key_press == 0) {
                key_state = 0; 
            } else if (key_press != key_prev) {
                key_prev = key_press;
                key_state = 1;
            }
            break;

        case 4: 
            if (key_press == 0) {
                key_time++;
                if (key_time >= 25) { 
                    Dispatch_KeyEvent(key_prev); 
                    key_state = 0;
                }
            } else if (key_press == key_prev) {
                key_state = 5; 
            } else {
                Dispatch_KeyEvent(key_prev); 
                key_prev = key_press;        
                key_state = 1;               
            }
            break;

        case 5: 
            if (key_press == key_prev) {
                Dispatch_KeyEvent(key_prev + 20); 
                key_state = 6;
            } else {
                key_state = 4; 
            }
            break;

        case 6: 
            if (key_press == 0) {
                key_state = 0;
            } else if (key_press != key_prev) {
                key_prev = key_press;
                key_state = 1;
            }
            break;
    }
}
