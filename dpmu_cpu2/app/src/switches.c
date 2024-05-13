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

/*** 26.15.1.5.3 Duty Cycle Range Limitation
 *
 * When using DBREDHR or DBFEDHR, DBRED and/or DBFED (the register corresponding
 * to the edge with hi-resolution displacement) must be greater than or equal to 7
***/
//bool switches_Qinrush(uint8_t state)
//{
//    int return_value = 1;
//
//    start_inrush_current_limiter(state);
//
//    return return_value;
//}

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

//uint8_t switches_read_state(void)
//{
//    uint8_t pinValues = 0;
//    uint8_t tmp;
//
//    tmp = GPIO_readPin(Qinrush);    // enkel pinne, på pinnen
//    pinValues |= tmp << 0;
//    tmp = GPIO_readPin(Qinb);       // enkel pinne, på pinnen
//    pinValues |= tmp << 1;
//    tmp = GPIO_readPin(Qlb);        // enkel pinne, på pinnen
//    pinValues |= tmp << 2;
//    tmp = GPIO_readPin(Qsb);        // enkel pinne, på pinnen
//    pinValues |= tmp << 3;
//
//    return pinValues;
//}
