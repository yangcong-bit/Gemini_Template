// lcd.c
#include "lcd.h"
#include "fonts.h"

static  vu16 TextColor = 0x0000, BackColor = 0xFFFF;
vu16 dummy;

void Delay_LCD(u16 n)
{
    u16 i, j;
    for (i = 0; i < n; ++i)
        for(j = 0; j < 3000; ++j);
}

void REG_8230_Init(void)
{
    LCD_WriteReg(0x0000, 0x0001);
    Delay_LCD(1000);
    LCD_WriteReg(0x0001, 0x0000);
    LCD_WriteReg(0x0010, 0x1790);
    LCD_WriteReg(0x0060, 0x2700);
    LCD_WriteReg(0x0061, 0x0001);
    LCD_WriteReg(0x0046, 0x0002);
    LCD_WriteReg(0x0013, 0x8010);
    LCD_WriteReg(0x0012, 0x80fe);
    LCD_WriteReg(0x0002, 0x0500);
    LCD_WriteReg(0x0003, 0x1030);

    LCD_WriteReg(0x0030, 0x0303);
    LCD_WriteReg(0x0031, 0x0303);
    LCD_WriteReg(0x0032, 0x0303);
    LCD_WriteReg(0x0033, 0x0300);
    LCD_WriteReg(0x0034, 0x0003);
    LCD_WriteReg(0x0035, 0x0303);
    LCD_WriteReg(0x0036, 0x0014);
    LCD_WriteReg(0x0037, 0x0303);
    LCD_WriteReg(0x0038, 0x0303);
    LCD_WriteReg(0x0039, 0x0303);
    LCD_WriteReg(0x003a, 0x0300);
    LCD_WriteReg(0x003b, 0x0003);
    LCD_WriteReg(0x003c, 0x0303);
    LCD_WriteReg(0x003d, 0x1400);

    LCD_WriteReg(0x0092, 0x0200);
    LCD_WriteReg(0x0093, 0x0303);
    LCD_WriteReg(0x0090, 0x080d);
    LCD_WriteReg(0x0003, 0x1018);
    LCD_WriteReg(0x0007, 0x0173);
}

void REG_932X_Init(void)
{
    LCD_WriteReg(R227, 0x3008);   
    LCD_WriteReg(R231, 0x0012); 
    LCD_WriteReg(R239, 0x1231);   
    LCD_WriteReg(R1, 0x0000);   
    LCD_WriteReg(R2, 0x0700);   
    LCD_WriteReg(R3, 0x1030);     
    LCD_WriteReg(R4, 0x0000);     
    LCD_WriteReg(R8, 0x0207);     
    LCD_WriteReg(R9, 0x0000);     
    LCD_WriteReg(R10, 0x0000);    
    LCD_WriteReg(R12, 0x0000);  
    LCD_WriteReg(R13, 0x0000);    
    LCD_WriteReg(R15, 0x0000);  

    LCD_WriteReg(R16, 0x0000);    
    LCD_WriteReg(R17, 0x0007);    
    LCD_WriteReg(R18, 0x0000);  
    LCD_WriteReg(R19, 0x0000);    

    HAL_Delay(200);
    LCD_WriteReg(R16, 0x1690);    
    LCD_WriteReg(R17, 0x0227);  

    HAL_Delay(50);
    LCD_WriteReg(R18, 0x001D);  

    HAL_Delay(50);
    LCD_WriteReg(R19, 0x0800);  
    LCD_WriteReg(R41, 0x0014);  
    LCD_WriteReg(R43, 0x000B);    

    HAL_Delay(50);
    LCD_WriteReg(R32, 0x0000);  
    LCD_WriteReg(R33, 0x0000);  

    LCD_WriteReg(R48, 0x0007);
    LCD_WriteReg(R49, 0x0707);
    LCD_WriteReg(R50, 0x0006);
    LCD_WriteReg(R53, 0x0704);
    LCD_WriteReg(R54, 0x1F04);
    LCD_WriteReg(R55, 0x0004);
    LCD_WriteReg(R56, 0x0000);
    LCD_WriteReg(R57, 0x0706);
    LCD_WriteReg(R60, 0x0701);
    LCD_WriteReg(R61, 0x000F);

    LCD_WriteReg(R80, 0x0000);    
    LCD_WriteReg(R81, 0x00EF);    
    LCD_WriteReg(R82, 0x0000);  
    LCD_WriteReg(R83, 0x013F);  
    LCD_WriteReg(R96, 0x2700);  
    LCD_WriteReg(R97, 0x0001);  
    LCD_WriteReg(R106, 0x0000); 

    LCD_WriteReg(R128, 0x0000);
    LCD_WriteReg(R129, 0x0000);
    LCD_WriteReg(R130, 0x0000);
    LCD_WriteReg(R131, 0x0000);
    LCD_WriteReg(R132, 0x0000);
    LCD_WriteReg(R133, 0x0000);

    LCD_WriteReg(R144, 0x0010);
    LCD_WriteReg(R146, 0x0000);
    LCD_WriteReg(R147, 0x0003);
    LCD_WriteReg(R149, 0x0110);
    LCD_WriteReg(R151, 0x0000);
    LCD_WriteReg(R152, 0x0000);

    LCD_WriteReg(R3, 0x01018);    

    LCD_WriteReg(R7, 0x0173);   
}

