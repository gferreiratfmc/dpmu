/*
 * switches.h
 *
 *  Created on: 7 nov. 2022
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#ifndef APP_INC_SWITCHES_H_
#define APP_INC_SWITCHES_H_

/* system */
#include "board.h"
#include "common.h"
#include "driverlib.h"
#include "hal.h"

/* local */

/* SWITHCES */
#define Qinrush GLOAD_1      /* in-rush current limiter - switch pin, 168 D4 */
#define Qlb     GLOAD_2      /* load                    - switch pin, 167 C4 */
#define Qsb     GLOAD_4_3_EPWMB_GPIO    /* sharing bus  - switch pin, 162 D9 */
#define Qinb    GLOAD_4_3_EPWMA_GPIO    /* input DC bus - switch pin, 161 C9 */

#define QINRUSH_MASK 0x0008   //0b00001000
#define QLB_MASK     0x0004   //0b00000100
#define QSB_MASK     0x0002   //0b00000010
#define QINB_MASK    0x0001   //0b00000001

void switches_Qinrush_digital(uint8_t state);   /* in-rush current limiter */
bool switches_Qinrush(uint8_t state);           /* execute in-rush current limiter to charge DC bus up to Voltage_at_DC_Bus */
void switches_Qlb(uint8_t state);       /* load bus                */
void switches_Qsb(uint8_t state);       /* sharing bus             */
void switches_Qinb(uint8_t state);      /* input DC bus            */

/* for board testing */
void switches_test_cont_toggling(void);

#endif /* APP_INC_SWITCHES_H_ */
