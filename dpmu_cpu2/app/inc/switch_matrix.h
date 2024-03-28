/*
 * switch_matrix.h
 *
 *  Created on: 18 nov. 2022
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#ifndef APP_INC_SWITCH_MATRIX_H_
#define APP_INC_SWITCH_MATRIX_H_

#include "board.h"
#include "stdbool.h"
#include "stdint.h"

#define LOGIC_SETUP_TIME 10
#define NUMBER_OF_CELLS 30

enum switch_matrix_battery_connections {
                                        BAT_0 = 0,
                                        BAT_1,
                                        BAT_2,
                                        BAT_3,
                                        BAT_4,
                                        BAT_5,
                                        BAT_6,
                                        BAT_7,
                                        BAT_8,
                                        BAT_9,
                                        BAT_10,
                                        BAT_11,
                                        BAT_12,
                                        BAT_13,
                                        BAT_14,
                                        BAT_15,
                                        BAT_15_N,
                                        BAT_16,
                                        BAT_17,
                                        BAT_18,
                                        BAT_19,
                                        BAT_20,
                                        BAT_21,
                                        BAT_22,
                                        BAT_23,
                                        BAT_24,
                                        BAT_25,
                                        BAT_26,
                                        BAT_27,
                                        BAT_28,
                                        BAT_29,
                                        BAT_30
};


/* polarity select
 *
 * used to control the polarity switch
 * select MATPCMD[0..3] by these signals */
#define N_DATA GCMD0    /* polarity switch data line */
// N_OE_POL
// N_LE_POL_0
// N_LE_POL_1
// MPOS0
// MPOS1

#define CELL_SWITCH_MATRIX_LOW_ENABLE_N  GCMD7
#define CELL_SWITCH_MATRIX_HIGH_ENABLE_N GCMD8

extern uint16_t cellNrReadOrder[];




/* address group */
#define GROUP_LOW_EVEN  GCMD3 /* MATGCMD0  .. MATGCMD7  - even numbers */
#define GROUP_LOW_ODD   GCMD5 /* MATGCMD1  .. MATGCMD15 - odd  numbers */
#define GROUP_HIGH_EVEN GCMD4 /* MATGCMD16 .. MATGCMD30 - even numbers */
#define GROUP_HIGH_ODD  GCMD6 /* MATGCMD17 .. MATGCMD31 - odd  numbers */


/* connection in battery
 *
 * connect battery to polarity switch PSWITCHB[0,1,16,17]
 *
 * MATGCMD0  connects BAT0 to PSWITCHB0
 * MATGCMD1  connects BAT1 to PSWITCHB1
 * MATGCMD2  connects BAT2 to PSWITCHB0
 * MATGCMD3  connects BAT3 to PSWITCHB1
 * ...
 * MATGCMD15 connects BAT15 to PSWITCHB1
 * MATGCMD16 connects BAT15 to PSWITCHB16
 * MATGCMD17 connects BAT16 to PSWITCHB17
 * MATGCMD18 connects BAT17 to PSWITCHB16
 * ...
 *
 * */
