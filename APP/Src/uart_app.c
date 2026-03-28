/**
 * @file    uart_app.c
 * @brief   串口空闲中断 + DMA 接收底层实现与业务路由
 */
#include "uart_app.h"
#include "usart.h"         // 引入底层串口句柄 huart1
#include "global_system.h" // 引入全局数据字典，操作就绪标志位和指示灯
#include <string.h>
#include <stdio.h>

extern DMA_HandleTypeDef hdma_usart1_rx; // 引入底层自动生成的 DMA 句柄

/* ==========================================
 * 内存双缓冲架构：防数据覆盖核心
 * ========================================== */
// 1. DMA 专属后台搬运区 (业务代码永远不要碰它，随时会被底层硬件静默刷新)
static uint8_t dma_rx_buf[UART_RX_MAX_LEN]; 

// 2. 供业务逻辑慢慢解析的快照区 (安全操作区)
uint8_t  uart_rx_buf[UART_RX_MAX_LEN]; 
uint16_t uart_rx_len = 0;

/**
 * @brief  串口 IDLE+DMA 接收初始化
 * @note   在 main 函数 while(1) 之前调用。
 */
void UART_Init(void) {
    // 启动空闲中断 + DMA 接收监听
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, dma_rx_buf, UART_RX_MAX_LEN);
    
    // 【考场避坑神技】：强制关闭 DMA 半传输中断 (Half Transfer)
    // HAL库默认开启它，会导致发长包时在中间截断并触发异常回调，必须关掉！
    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT); 
}

/**
 * @brief  ReceiveToIdle 专属中断回调函数 
 * @note   由 HAL 库在 RX 总线空闲 (即一帧数据发完) 时自动调用。
 * @param  Size: DMA 实际搬运了多少个字节
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart->Instance == USART1) {
        
        // 1. 极速将 DMA 后台数据抢救到业务缓冲区，防止被下一帧覆盖
        memcpy(uart_rx_buf, dma_rx_buf, Size);
        
        // 2. 贴心地加上字符串结束符，防溢出，方便后续直接用 sscanf 解析文本
        uart_rx_buf[Size] = '\0'; 
        uart_rx_len = Size;
        
        // 3. 标记数据就绪，通知主循环 scheduler 去处理
        sys.uart_rx_ready = true; 
        
        // 4. 触发后台事件指示灯 (LED8 亮起 10 个调度周期，约 200ms)
        sys.led8_timer = 10;
        
        // 5. 重启一轮新的 IDLE + DMA 接收监听
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, dma_rx_buf, UART_RX_MAX_LEN);
        __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT); // 再次关闭半传输中断
    }
}

/**
 * @brief  串口协议解析任务 (消费者)
 * @note   建议调度周期：10ms。
 */
void UART_Proc(void) {
    // 检测系统统一的数据就绪标志位
    if (sys.uart_rx_ready == true) {
        
        float temp_v = 0.0f;
        
        // --- 业务解析逻辑开始 ---
        // 范例：接收 "SET_V:2.55\r\n" 指令，修改电压阈值
        if (sscanf((char *)uart_rx_buf, "SET_V:%f", &temp_v) == 1) {
            
            // 参数合法性校验防呆 (0.0V ~ 3.3V)
            if (temp_v >= 0.0f && temp_v <= 3.3f) {
                sys.v_threshold = temp_v;
                UART_SendString("OK\r\n");
            } else {
                UART_SendString("ERR: Out of Range\r\n");
            }
        } else {
            UART_SendString("ERR: Format Error\r\n");
        }
        // --- 业务解析逻辑结束 ---
        
        // 【核心闭环】：消费完毕，必须清空标志位！否则任务会死锁重复执行
        sys.uart_rx_ready = false;
    }
}

/**
 * @brief  阻塞式串口字符串发送函数
 * @param  str: 需发送的以 \0 结尾的字符串指针
 */
void UART_SendString(char *str) {
    HAL_UART_Transmit(&huart1, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}
