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

float PI=3.14;

void Logic_Ctrl_Proc(void) {
    uint8_t key_val = 0;

    while (Key_Get_Event(&key_val)) {

                 switch (key_val) {
            case 1:
            if(sys.current_page== PAGE_DATA){
            sys.current_page= PAGE_PARA;
            sys.para_select=0;
            }else if(sys.current_page== PAGE_PARA){
            sys.current_page= PAGE_RECD;
                sys.NAME_R=sys.TMP_R;
                sys.NAME_K=sys.TMP_K;
            }else if(sys.current_page== PAGE_RECD){
            sys.current_page= PAGE_DATA;
            }
                break;

            case 2: 
                if (sys.current_page == PAGE_PARA) {
                sys.para_select=! sys.para_select;
                }else if(sys.current_page == PAGE_DATA){
                    if(sys.M_flag==0){          
                    sys.NAME_M=!sys.NAME_M;
                    sys.NAME_N++;
                    sys.M_flag=1;
                    if(sys.NAME_M==0){
                    sys.start_freq=8000;
                    sys.target_freq=4000;
                    }else if(sys.NAME_M==1){
                    sys.start_freq=4000;
                    sys.target_freq=8000;
                        }
                    }

                }
                break;

            case 3: 
                if (sys.current_page == PAGE_PARA) {
                    if(sys.para_select==0){
                    sys.TMP_R++;
                    if(sys.TMP_R > 10) sys.TMP_R = 1; 
                    }else if(sys.para_select==1){
                    sys.TMP_K++;
                    if(sys.TMP_K > 10) sys.TMP_K = 1; 
                    }
                }
                break;

            case 4: 
                if (sys.current_page == PAGE_PARA) {
                    if(sys.para_select==0){
                    sys.TMP_R--;
                        if(sys.TMP_R < 1) sys.TMP_R = 10; 
                    }else if(sys.para_select==1){
                    sys.TMP_K--;
                        if(sys.TMP_K < 1) sys.TMP_K = 10; 
                    }
                }
                break;

            case 14: 
                if (sys.current_page == PAGE_DATA){

                sys.is_locked =! sys.is_locked;

                }

                break;
        }

    }
}

void Logic_Data_Proc(void) {

    static uint16_t TIME_M;
    static float M_STEP;

    adc_proc(); 
    rtc_proc(); 

    if(sys.M_flag==1){
        TIME_M++;
        M_STEP=(sys.target_freq-sys.start_freq)/500;
        sys.pwm_freq=sys.start_freq+(TIME_M*M_STEP);
        if(TIME_M>=500){
        sys.M_flag=0;
            TIME_M=0;
            sys.pwm_freq=sys.target_freq;
        }
    }

    sys.NAME_V=sys.freq_ch1*2*PI*sys.NAME_R/(100*sys.NAME_K);

    if(sys.is_locked==0){
        if(sys.r37_voltage<=1){
        sys.pwm_duty=0.1;
        }else if(1<sys.r37_voltage && sys.r37_voltage<3){
        sys.pwm_duty=0.1+((sys.r37_voltage-1)/2)*0.75;
        }else if(sys.r37_voltage>=3){
        sys.pwm_duty=0.85;
        }
    }

    sys.NAME_P=sys.pwm_duty*100;

    static float candidate_speed = 0.0f; 
    static uint16_t speed_timer = 0;     

    float diff = sys.NAME_V - candidate_speed;
    if(diff < 0) diff = -diff;

    if(diff < 0.5f){
        speed_timer++;
        if(speed_timer >= 200){ 

            if(sys.NAME_M == 1 && candidate_speed > sys.NAME_MH) {
                sys.NAME_MH = candidate_speed;
            }
            if(sys.NAME_M == 0 && candidate_speed > sys.NAME_ML) {
                sys.NAME_ML = candidate_speed;
            }
            speed_timer = 200; 
        }
    }else{

        candidate_speed = sys.NAME_V;
        speed_timer = 0;
    }

}

void Logic_LED_Proc(void) {

    for(int i = 0; i < 8; i++) sys.led_ctrl[i] = 0;

    if (sys.current_page == PAGE_DATA) {
        sys.led_ctrl[0] = 1; 
    }else{
        sys.led_ctrl[0] = 0; 
    }

    if(sys.M_flag==1){

    if(HAL_GetTick()%200>100){
        sys.led_ctrl[1] = 1;
    }else{
        sys.led_ctrl[1] = 0;
        }
    }else{
        sys.led_ctrl[1] = 0;
    }

    if(sys.is_locked==1){
    sys.led_ctrl[2] = 1;
    }else{
    sys.led_ctrl[2] = 0;
    }

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

void Logic_UI_Proc(void) {
    char temp[32]; 

    static PageState_e last_page = PAGE_DATA;
    if (sys.current_page != last_page) {
        memset(lcd_vram_bak, 0, sizeof(lcd_vram_bak)); 
        last_page = sys.current_page;
    }

    for(int i = 0; i < 10; i++) {
        sprintf(lcd_vram[i], "                    "); 
    }

         if(sys.current_page == PAGE_DATA) {
         sprintf(temp, "        DATA");
         sprintf(lcd_vram[1], "%-20s", temp);     

         sprintf(temp, "     M=%d", sys.NAME_M);
         sprintf(lcd_vram[3], "%-20s", temp);     

         sprintf(temp, "     P=%d%%", (int)sys.NAME_P);
         sprintf(lcd_vram[4], "%-20s", temp);

         sprintf(temp, "     V=%.1f", sys.NAME_V);
         sprintf(lcd_vram[5], "%-20s", temp);

        } 
         else if(sys.current_page == PAGE_PARA) {
         sprintf(temp, "        PARA     ");
         sprintf(lcd_vram[1], "%-20s", temp);     

         sprintf(temp, "     R=%d", sys.TMP_R);
         sprintf(lcd_vram[3], "%-20s", temp);

         sprintf(temp, "     R=%d", sys.TMP_K);
         sprintf(lcd_vram[4], "%-20s", temp);

        } else if(sys.current_page == PAGE_RECD) {
         sprintf(temp, "        RECD");
         sprintf(lcd_vram[1], "%-20s", temp);     

         sprintf(temp, "     N=%d", sys.NAME_N);
         sprintf(lcd_vram[3], "%-20s", temp);

         sprintf(temp, "     MH=%.1f", sys.NAME_MH);
         sprintf(lcd_vram[4], "%-20s", temp);

         sprintf(temp, "     ML=%.1f", sys.NAME_ML);
         sprintf(lcd_vram[5], "%-20s", temp);

        }

    for(uint8_t i = 0; i < 10; i++) {

        if (strcmp(lcd_vram[i], lcd_vram_bak[i]) != 0) {
            LCD_DisplayStringLine(i * 24, (uint8_t *)lcd_vram[i]);
            strcpy(lcd_vram_bak[i], lcd_vram[i]); 
        }
    }
}
