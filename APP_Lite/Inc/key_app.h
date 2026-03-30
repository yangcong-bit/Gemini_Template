// key_app.h
#ifndef __KEY_APP_H
#define __KEY_APP_H

#include "main.h"
#include "global_system.h" 

#define KEY1  1
#define KEY2  2
#define KEY3  3
#define KEY4  4

void Key_Proc(void);

bool Key_Get_Event(uint8_t *out_event);

#endif
