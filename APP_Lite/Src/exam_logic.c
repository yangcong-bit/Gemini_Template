// exam_logic.c
#include "exam_logic.h"
#include "global_system.h" 

#include "key_app.h"
#include "led_app.h"
#include "lcd.h"
#include "uart_app.h"
#include "eeprom_app.h"
#include "adc_app.h"
#include "rtc_app.h"
#include <stdio.h>
#include <string.h>

void Logic_Ctrl_Proc(void) {
    uint8_t key_val = 0;

    while (Key_Get_Event(&key_val)) {

    }
}

void Logic_Data_Proc(void) {

    adc_proc(); 
    rtc_proc(); 

}

void Logic_LED_Proc(void) {

    for(int i = 0; i < 8; i++) sys.led_ctrl[i] = 0;

    LED_Disp();
}

void Logic_UART_Proc(void) {

    if (sys.uart_rx_ready == true) { 

        sys.uart_rx_ready = false;               
        memset(uart_rx_buf, 0, UART_RX_MAX_LEN); 
        uart_rx_len = 0;
    }
}

static char lcd_vram[10][21];      
static char lcd_vram_bak[10][21];  

static uint16_t lcd_color[10];         
static uint16_t lcd_color_bak[10];     
static uint16_t lcd_bg_color[10];      
static uint16_t lcd_bg_color_bak[10];  

void Logic_UI_Proc(void) {
    char temp[32]; 

    static PageState_e last_page = PAGE_DATA;
    if (sys.current_page != last_page) {
        memset(lcd_vram_bak, 0, sizeof(lcd_vram_bak)); 
        memset(lcd_color_bak, 0, sizeof(lcd_color_bak)); 
        memset(lcd_bg_color_bak, 0, sizeof(lcd_bg_color_bak)); 
        last_page = sys.current_page;
    }

    for(int i = 0; i < 10; i++) {
        sprintf(lcd_vram[i], "                    "); 
        lcd_color[i] = White;    
        lcd_bg_color[i] = Black; 
    }

    for(uint8_t i = 0; i < 10; i++) {

        if (strcmp(lcd_vram[i], lcd_vram_bak[i]) != 0 || 
            lcd_color[i] != lcd_color_bak[i] || 
            lcd_bg_color[i] != lcd_bg_color_bak[i]) {

            LCD_SetTextColor(lcd_color[i]);
            LCD_SetBackColor(lcd_bg_color[i]);

            LCD_DisplayStringLine(i * 24, (uint8_t *)lcd_vram[i]); 

            strcpy(lcd_vram_bak[i], lcd_vram[i]); 
            lcd_color_bak[i] = lcd_color[i];
            lcd_bg_color_bak[i] = lcd_bg_color[i];
        }
    }

    LCD_SetTextColor(White);
    LCD_SetBackColor(Black);
}
