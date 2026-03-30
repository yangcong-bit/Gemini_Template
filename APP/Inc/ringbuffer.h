/**
 * @file    ringbuffer.h
 * @brief   通用型单字节环形缓冲区 (Byte-Oriented FIFO)
 * @note    适用于串口纯裸字节接收缓冲等场景。
 * (注：本系统的高级按键和日志存储使用的是 global_system.h 中定制的结构体队列)
 */
#ifndef __RINGBUFFER_H
#define __RINGBUFFER_H

#include <stdint.h>
#include <stdbool.h>

#define RB_MAX_LEN  256  ///< 缓冲区深度，强烈建议设置为 2 的幂次方以利用位掩码优化

/**
 * @brief 环形队列结构体定义
 */
typedef struct {
    uint16_t head;                 ///< 队头指针（生产者写入位置）
    uint16_t tail;                 ///< 队尾指针（消费者读取位置）
    uint8_t  buffer[RB_MAX_LEN];   ///< 物理数据存储区
} RingBuffer_t;

void RB_Init(RingBuffer_t *rb);
bool RB_Write(RingBuffer_t *rb, uint8_t data);
bool RB_Read(RingBuffer_t *rb, uint8_t *data);
uint16_t RB_Get_Length(RingBuffer_t *rb);

#endif /* __RINGBUFFER_H */
