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
    // 1. 初始化软件环形队列
    RB_Init(&uart_rb);
    
    // 2. 彻底清空解析缓存数组
    frame_len = 0;
    memset(frame_buf, 0, sizeof(frame_buf));

    // 3. 【防毛刺核心】：清除上电瞬间产生的硬件错误标志（溢出、帧错误等）
    __HAL_UART_CLEAR_FLAG(&huart1, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_PEF | UART_CLEAR_FEF);
    
    // 4. 【防毛刺核心】：强制清空硬件接收数据寄存器 (STM32G4系列特有)，把上电抖动产生的乱码直接倒进垃圾桶
    __HAL_UART_SEND_REQ(&huart1, UART_RXDATA_FLUSH_REQUEST);

    // 5. 开启第一次单字节接收中断
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
            // 3. 核心业务：进行字符串解析 (增强抗干扰能力)
            // ==========================================
            float new_v = 0.0f;
            char *ptr; // 用于定位指令在缓冲区中的实际位置
            
            // 使用 strstr 查找子串，容忍指令前面带有上电乱码
            if ((ptr = strstr(frame_buf, "SET_V:")) != NULL) {
                // 从找到指令的地方开始进行格式化提取
                if (sscanf(ptr, "SET_V:%f", &new_v) == 1) {
                    sys.v_threshold = new_v;          
                    sys.eeprom_save_flag = true;      
                    printf("OK: V_Thr Set to %.2f\r\n", new_v); 
                }
            }
            else if (strstr(frame_buf, "GET_DATA") != NULL) {
                // 只要包含了 GET_DATA 就认为是合法请求
                printf("VOLT:%.2f, FREQ1:%d, FREQ2:%d\r\n", 
                        sys.r37_voltage, sys.freq_ch1, sys.freq_ch2);
            }
           else {
                // 【防毛刺核心】：只有当首字符是正常的英文字母时，才认为是人类敲错的指令并报错。
                // 如果是不可见字符或纯符号（上电乱码的典型特征），直接当成空气忽略掉，不打印 ERROR。
                if (frame_len > 1 && ((frame_buf[0] >= 'A' && frame_buf[0] <= 'Z') || (frame_buf[0] >= 'a' && frame_buf[0] <= 'z'))) {
                    printf("ERROR: Unknown Command\r\n");
                }
            }
            
            sys.uart_rx_ready = true;
            frame_len = 0;
            memset(frame_buf, 0, sizeof(frame_buf));
        }
    }
}
