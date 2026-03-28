/* uart_app.c */
#include "uart_app.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>

extern DMA_HandleTypeDef hdma_usart1_rx; //把底层自动生成的 DMA 句柄引过来

// ==========================================
// 内存双缓冲架构：防数据覆盖
// ==========================================
// 1. DMA 专属后台搬运区 (业务代码永远不要碰它)
static uint8_t dma_rx_buf[UART_RX_MAX_LEN]; 

// 2. 供业务逻辑慢慢解析的快照区
uint8_t  uart_rx_buf[UART_RX_MAX_LEN]; 
uint16_t uart_rx_len = 0;
bool     uart_rx_flag = false; 

// ==========================================
// 初始化与使能
// ==========================================
void UART_Init(void) {
    // 启动空闲中断 + DMA 接收 (最多接收 UART_RX_MAX_LEN 个字节)
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, dma_rx_buf, UART_RX_MAX_LEN);
    
    // 【考场避坑神技】：强制关闭 DMA 半传输中断 (Half Transfer)
    // HAL库默认会开启它，导致如果发送64个字节，在第32个字节时就会打断程序，极易出错！
    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT); 
}

// ==========================================
// 中断回调 (由 HAL 库底层在总线空闲时自动调用)
// ==========================================
/**
 * @brief  ReceiveToIdle 专属回调函数 (只有当一帧完整数据收完后，才会进入这里)
 * @param  Size: DMA 实际搬运了多少个字节
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart->Instance == USART1) {
        
        // 1. 迅速将 DMA 后台数据拷贝到业务缓冲区，防止被下一帧覆盖
        memcpy(uart_rx_buf, dma_rx_buf, Size);
        
        // 2. 贴心地加上字符串结束符，方便后面直接用 printf / sscanf 解析文本指令
        uart_rx_buf[Size] = '\0'; 
        uart_rx_len = Size;
        
        // 3. 标记数据就绪，通知主循环去处理
        uart_rx_flag = true; 
        
        // 4. 重启一轮新的 IDLE + DMA 接收监听
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, dma_rx_buf, UART_RX_MAX_LEN);
        __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT); // 再次关闭半传输中断
    }
}

// ==========================================
// 业务逻辑处理 (放在主调度器 scheduler.c 中执行)
// ==========================================
void UART_Proc(void) {
    if (uart_rx_flag == true) {
        
        // --- 在这里写你的串口解析业务逻辑 ---
        
        // 例子：假设考题要求接收 "SET_V:2.55\r\n" 指令，修改电压阈值
        float temp_v = 0.0f;
        
        // 使用 sscanf 格式化提取字符串里的浮点数
        if (sscanf((char *)uart_rx_buf, "SET_V:%f", &temp_v) == 1) {
            // 参数合法性校验防呆
            if (temp_v >= 0.0f && temp_v <= 3.3f) {
                sys.v_threshold = temp_v;
                UART_SendString("OK\r\n");
            } else {
                UART_SendString("ERR: Out of Range\r\n");
            }
        } else {
            UART_SendString("ERR: Format Error\r\n");
        }
        
        // --- 业务处理结束 ---
        
        // 消费完毕，清空标志位
        uart_rx_flag = false;
    }
}

// ==========================================
// 辅助发送函数
// ==========================================
void UART_SendString(char *str) {
    HAL_UART_Transmit(&huart1, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}
