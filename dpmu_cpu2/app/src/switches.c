/*
 * switches.c
 *
 *  Created on: 7 nov. 2022
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

/* system */
#include "DCDC.h"
#include "stdbool.h"

/* local */
#include "cli_cpu2.h"
#include "device.h"
#include "state_machine.h"
#include "switches.h"

void switches_Qlb(uint8_t state)
{
    GPIO_writePin(Qlb, (state == SW_ON ? 1: 0));
}

void switches_Qsb(uint8_t state)
{//GLOAD_3
    PRINT("FILE:%s LINE:%d FUNCTION:%s\r\n", __FILE__, __LINE__, __FUNCTION__);
    if(SW_ON == state)
        TurnOnGload_3();
    else
        TurnOffGload_3();
}

void switches_Qinb(uint8_t state)
{//GLOAD_4
    if(SW_ON == state)
        TurnOnGload_4();
    else
        TurnOffGload_4();
}

void switches_test_cont_toggling(void)
{
    int state = SW_OFF;
    uint32_t delay = (uint32_t)100*1000; /* 100 ms */
    for(;;) {
        switches_Qinrush_digital(state);
        DEVICE_DELAY_US(delay);
        switches_Qlb(state);
        DEVICE_DELAY_US(delay);
        switches_Qsb(state);
        DEVICE_DELAY_US(delay);
        switches_Qinb(state);
        DEVICE_DELAY_US(delay);

        /*change state */
        state = (state == SW_OFF ? SW_ON : SW_OFF);
    }
}


