/* uart_app.h */
#ifndef __UART_APP_H
#define __UART_APP_H

#include "main.h"
#include "global_system.h" // 如果你有使用系统全局字典
#include <stdbool.h>

#define UART_RX_MAX_LEN 128 // 定义单帧数据包最大长度

// 暴露给外部的业务缓冲区和标志位
extern uint8_t  uart_rx_buf[UART_RX_MAX_LEN];
extern uint16_t uart_rx_len;
extern bool     uart_rx_flag;

void UART_Init(void);
void UART_Proc(void);
void UART_SendString(char *str);

#endif
