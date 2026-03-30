// ringbuffer.c
#include "ringbuffer.h"

void RB_Init(RingBuffer_t *rb) {
    rb->head = 0;
    rb->tail = 0;
}

bool RB_Write(RingBuffer_t *rb, uint8_t data) {

    uint16_t next_head = (rb->head + 1) % RB_MAX_LEN;

    if (next_head == rb->tail) {
        return false; 
    }

    rb->buffer[rb->head] = data;
    rb->head = next_head;
    return true;
}

bool RB_Read(RingBuffer_t *rb, uint8_t *data) {

    if (rb->head == rb->tail) {
        return false;
    }

    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % RB_MAX_LEN;
    return true;
}

uint16_t RB_Get_Length(RingBuffer_t *rb) {

    return (rb->head >= rb->tail) ? (rb->head - rb->tail) : (RB_MAX_LEN - rb->tail + rb->head);
}
