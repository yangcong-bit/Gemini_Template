// led_app.c
#include "led_app.h"
#include "global_system.h" 

void LED_Disp(void) {
    uint32_t odr_backup = GPIOC->ODR; 
    uint8_t led_status = 0;           

    for(int i = 0; i < 8; i++) {
        if(sys.led_ctrl[i] != 0) {
            led_status |= (1 << i);   
        }
    }

    GPIOC->ODR = (odr_backup & 0x00FF) | ((~led_status & 0xFF) << 8);

    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);   
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET); 

    GPIOC->ODR = odr_backup;
}
