/**
 * @file    uart_app.h
 * @brief   串口 IDLE+DMA 不定长接收与解析模块
 * @note    纯异步非阻塞架构。底层 DMA 负责后台搬运，主循环负责业务解析。
 */
#ifndef __UART_APP_H
#define __UART_APP_H

#include "main.h"

/* 串口单帧数据包最大接收长度 */
#define UART_RX_MAX_LEN 128 

/* 暴露给外部的业务快照缓冲区 (供外部 printf 或 sscanf 解析用) */
extern uint8_t  uart_rx_buf[UART_RX_MAX_LEN]; 
extern uint16_t uart_rx_len;                  

void UART_Init(void);
void UART_Proc(void);
void UART_SendString(char *str);

#endif /* __UART_APP_H */
