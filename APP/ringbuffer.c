#include "ringbuffer.h"

void RB_Init(RingBuffer_t *rb) {
    rb->head = 0;
    rb->tail = 0;
}

// 写入数据 (生产者调用，如串口中断)
bool RB_Write(RingBuffer_t *rb, uint8_t data) {
    uint16_t next_head = (rb->head + 1) % RB_MAX_LEN;
    if (next_head == rb->tail) {
        return false; // 队列已满，丢弃数据
    }
    rb->buffer[rb->head] = data;
    rb->head = next_head;
    return true;
}

// 读取数据 (消费者调用，如主循环任务)
bool RB_Read(RingBuffer_t *rb, uint8_t *data) {
    if (rb->head == rb->tail) {
        return false; // 队列为空
    }
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % RB_MAX_LEN;
    return true;
}

// 获取当前队列中的数据长度
uint16_t RB_Get_Length(RingBuffer_t *rb) {
    return (rb->head >= rb->tail) ? (rb->head - rb->tail) : (RB_MAX_LEN - rb->tail + rb->head);
}
