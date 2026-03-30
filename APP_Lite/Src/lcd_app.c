// lcd_app.c
#include "lcd_app.h"
#include "lcd.h"           
#include "key_app.h"       
#include "global_system.h" 
#include <stdio.h>
#include <string.h>

static char lcd_vram[10][21];      
static char lcd_vram_bak[10][21];  

void UI_Init(void) {
    LCD_Init();                 
    LCD_Clear(Black);           
    LCD_SetBackColor(Black);    
    LCD_SetTextColor(White);    

    memset(lcd_vram, 0, sizeof(lcd_vram));
    memset(lcd_vram_bak, 0, sizeof(lcd_vram_bak));
}
