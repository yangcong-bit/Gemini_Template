// mcp4017_app.c
#include "mcp4017_app.h"
#include "i2c_hal.h"       
#include "global_system.h" 

static uint8_t last_res_step = 0xFF; 

void MCP4017_Init(void) {

    mcp4017_write(sys.res_step);
    last_res_step = sys.res_step;
}

void MCP4017_Proc(void) {

    if (sys.res_step > 127) {
        sys.res_step = 127; 
    }

    if (sys.res_step != last_res_step) {

        mcp4017_write(sys.res_step); 
        last_res_step = sys.res_step; 
    }
}