void LCD_Init(void)
{
    LCD_CtrlLinesConfig();
    dummy = LCD_ReadReg(0);

    if(dummy == 0x8230)
    {
        REG_8230_Init();
    }
    else
    {
        REG_932X_Init();
    }
    dummy = LCD_ReadReg(0);

}

void LCD_SetTextColor(vu16 Color)
{
    TextColor = Color;
}

void LCD_SetBackColor(vu16 Color)
{
    BackColor = Color;
}

void LCD_ClearLine(u8 Line)
{
    LCD_DisplayStringLine(Line, "                    ");
}

void LCD_Clear(u16 Color)
{
    u32 index = 0;
    LCD_SetCursor(0x00, 0x0000);
    LCD_WriteRAM_Prepare(); 
    for(index = 0; index < 76800; index++)
    {
        LCD_WriteRAM(Color);
    }
}

void LCD_SetCursor(u8 Xpos, u16 Ypos)
{
    LCD_WriteReg(R32, Xpos);
    LCD_WriteReg(R33, Ypos);
}

void LCD_DrawChar(u8 Xpos, u16 Ypos, uc16 *c)
{
    u32 index = 0, i = 0;
    u8 Xaddress = 0;

    Xaddress = Xpos;
    LCD_SetCursor(Xaddress, Ypos);

    for(index = 0; index < 24; index++)
    {
        LCD_WriteRAM_Prepare(); 
        for(i = 0; i < 16; i++)
        {
            if((c[index] & (1 << i)) == 0x00)
            {
                LCD_WriteRAM(BackColor);
            }
            else
            {
                LCD_WriteRAM(TextColor);
            }
        }
        Xaddress++;
        LCD_SetCursor(Xaddress, Ypos);
    }
}

void LCD_DisplayChar(u8 Line, u16 Column, u8 Ascii)
{
    Ascii -= 32;
    LCD_DrawChar(Line, Column, &ASCII_Table[Ascii * 24]);
}

void LCD_DisplayStringLine(u8 Line, u8 *ptr)
{
    u32 i = 0;
    u16 refcolumn = 319;

    while ((*ptr != 0) && (i < 20))	 
    {
        LCD_DisplayChar(Line, refcolumn, *ptr);
        refcolumn -= 16;
        ptr++;
        i++;
    }
}

void LCD_SetDisplayWindow(u8 Xpos, u16 Ypos, u8 Height, u16 Width)
{
    if(Xpos >= Height)
    {
        LCD_WriteReg(R80, (Xpos - Height + 1));
    }
    else
    {
        LCD_WriteReg(R80, 0);
    }
    LCD_WriteReg(R81, Xpos);
    if(Ypos >= Width)
    {
        LCD_WriteReg(R82, (Ypos - Width + 1));
    }
    else
    {
        LCD_WriteReg(R82, 0);
    }

    LCD_WriteReg(R83, Ypos);
    LCD_SetCursor(Xpos, Ypos);
}

void LCD_WindowModeDisable(void)
{
    LCD_SetDisplayWindow(239, 0x13F, 240, 320);
    LCD_WriteReg(R3, 0x1018);
}

void LCD_DrawLine(u8 Xpos, u16 Ypos, u16 Length, u8 Direction)
{
    u32 i = 0;

    LCD_SetCursor(Xpos, Ypos);
    if(Direction == Horizontal)
    {
        LCD_WriteRAM_Prepare(); 
        for(i = 0; i < Length; i++)
        {
            LCD_WriteRAM(TextColor);
        }
    }
    else
    {
        for(i = 0; i < Length; i++)
        {
            LCD_WriteRAM_Prepare(); 
            LCD_WriteRAM(TextColor);
            Xpos++;
            LCD_SetCursor(Xpos, Ypos);
        }
    }
}

