#include "uart_app.h"
#include "ringbuffer.h"
#include "usart.h" // 包含 huart1
#include <stdio.h>
#include <string.h>

RingBuffer_t uart_rb;        // 实例化环形缓冲区
uint8_t rx_temp_byte;        // 用于存放中断接收到的单字节数据

// 帧解析缓存区 (用于把 RingBuffer 里的散装字节拼成一个完整的字符串)
static char frame_buf[64];
static uint8_t frame_len = 0;

/* =======================================================
 * printf 重定向实现
 * 放在这里永远不会被 CubeMX 抹除，保证模板的绝对开箱即用
 * ======================================================= */
int fputc(int ch, FILE *f) {
    // 采用阻塞发送，保证 printf 打印数据的完整性
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/**
 * @brief  串口应用初始化
 * @note   在 main.c 的 while(1) 之前调用
 */
void UART_Init(void) {
    RB_Init(&uart_rb);
    // 开启第一次单字节接收中断
    HAL_UART_Receive_IT(&huart1, &rx_temp_byte, 1);
}

/**
 * @brief  串口接收中断回调函数 (这里是“生产者”)
 * @note   每收到 1 个字节，HAL 库会自动调用此函数。执行速度极快，不阻碍系统。
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        // 1. 将收到的这个字节扔进环形缓冲区
        RB_Write(&uart_rb, rx_temp_byte);
        
        // 2. 必须再次开启接收中断，否则只能收一次
        HAL_UART_Receive_IT(&huart1, &rx_temp_byte, 1);
    }
}

/**
 * @brief  串口协议解析任务 (这里是“消费者”)
 * @note   建议在调度器中注册为每 10ms 执行一次
 */
void UART_Proc(void) {
    uint8_t data;
    
    // 1. 只要环形队列里有数据，就一直读出来拼装
    while (RB_Read(&uart_rb, &data)) {
        
        frame_buf[frame_len++] = data;
        
        // 防溢出保护
        if (frame_len >= 63) {
            frame_len = 0; 
            memset(frame_buf, 0, sizeof(frame_buf));
        }
        
        // 2. 检测到帧尾（约定以 '\n' 结尾）
        if (data == '\n') {
            // 【关键修复】：兼容 \r\n 和纯 \n 的换行符
            // 如果最后一个字符是 \n，且倒数第二个是 \r，则把 frame_len 减 1 剔除它
            if (frame_len >= 2 && frame_buf[frame_len - 2] == '\r') {
                frame_buf[frame_len - 2] = '\0'; 
            } else {
                frame_buf[frame_len - 1] = '\0'; 
            }
            
            // ==========================================
            // 3. 核心业务：进行字符串解析 (sscanf 或 strncmp)
            // ==========================================
            float new_v = 0.0f;
            
            if (sscanf(frame_buf, "SET_V:%f", &new_v) == 1) {
                sys.v_threshold = new_v;          
                sys.eeprom_save_flag = true;      
                printf("OK: V_Thr Set to %.2f\r\n", new_v); 
            }
            else if (strncmp(frame_buf, "GET_DATA", 8) == 0) {
                printf("VOLT:%.2f, FREQ1:%d, FREQ2:%d\r\n", 
                        sys.r37_voltage, sys.freq_ch1, sys.freq_ch2);
            }
            else {
                printf("ERROR: Unknown Command\r\n");
            }
            
            sys.uart_rx_ready = true;
            frame_len = 0;
            memset(frame_buf, 0, sizeof(frame_buf));
        }
    }
}
