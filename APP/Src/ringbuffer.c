/**
 * @file    ringbuffer.c
 * @brief   通用环形缓冲区的实现
 */
#include "ringbuffer.h"

/**
 * @brief  初始化环形缓冲区，复位头尾指针
 * @param  rb 目标缓冲区句柄
 */
void RB_Init(RingBuffer_t *rb) {
    rb->head = 0;
    rb->tail = 0;
}

/**
 * @brief  [生产者] 向队列尾部写入一个字节数据
 * @param  rb   目标缓冲区句柄
 * @param  data 待写入的数据
 * @return true 写入成功; false 队列已满，数据丢弃
 */
bool RB_Write(RingBuffer_t *rb, uint8_t data) {
    // 预测下一个头指针位置 (使用取模实现环回)
    uint16_t next_head = (rb->head + 1) % RB_MAX_LEN;
    
    // 如果追上了尾指针，说明队列已满
    if (next_head == rb->tail) {
        return false; // 严防覆盖旧数据
    }
    
    rb->buffer[rb->head] = data;
    rb->head = next_head;
    return true;
}

/**
 * @brief  [消费者] 从队列头部读取一个字节数据
 * @param  rb   目标缓冲区句柄
 * @param  data 用于带回读取到的数据指针
 * @return true 读取成功; false 队列为空，无数据可读
 */
bool RB_Read(RingBuffer_t *rb, uint8_t *data) {
    // 头尾指针重合，说明队列内无数据
    if (rb->head == rb->tail) {
        return false;
    }
    
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % RB_MAX_LEN;
    return true;
}

/**
 * @brief  计算当前队列中积压的有效数据长度
 * @param  rb 目标缓冲区句柄
 * @return 积压数据的字节数
 */
uint16_t RB_Get_Length(RingBuffer_t *rb) {
    // 分两种情况计算：指针未发生折叠，和指针发生了环回折叠
    return (rb->head >= rb->tail) ? (rb->head - rb->tail) : (RB_MAX_LEN - rb->tail + rb->head);
}
