/*
 * switch_matrix.c
 *
 *  Created on: 18 nov. 2022
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#include "GlobalV.h"
#include "switch_matrix.h"



/* turns off all cell selection and polarity lines
 *
 * disable outputs of demuxes (cell address lines)
 * GCMD7 = HIGH
 * -> MATGCMD[0..31] = HIGH
 *
 * latch disable of demuxes (cell address lines)
 * GCMD[3..6] = HIGH
 *
 * disable outputs of polarity switch
 * N_*_POL* = HIGH
 * -> MATPCMD[0..3] = HIGH
 * -> PSWITCH[0..1] = FLOATING
 *
 * parameter
 *      none
 *
 * returns
 *      none
 *
 * assumptions:
 *      none
 * */
void switch_matrix_reset(void)
{
    /* disable cell switch matrix
     * all cell address lines are high (active low)
     * connected to no individual cells*/
    GPIO_writePin(GCMD7, 1); //GPIO_writePin(GCMD7, 1);     /* OE1 (GCMD7)      active low */
    GPIO_writePin(GCMD8, 1); //GPIO_writePin(GCMD8, 1);    /* OE1 (GCMD7_HIGH) active low */

    /* unselect all cell groups
     * after this no cell groups can be selected without lower its group LE pin (latch enable) */
    GPIO_writePin(GCMD3,  1);  /*GCMD3  [BAT1..BAT15] even numbers  */
    GPIO_writePin(GCMD5,   1);  /*GCMD5  [BAT1..BAT15] odd  numbers  */
    GPIO_writePin(GCMD4, 1);  /*GCMD4 [BAT16..BAT30] even numbers */
    GPIO_writePin(GCMD6,  1);  /* GCMD6 [BAT16..BAT30] odd  numbers */

    /* disable output from polarity switch */
    GPIO_writePin(N_OE_POL,   1); /* disable output */
    GPIO_writePin(N_LE_POL_0, 1); /* disable output for cell GROUP_LOW_x  [BAT1..BAT15]  */
    GPIO_writePin(N_LE_POL_1, 1); /* disable output for cell GROUP_HIGH_x [BAT16..BAT30] */

    DEVICE_DELAY_US( LOGIC_SETUP_TIME );

}

/* set the correct polarity for the cell connecting to the CLLC
 *
 * parameter
 *      cell_number - the cell number that is about to be connected
 *
 * returns
 *      none
 *
 * assumptions:
 *      none
 * */
void switch_matrix_set_cell_polarity(uint16_t cell_number)
{
    int polarity = 0;
    /* Data input for D-switch, 0 for BAT[odd], 1 for BAT[even] */
    polarity = (cell_number + 1) & 0x01; // take last bit of cell number (cell address)

    //Disable outputs
    GPIO_writePin(N_OE_POL, 1);

    DEVICE_DELAY_US( LOGIC_SETUP_TIME );

    /* latch enable to shift in the input */
    if(cell_number <= 15) {
        /* low cell group, BAT1..BAT15 */
        GPIO_writePin(N_LE_POL_0, 0);
    } else {
        /* high cell group, BAT16..BAT30 */
        GPIO_writePin(N_LE_POL_1, 0);
    }

    DEVICE_DELAY_US( LOGIC_SETUP_TIME );

    /* polarity pin */
    GPIO_writePin(N_DATA, polarity) ; /* set D input */

    DEVICE_DELAY_US( LOGIC_SETUP_TIME );

    /* disable latches */
    GPIO_writePin(N_LE_POL_0, 1);
    GPIO_writePin(N_LE_POL_1, 1);

    DEVICE_DELAY_US( LOGIC_SETUP_TIME );
    /* enable the outputs */
    GPIO_writePin(N_OE_POL, 0);

    DEVICE_DELAY_US( LOGIC_SETUP_TIME );
}

/* sets the address line [GCMD0..GCMD2]
 *
 * three address lines -> eight addresses
 *
 * parameter
 *      address - the cell number that is about to be connected
 *
 * returns
 *      none
 *
 * assumptions:
 *      none
 * */
static void switch_matrix_set_address(uint16_t address)
{
    /* filter out the wanted bits */
    address %= 8;

    /* clear address lines */
    GPIO_writePin(GCMD0, 0);
    GPIO_writePin(GCMD1, 0);
    GPIO_writePin(GCMD2, 0);

    /* set address lines */

    /* address line A0 */
    if(address & (1 << 0))
        GPIO_writePin(GCMD0, 1);

    /* address line A1 */
    if(address & (1 << 1))
        GPIO_writePin(GCMD1, 1);

    /* address line A2 */
    if(address & (1 << 2))
        GPIO_writePin(GCMD2, 1);

    DEVICE_DELAY_US( LOGIC_SETUP_TIME );
}