/*      Signal    (demux adress in signals) */
//#define MATGCMD0  (0     | 0     | 0     )
//#define MATGCMD1  (0     | 0     | GCMD0 )
//#define MATGCMD2  (0     | GCMD1 | 0     )
//#define MATGCMD3  (0     | GCMD1 | GCMD0 )
//#define MATGCMD4  (GCMD2 | 0     | 0     )
//#define MATGCMD5  (GCMD2 | 0     | GCMD0 )
//#define MATGCMD6  (GCMD2 | GCMD1 | 0     )
//#define MATGCMD7  (GCMD2 | GCMD1 | GCMD0 )
//#define MATGCMD8  MATGCMD0
//#define MATGCMD9  MATGCMD1
//#define MATGCMD10 MATGCMD2
//#define MATGCMD11 MATGCMD3
//#define MATGCMD12 MATGCMD4
//#define MATGCMD13 MATGCMD5
//#define MATGCMD14 MATGCMD6
//#define MATGCMD15 MATGCMD7
//#define MATGCMD16 MATGCMD0
//#define MATGCMD17 MATGCMD1
//#define MATGCMD18 MATGCMD2
//#define MATGCMD19 MATGCMD3
//#define MATGCMD20 MATGCMD4
//#define MATGCMD21 MATGCMD5
//#define MATGCMD22 MATGCMD6
//#define MATGCMD23 MATGCMD7
//#define MATGCMD24 MATGCMD0
//#define MATGCMD25 MATGCMD1
//#define MATGCMD26 MATGCMD2
//#define MATGCMD27 MATGCMD3
//#define MATGCMD28 MATGCMD4
//#define MATGCMD29 MATGCMD5
//#define MATGCMD30 MATGCMD6
//#define MATGCMD31 MATGCMD7


/* cell selection
 *
 * BAT_0    MATGCMD0 connects BAT0 to PSWITCHB0
 * BAT_1    MATGCMD1 connects BAT1 to PSWITCHB1
 * ...
 * BAT_14   MATGCMD14 connects BAT14 to PSWITCHB0
 * BAT_15   MATGCMD1 connects BAT1 to PSWITCHB1
 * BAT_15_N MATGCMD16 connects BAT15 to PSWITCHB16
 * BAT_16   MATGCMD17 connects BAT16 to PSWITCHB17
 * ...
 *
 **/
//#define BAT_0    (MATGCMD0 ) /* BAT0 to PSWITCHB0 */
//#define BAT_1    (MATGCMD1 ) /* BAT1 to PSWITCHB1 */
//#define BAT_2    (MATGCMD2 ) /* BAT2 to PSWITCHB0 */
//#define BAT_3    (MATGCMD3 )
//#define BAT_4    (MATGCMD4 )
//#define BAT_5    (MATGCMD5 )
//#define BAT_6    (MATGCMD6 )
//#define BAT_7    (MATGCMD7 )
//#define BAT_8    (MATGCMD8 )
//#define BAT_9    (MATGCMD9 )
//#define BAT_10   (MATGCMD10)
//#define BAT_11   (MATGCMD11)
//#define BAT_12   (MATGCMD12)
//#define BAT_13   (MATGCMD13)
//#define BAT_14   (MATGCMD14) /* BAT14 to PSWITCHB0 */
//#define BAT_15   (MATGCMD15) /* BAT15 to PSWITCHB1 */
//#define BAT_15_N (MATGCMD16) /* BAT15 to PSWITCHB16 */
//#define BAT_16   (MATGCMD17) /* BAT16 to PSWITCHB17 */
//#define BAT_17   (MATGCMD18) /* BAT17 to PSWITCHB16 */
//#define BAT_18   (MATGCMD19)
//#define BAT_19   (MATGCMD20)
//#define BAT_20   (MATGCMD21)
//#define BAT_21   (MATGCMD22)
//#define BAT_22   (MATGCMD23)
//#define BAT_23   (MATGCMD24)
//#define BAT_24   (MATGCMD25)
//#define BAT_25   (MATGCMD26)
//#define BAT_26   (MATGCMD27)
//#define BAT_27   (MATGCMD28)
//#define BAT_28   (MATGCMD29)
//#define BAT_29   (MATGCMD30)
//#define BAT_30   (MATGCMD31)


/* turns off all cell selection and polarity lines
 * MATGCMD[0..31] = HIGH */
void switch_matrix_reset(void);

/* connects cell <battery_number> to llc */
int switch_matrix_connect_cell(uint16_t battery_number);
//void ActiveMatrixSwitches(void);
void switch_matrix_set_cell_polarity(uint16_t cell_number);

#endif /* APP_INC_SWITCH_MATRIX_H_ */
