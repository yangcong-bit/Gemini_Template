// eeprom_app.h
#ifndef __EEPROM_APP_H
#define __EEPROM_APP_H

#include "main.h"
#include "global_system.h" 

void EEPROM_Init(void);  
void EEPROM_Proc(void);  

void EEPROM_PushLog(LogData_t new_log);

#endif
