/**
 * @file    data_app.h
 * @brief   数据中枢与业务联动模块 (Data Glue Layer)
 * @note    该模块负责调度底层采集接口，并处理跨模块的业务逻辑（如电压映射PWM、触发EEPROM保存）。
 */
#ifndef __DATA_APP_H
#define __DATA_APP_H

#include "main.h"

void Data_Proc(void);

#endif /* __DATA_APP_H */
