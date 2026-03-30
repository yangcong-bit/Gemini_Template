// uart_app.h
#ifndef __UART_APP_H
#define __UART_APP_H

#include "main.h"

#define UART_RX_MAX_LEN 128 

extern uint8_t  uart_rx_buf[UART_RX_MAX_LEN]; 
extern uint16_t uart_rx_len;                  

void UART_Init(void);
void UART_Proc(void);
void UART_SendString(char *str);

#endif