void LCD_DrawRect(u8 Xpos, u16 Ypos, u8 Height, u16 Width)
{
    LCD_DrawLine(Xpos, Ypos, Width, Horizontal);
    LCD_DrawLine((Xpos + Height), Ypos, Width, Horizontal);

    LCD_DrawLine(Xpos, Ypos, Height, Vertical);
    LCD_DrawLine(Xpos, (Ypos - Width + 1), Height, Vertical);
}

void LCD_DrawCircle(u8 Xpos, u16 Ypos, u16 Radius)
{
    s32  D;
    u32  CurX;
    u32  CurY;

    D = 3 - (Radius << 1);
    CurX = 0;
    CurY = Radius;

    while (CurX <= CurY)
    {
        LCD_SetCursor(Xpos + CurX, Ypos + CurY);
        LCD_WriteRAM_Prepare(); 
        LCD_WriteRAM(TextColor);

        LCD_SetCursor(Xpos + CurX, Ypos - CurY);
        LCD_WriteRAM_Prepare(); 
        LCD_WriteRAM(TextColor);

        LCD_SetCursor(Xpos - CurX, Ypos + CurY);
        LCD_WriteRAM_Prepare(); 
        LCD_WriteRAM(TextColor);

        LCD_SetCursor(Xpos - CurX, Ypos - CurY);
        LCD_WriteRAM_Prepare(); 
        LCD_WriteRAM(TextColor);

        LCD_SetCursor(Xpos + CurY, Ypos + CurX);
        LCD_WriteRAM_Prepare(); 
        LCD_WriteRAM(TextColor);

        LCD_SetCursor(Xpos + CurY, Ypos - CurX);
        LCD_WriteRAM_Prepare(); 
        LCD_WriteRAM(TextColor);

        LCD_SetCursor(Xpos - CurY, Ypos + CurX);
        LCD_WriteRAM_Prepare(); 
        LCD_WriteRAM(TextColor);

        LCD_SetCursor(Xpos - CurY, Ypos - CurX);
        LCD_WriteRAM_Prepare(); 
        LCD_WriteRAM(TextColor);

        if (D < 0)
        {
            D += (CurX << 2) + 6;
        }
        else
        {
            D += ((CurX - CurY) << 2) + 10;
            CurY--;
        }
        CurX++;
    }
}

void LCD_DrawMonoPict(uc32 *Pict)
{
    u32 index = 0, i = 0;

    LCD_SetCursor(0, 319);

    LCD_WriteRAM_Prepare(); 
    for(index = 0; index < 2400; index++)
    {
        for(i = 0; i < 32; i++)
        {
            if((Pict[index] & (1 << i)) == 0x00)
            {
                LCD_WriteRAM(BackColor);
            }
            else
            {
                LCD_WriteRAM(TextColor);
            }
        }
    }
}

void LCD_WriteBMP(u32 BmpAddress)
{
    u32 index = 0, size = 0;

    size = *(vu16 *) (BmpAddress + 2);
    size |= (*(vu16 *) (BmpAddress + 4)) << 16;

    index = *(vu16 *) (BmpAddress + 10);
    index |= (*(vu16 *) (BmpAddress + 12)) << 16;
    size = (size - index) / 2;
    BmpAddress += index;

    LCD_WriteReg(R3, 0x1008);
    LCD_WriteRAM_Prepare();
    for(index = 0; index < size; index++)
    {
        LCD_WriteRAM(*(vu16 *)BmpAddress);
        BmpAddress += 2;
    }
    LCD_WriteReg(R3, 0x1018);
}

void LCD_WriteReg(u8 LCD_Reg, u16 LCD_RegValue)
{
    GPIOB->BRR  = GPIO_PIN_9;
    GPIOB->BRR  = GPIO_PIN_8;
    GPIOB->BSRR = GPIO_PIN_5;

    GPIOC->ODR = LCD_Reg;
    GPIOB->BRR  = GPIO_PIN_5;
    __nop();
    __nop();
    __nop();
    GPIOB->BSRR = GPIO_PIN_5;
    GPIOB->BSRR = GPIO_PIN_8;

    GPIOC->ODR = LCD_RegValue;
    GPIOB->BRR  = GPIO_PIN_5;
    __nop();
    __nop();
    __nop();
    GPIOB->BSRR = GPIO_PIN_5;
    GPIOB->BSRR = GPIO_PIN_8;
}

