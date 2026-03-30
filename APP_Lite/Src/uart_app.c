// uart_app.c
#include "uart_app.h"
#include "usart.h"         
#include "global_system.h" 
#include <string.h>
#include <stdio.h>

extern DMA_HandleTypeDef hdma_usart1_rx; 

static uint8_t dma_rx_buf[UART_RX_MAX_LEN]; 

uint8_t  uart_rx_buf[UART_RX_MAX_LEN]; 
uint16_t uart_rx_len = 0;

void UART_Init(void) {

    HAL_UARTEx_ReceiveToIdle_DMA(&UART_APP_HANDLE, dma_rx_buf, UART_RX_MAX_LEN);

    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart->Instance == UART_APP_INST) {

        memcpy(uart_rx_buf, dma_rx_buf, Size);

        uart_rx_buf[Size] = '\0'; 
        uart_rx_len = Size;

        sys.uart_rx_ready = true; 

        HAL_UARTEx_ReceiveToIdle_DMA(&UART_APP_HANDLE, dma_rx_buf, UART_RX_MAX_LEN);
        __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT); 
    }
}

void UART_SendString(char *str) {
    HAL_UART_Transmit(&UART_APP_HANDLE, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}
