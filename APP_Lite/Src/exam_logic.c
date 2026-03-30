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
        switch (key_val) {
            case 1: 
                if (sys.current_page == PAGE_PARA) {
                    if(sys.key_data==0){
                    sys.PD=sys.PD+100;
                    if(sys.PD>=1000){
                    sys.PD=1000;
                    }
                    }else if(sys.key_data==1){
                    sys.PH=sys.PH+100;
                    if(sys.PH>=10000){
                    sys.PH=10000;
                    }
                    }else if(sys.key_data==2){
                    sys.PX=sys.PX+100;
                    if(sys.PX>=1000){
                    sys.PX=1000;
                    }
                    }
                }
                break;

            case 2: 
                if (sys.current_page == PAGE_PARA) {
                    if(sys.key_data==0){
                    sys.PD=sys.PD-100;
                    if(sys.PD<=100){
                    sys.PD=100;
                    }
                    }else if(sys.key_data==1){
                    sys.PH=sys.PH-100;
                    if(sys.PH<=1000){
                    sys.PH=1000;
                    }
                    }else if(sys.key_data==2){
                    sys.PX=sys.PX-100;
                    if(sys.PX<=-1000){
                    sys.PX=-1000;
                    }
                    }
                }
                break;

            case 3: 
                if (sys.current_page == PAGE_DATA) {
                sys.flag=!sys.flag;
                }else if(sys.current_page == PAGE_PARA){
                if(sys.key_data==0){
                sys.key_data++;
                }else if(sys.key_data==1){
                sys.key_data++;
                }else if(sys.key_data==2){
                sys.key_data=0;
                }
                }

                break;
            case 4:
                if(sys.current_page==PAGE_DATA){
                    sys.current_page=PAGE_PARA;
                    sys.flag = 0;
                }else if(sys.current_page==PAGE_PARA){
                    sys.current_page=PAGE_RECD;
                    sys.key_data = 0;
                }else if(sys.current_page==PAGE_RECD){
                    sys.current_page=PAGE_DATA;
                }
                break;
        }
    }
}

void Logic_Data_Proc(void) {

    static uint16_t time_count = 0;

    static int32_t max_a = 0, min_a = 999999; 
    static int32_t max_b = 0, min_b = 999999;

    if (sys.FA > 0) {
        if (sys.FA > max_a) max_a = sys.FA;
        if (sys.FA < min_a) min_a = sys.FA;
    }
    if (sys.FB > 0) {
        if (sys.FB > max_b) max_b = sys.FB;
        if (sys.FB < min_b) min_b = sys.FB;
    }

    time_count++;
    if (time_count >= 60) {

        if ((max_a - min_a) > sys.PD) {
            sys.NDA++;
        }

        if ((max_b - min_b) > sys.PD) {
            sys.NDB++;
        }

        max_a = 0; min_a = 999999;
        max_b = 0; min_b = 999999;
        time_count = 0;
    }

    adc_proc(); 
    rtc_proc();

    sys.FA=sys.freq_ch1-sys.PX;
    sys.FB=sys.freq_ch2-sys.PX;

    if(sys.Alog==0 && sys.FA>sys.PH){
    sys.Alog=1;
    sys.NHA++;
    }else
    sys.Alog=0;

    if(sys.Blog==0 && sys.FB>sys.PH){
     sys.Blog=1;
     sys.NHB++;
    }else
    sys.Blog=0;

    }

void Logic_LED_Proc(void) {

    for(int i = 0; i < 8; i++) sys.led_ctrl[i] = 0;

    if(sys.current_page==PAGE_DATA){
        sys.led_ctrl[0]=1;
     }else if(sys.current_page==PAGE_PARA){
        sys.led_ctrl[0]=0;
     }
     if(sys.FA>sys.PH)sys.led_ctrl[1]=1;else sys.led_ctrl[1]=0;
     if(sys.FB>sys.PH)sys.led_ctrl[2]=1;else sys.led_ctrl[2]=0;

     if(sys.NDA>3 || sys.NDB>3){
     sys.led_ctrl[7]=1;
     }else {
     sys.led_ctrl[7]=0;
     }

    LED_Disp();
}