/* connects cell <cell_number> to CLLC
 *
 * connects CLLC to correct points in correct energy bank
 * energy bank low,  BAT_1..BAT_15
 * energy bank high, BAT_16..BAT_30
 *
 * BAT_0 is connected to BAT_GND
 * BAT15_N is connected to BAT_MidPWR and (logically) negative side of BAT16
 * BAT30 is connected to BAT_PWR
 *
 * BAT0..BAT15 is used for charging through one CLLC
 * BAT15_N..BAT30 is used for charging through the other CLLC
 *
 * parameter
 *      cell_number - number of the energy cell that is going to be connected
 *                    BAT1..BAT15 or BAT16..BAT30
 *                    (the positive side of the cell to connect)
 *
 * returns
 *      0 - out of range/illegal choice (BAT0, BAT15_N, >BAT30)
 *          resets the matrix as a security precaution
 *      1 - OK
 *
 * assumptions:
 *      always do switching using 'break before make'
 *      always start by resetting the switch (called inside this function)
 * */
int switch_matrix_connect_cell(uint16_t cell_number)
{
    int return_value = 1;   /* true */
    uint8_t address = (cell_number & 0x0f) >> 1;

    /* set switches for selecting the wanted cell to charge/discharge */
    if((cell_number > BAT_30) || (cell_number == BAT_0) || (cell_number == BAT_15_N)) {
        /* out of range */

        /* reset switch matrix
         * no cell is chosen
         * no polarity is chosen */
        switch_matrix_reset();

        return_value = 0;
    }
    else
    {
        /* break before make */
        //Set polarity circuit to high impedance to disconnect cell
        GPIO_writePin(N_OE_POL, 1);
        DEVICE_DELAY_US( 10 * LOGIC_SETUP_TIME );

        /* set address lines for selecting cell connecting nodes */

        /*****************************
         * cell first connecting node
         * ***************************/

        switch_matrix_set_address(address);
        DEVICE_DELAY_US( LOGIC_SETUP_TIME );

        /* latch enable to shift in the address */
        if(cell_number <= BAT_15)
        {   /* low cell group, BAT1..BAT15 */
            GPIO_writePin(GCMD3, 0);
        } else
        {   /* high cell group, BAT16..BAT30 */
            GPIO_writePin(GCMD4, 0);
        }
        DEVICE_DELAY_US( LOGIC_SETUP_TIME );


        /* latch disable */
        if(cell_number <= BAT_15)
        {   /* low cell group, BAT1..BAT15 */
            GPIO_writePin(GCMD3, 1);
        } else
        {   /* high cell group, BAT16..BAT30 */
            GPIO_writePin(GCMD4, 1);
        }
        DEVICE_DELAY_US( LOGIC_SETUP_TIME );

        /*****************************
         * cell second connecting node
         * ***************************/

        if(0 == (cell_number & 1))
        {   /* for even cell numbers */
            if(address > 0)
                address--;
        }

        switch_matrix_set_address(address);
        DEVICE_DELAY_US( LOGIC_SETUP_TIME );

        /* latch enable to shift in the address */
        if(cell_number <= BAT_15)
        {   /* low cell group, BAT1..BAT15 */
            GPIO_writePin(GCMD5, 0);
        } else
        {   /* high cell group, BAT16..BAT30 */
            GPIO_writePin(GCMD6, 0);
        }
        DEVICE_DELAY_US( LOGIC_SETUP_TIME );



        /* latch disable */
        if(cell_number <= BAT_15)
        {   /* low cell group, BAT1..BAT15 */
            GPIO_writePin(GCMD5, 1);
        } else
        {   /* high cell group, BAT16..BAT30 */
            GPIO_writePin(GCMD6, 1);
        }
        DEVICE_DELAY_US( LOGIC_SETUP_TIME );

        /*****************************
         * activate outputs
         * ***************************/
        if (cell_number <= BAT_15)
        {
            GPIO_writePin(GCMD7, 0);
        } else
        {
            GPIO_writePin(GCMD8, 0);
        }
        DEVICE_DELAY_US( LOGIC_SETUP_TIME );

    }

    return return_value;
}


