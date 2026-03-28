/**
 * @file    i2c_hal.c
 * @brief   软件 I2C 底层模拟协议与芯片驱动扩展
 * @note    内含蓝桥杯官方驱动，以及追加的 AT24C02 连续读写与 MCP4017 控制函数。
 */
#include "i2c_hal.h"

#define DELAY_TIME  40
//
void SDA_Input_Mode()
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Pin = GPIO_PIN_7;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
}

//
void SDA_Output_Mode()
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Pin = GPIO_PIN_7;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
}

//
void SDA_Output( uint16_t val )
{
    if ( val )
    {
        GPIOB->BSRR |= GPIO_PIN_7;
    }
    else
    {
        GPIOB->BRR |= GPIO_PIN_7;
    }
}

//
void SCL_Output( uint16_t val )
{
    if ( val )
    {
        GPIOB->BSRR |= GPIO_PIN_6;
    }
    else
    {
        GPIOB->BRR |= GPIO_PIN_6;
    }
}

//
uint8_t SDA_Input(void)
{
	if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) == GPIO_PIN_SET){
		return 1;
	}else{
		return 0;
	}
}

//
static void delay1(volatile unsigned int n)
{
    volatile uint32_t i;
    for ( i = 0; i < n; ++i);
}

//
void I2CStart(void)
{
    SDA_Output(1);
    delay1(DELAY_TIME);
    SCL_Output(1);
    delay1(DELAY_TIME);
    SDA_Output(0);
    delay1(DELAY_TIME);
    SCL_Output(0);
    delay1(DELAY_TIME);
}

//
void I2CStop(void)
{
    SCL_Output(0);
    delay1(DELAY_TIME);
    SDA_Output(0);
    delay1(DELAY_TIME);
    SCL_Output(1);
    delay1(DELAY_TIME);
    SDA_Output(1);
    delay1(DELAY_TIME);

}

//
unsigned char I2CWaitAck(void)
{
    unsigned short cErrTime = 5;
    SDA_Input_Mode();
    delay1(DELAY_TIME);
    SCL_Output(1);
    delay1(DELAY_TIME);
    while(SDA_Input())
    {
        cErrTime--;
        delay1(DELAY_TIME);
        if (0 == cErrTime)
        {
            SDA_Output_Mode();
            I2CStop();
            return ERROR;
        }
    }
    SCL_Output(0);
    SDA_Output_Mode();
    delay1(DELAY_TIME);
    return SUCCESS;
}

//
void I2CSendAck(void)
{
    SDA_Output(0);
    delay1(DELAY_TIME);
    delay1(DELAY_TIME);
    SCL_Output(1);
    delay1(DELAY_TIME);
    SCL_Output(0);
    delay1(DELAY_TIME);

}

//
void I2CSendNotAck(void)
{
    SDA_Output(1);
    delay1(DELAY_TIME);
    delay1(DELAY_TIME);
    SCL_Output(1);
    delay1(DELAY_TIME);
    SCL_Output(0);
    delay1(DELAY_TIME);

}

//
void I2CSendByte(unsigned char cSendByte)
{
    unsigned char  i = 8;
    while (i--)
    {
        SCL_Output(0);
        delay1(DELAY_TIME);
        SDA_Output(cSendByte & 0x80);
        delay1(DELAY_TIME);
        cSendByte += cSendByte;
        delay1(DELAY_TIME);
        SCL_Output(1);
        delay1(DELAY_TIME);
    }
    SCL_Output(0);
    delay1(DELAY_TIME);
}

//
unsigned char I2CReceiveByte(void)
{
    unsigned char i = 8;
    unsigned char cR_Byte = 0;
    SDA_Input_Mode();
    while (i--)
    {
        cR_Byte += cR_Byte;
        SCL_Output(0);
        delay1(DELAY_TIME);
        delay1(DELAY_TIME);
        SCL_Output(1);
        delay1(DELAY_TIME);
        cR_Byte |=  SDA_Input();
    }
    SCL_Output(0);
    delay1(DELAY_TIME);
    SDA_Output_Mode();
    return cR_Byte;
}

//
void I2CInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Pin = GPIO_PIN_7 | GPIO_PIN_6;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
}


/**
 * @brief AT24C02 EEPROM 连续写函数
 * @note  【警告】该函数内部包含 HAL_Delay(5)！
 * 严禁在主循环中直接使用它保存大数据，必须配合 eeprom_app.c 的异步切片状态机使用。
 */
void eeprom_write(uint8_t *buf, uint8_t addr, uint8_t num) {
    while(num--) {
        I2CStart();
        I2CSendByte(0xA0);   // 写设备地址
        I2CWaitAck();
        I2CSendByte(addr++); // 内存地址
        I2CWaitAck();
        I2CSendByte(*buf++); // 数据字节
        I2CWaitAck();
        I2CStop();
        HAL_Delay(5); // 物理烧录死区时间
    }
}

/**
 * @brief AT24C02 EEPROM 连续读函数
 */
void eeprom_read(uint8_t *buf, uint8_t addr, uint8_t num) {
    I2CStart();
    I2CSendByte(0xA0); // 伪写定位
    I2CWaitAck();
    I2CSendByte(addr);
    I2CWaitAck();

    I2CStart();
    I2CSendByte(0xA1); // 读指令
    I2CWaitAck();
    while(num--) {
        *buf++ = I2CReceiveByte();
        if(num) {
            I2CSendAck();    // 还没读完，给应答
        } else {
            I2CSendNotAck(); // 最后一个字节，给非应答
        }
    }
    I2CStop();
}

/**
 * @brief MCP4017 可编程数字电阻写入档位
 * @param val: 0 ~ 127 (对应 0 ~ 100kΩ)
 */
void mcp4017_write(uint8_t val) {
    I2CStart();
    I2CSendByte(0x5E); // MCP4017 设备地址
    I2CWaitAck();
    I2CSendByte(val);  // 写入游标步进值
    I2CWaitAck();
    I2CStop();
}

/**
 * @brief MCP4017 读取当前游标步进值
 */
uint8_t mcp4017_read(void) {
    uint8_t val;
    I2CStart();
    I2CSendByte(0x5F); 
    I2CWaitAck();
    val = I2CReceiveByte();
    I2CSendNotAck();
    I2CStop();
    return val;
}
