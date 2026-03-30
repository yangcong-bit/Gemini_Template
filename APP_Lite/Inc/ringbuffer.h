// ringbuffer.h
#ifndef __RINGBUFFER_H
#define __RINGBUFFER_H

#include <stdint.h>
#include <stdbool.h>

#define RB_MAX_LEN  256  

typedef struct {
    uint16_t head;                 
    uint16_t tail;                 
    uint8_t  buffer[RB_MAX_LEN];   
} RingBuffer_t;

void RB_Init(RingBuffer_t *rb);
bool RB_Write(RingBuffer_t *rb, uint8_t data);
bool RB_Read(RingBuffer_t *rb, uint8_t *data);
uint16_t RB_Get_Length(RingBuffer_t *rb);

#endif