u16 LCD_ReadReg(u8 LCD_Reg)
{
    u16 temp;

    GPIOB->BRR = GPIO_PIN_9;
    GPIOB->BRR = GPIO_PIN_8;
    GPIOB->BSRR = GPIO_PIN_5;

    GPIOC->ODR = LCD_Reg;
    GPIOB->BRR = GPIO_PIN_5;
    __nop();
    __nop();
    __nop();
    GPIOB->BSRR = GPIO_PIN_5;
    GPIOB->BSRR = GPIO_PIN_8;

    LCD_BusIn();
    GPIOA->BRR = GPIO_PIN_8;
    __nop();
    __nop();
    __nop();
    temp = GPIOC->IDR;
    GPIOA->BSRR = GPIO_PIN_8;

    LCD_BusOut();
    GPIOB->BSRR = GPIO_PIN_9;

    return temp;
}

void LCD_WriteRAM_Prepare(void)
{
    GPIOB->BRR  =  GPIO_PIN_9;
    GPIOB->BRR  =  GPIO_PIN_8;
    GPIOB->BSRR =  GPIO_PIN_5;

    GPIOC->ODR = R34;
    GPIOB->BRR  =  GPIO_PIN_5;
    __nop();
    __nop();
    __nop();
    GPIOB->BSRR =  GPIO_PIN_5;
    GPIOB->BSRR =  GPIO_PIN_8;
    __nop();
    __nop();
    __nop();
    GPIOB->BSRR =  GPIO_PIN_9;
}

void LCD_WriteRAM(u16 RGB_Code)
{
    GPIOB->BRR  =  GPIO_PIN_9;
    GPIOB->BSRR =  GPIO_PIN_8;
    GPIOB->BSRR =  GPIO_PIN_5;

    GPIOC->ODR = RGB_Code;
    GPIOB->BRR  =  GPIO_PIN_5;
    __nop();
    __nop();
    __nop();
    GPIOB->BSRR =  GPIO_PIN_5;
    GPIOB->BSRR =  GPIO_PIN_8;
    __nop();
    __nop();
    __nop();
    GPIOB->BSRR =  GPIO_PIN_9;
}

u16 LCD_ReadRAM(void)
{
    u16 temp;

    GPIOB->BRR  =  GPIO_PIN_9;
    GPIOB->BRR  =  GPIO_PIN_8;
    GPIOB->BSRR =  GPIO_PIN_5;

    GPIOC->ODR = R34;
    GPIOB->BRR  =  GPIO_PIN_5;
    __nop();
    __nop();
    __nop();
    GPIOB->BSRR =  GPIO_PIN_5;
    GPIOB->BSRR =  GPIO_PIN_8;

    LCD_BusIn();
    GPIOA->BRR =  GPIO_PIN_8;
    __nop();
    __nop();
    __nop();
    temp = GPIOC->IDR;
    GPIOA->BSRR =  GPIO_PIN_8;

    LCD_BusOut();
    GPIOB->BSRR =  GPIO_PIN_9;

    return temp;
}

void LCD_PowerOn(void)
{
    LCD_WriteReg(R16, 0x0000);
    LCD_WriteReg(R17, 0x0000);
    LCD_WriteReg(R18, 0x0000);
    LCD_WriteReg(R19, 0x0000);
    Delay_LCD(20);
    LCD_WriteReg(R16, 0x17B0);
    LCD_WriteReg(R17, 0x0137);
    Delay_LCD(5);
    LCD_WriteReg(R18, 0x0139);
    Delay_LCD(5);
    LCD_WriteReg(R19, 0x1d00);
    LCD_WriteReg(R41, 0x0013);
    Delay_LCD(5);
    LCD_WriteReg(R7, 0x0173);
}

void LCD_DisplayOn(void)
{
    LCD_WriteReg(R7, 0x0173);
}

void LCD_DisplayOff(void)
{
    LCD_WriteReg(R7, 0x0);
}

void LCD_CtrlLinesConfig(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin =  GPIO_PIN_8 ;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    LCD_BusOut();

    GPIOA->BSRR = 0x0100;
    GPIOB->BSRR = 0x0220;
}

void LCD_BusIn(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_PIN_All;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void LCD_BusOut(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_PIN_All;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void LCD_DrawPicture(const u8 *picture)
{
    int index;
    LCD_SetCursor(0x00, 0x0000);

    LCD_WriteRAM_Prepare(); 

    for(index = 0; index < 76800; index++)
    {
        LCD_WriteRAM(picture[2 * index + 1] << 8 | picture[2 * index]);
    }
}