void Logic_UART_Proc(void) {

}

static char lcd_vram[10][21];      
static char lcd_vram_bak[10][21];  

void Logic_UI_Proc(void) {
    char temp[32]; 
    float UI_FA;
    float UI_FB;

    static PageState_e last_page = PAGE_DATA;
    if (sys.current_page != last_page) {
        memset(lcd_vram_bak, 0, sizeof(lcd_vram_bak));
        last_page = sys.current_page;
    }

    for(int i = 0; i < 10; i++) {
        sprintf(lcd_vram[i], "                    "); 
    }

    if(sys.current_page == PAGE_DATA){
        sprintf(temp, "        DATA");
        sprintf(lcd_vram[1], "%-20s", temp);
        if(sys.flag==0){
        if(sys.FA>=1000){
            UI_FA=sys.FA/1000;
            sprintf(temp, "     V=%.2fKHz",UI_FA);
            sprintf(lcd_vram[3], "%-20s", temp);
        }else{
        UI_FA=sys.FA;
        sprintf(temp, "     V=%.2fHz",UI_FA);
        sprintf(lcd_vram[3], "%-20s", temp);
        }

        if(sys.FB>=1000){
            UI_FB=sys.FB/1000;
            sprintf(temp, "     V=%.2fKHz",UI_FB);
            sprintf(lcd_vram[4], "%-20s", temp);
        }else{
        UI_FB=sys.FB;
        sprintf(temp, "     V=%.2fHz",UI_FB);
        sprintf(lcd_vram[4], "%-20s", temp);
        }
    }else {
        if (sys.FA > 0) { 
            float period_us = 1000000.0f / sys.FA; 
            if (period_us > 1000.0f) {
                float period_ms = period_us / 1000.0f;
                sprintf(temp, "     A=%.2fmS", period_ms);
            } else {
                sprintf(temp, "     A=%duS", (int)period_us);
            }
        } else {
            sprintf(temp, "     A=NULL"); 
        }
        sprintf(lcd_vram[3], "%-20s", temp);

        if (sys.FB > 0) { 
            float period_us = 1000000.0f / sys.FB; 
            if (period_us > 1000.0f) {
                float period_ms = period_us / 1000.0f;
                sprintf(temp, "     B=%.2fmS", period_ms);
            } else {
                sprintf(temp, "     B=%duS", (int)period_us);
            }
        } else {
            sprintf(temp, "     B=NULL"); 
        }
        sprintf(lcd_vram[4], "%-20s", temp);

    }

    }else if(sys.current_page == PAGE_PARA){

        sprintf(temp, "       PARA");
        sprintf(lcd_vram[1], "%-20s", temp);
        sprintf(temp, "     PD=%dHz",sys.PD);
        sprintf(lcd_vram[3], "%-20s", temp);
        sprintf(temp, "     PH=%dHz",sys.PH);
        sprintf(lcd_vram[4], "%-20s", temp);
        sprintf(temp, "     PX=%dHz",sys.PX);
        sprintf(lcd_vram[5], "%-20s", temp);
        sprintf(temp, "     %d",sys.key_data);
        sprintf(lcd_vram[7], "%-20s", temp);

    }else if(sys.current_page == PAGE_RECD){

        sprintf(temp, "       RECD");
        sprintf(lcd_vram[1], "%-20s", temp);
        sprintf(temp, "     NDA=%dHz",sys.NDA);
        sprintf(lcd_vram[3], "%-20s", temp);
        sprintf(temp, "     NDB=%dHz",sys.NDB);
        sprintf(lcd_vram[4], "%-20s", temp);
        sprintf(temp, "     NHA=%dHz",sys.NHA);
        sprintf(lcd_vram[5], "%-20s", temp);
        sprintf(temp, "     NHB=%dHz",sys.NHB);
        sprintf(lcd_vram[6], "%-20s", temp);

    }

    for(uint8_t i = 0; i < 10; i++) {

        if (strcmp(lcd_vram[i], lcd_vram_bak[i]) != 0) {
            LCD_DisplayStringLine(i * 24, (uint8_t *)lcd_vram[i]); 

            strcpy(lcd_vram_bak[i], lcd_vram[i]);
        }
    }
}
