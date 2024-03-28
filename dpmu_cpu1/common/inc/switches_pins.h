/*
 * switches_pins.h
 *
 *  Created on: 5 feb. 2024
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#ifndef COMMON_INC_SWITCHES_PINS_H_
#define COMMON_INC_SWITCHES_PINS_H_


#include "board.h"

#define Qinrush GLOAD_1      /* in-rush current limiter - switch pin, 168 D4 */
#define Qlb     GLOAD_2      /* load                    - switch pin, 167 C4 */
#define Qsb     GLOAD_4_3_EPWMB_GPIO    /* sharing bus  - switch pin, 162 D9 */
#define Qinb    GLOAD_4_3_EPWMA_GPIO    /* input DC bus - switch pin, 161 C9 */


#endif /* COMMON_INC_SWITCHES_PINS_H_ */
