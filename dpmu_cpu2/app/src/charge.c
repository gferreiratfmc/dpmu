/*
 * charge.c
 *
 *  Created on: 15 nov. 2022
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#include "stdbool.h"
#include "stdint.h"

#include "switches.h"


#define Min_Allowed_State_of_Charge_of_Energy_Bank 30   /* TODO change to variable updated inenergy_storage_update_settings */
#define Max_Allowed_State_of_Charge_of_Energy_Bank 90   /* TODO change to variable updated inenergy_storage_update_settings */
#define Min_Voltage_Energy_Cell 1                       /* TODO change to variable updated inenergy_storage_update_settings */

#define NR_OF_ENERGY_BANKS 2
#define NR_OF_ENERGY_CELLS_PER_BANK 15

uint8_t charge_measure_SoC_energy_bank(){return 100;}

uint8_t charge_measure_SoC_energy_cell(uint8_t i, uint8_t j)
{
    /* measure the voltage of the cell
     * return the measurement
     *  */

    return 1;
}

bool charge_measure_SoC_energy_cells(uint8_t voltage_per_energy_cell)
{
    bool return_value = true;

    /* if one cell has not reached its predefined value (if good cell), return false */
    for(int i = 0; i < NR_OF_ENERGY_BANKS; i++) {
        for(int j = 0; j < NR_OF_ENERGY_CELLS_PER_BANK; j++) {
            if(charge_measure_SoC_energy_cell(i, j) < voltage_per_energy_cell) {
                return_value = false;
                break;
            }
        }
    }

    return return_value;
}

uint8_t charge_measure_V_DC_bus(){return 1;}
bool    charge_slave_DPMU_reached_its_DCbus_value(){return 1;}           /* yes/{no,no_aswer} from slave DPMU */
bool    charge_slave_DPMU_activate_sharing(){return 1;}                  /* yes/{no,no_aswer} from slave DPMU */
//uint8_t charge_wait_for_change_of_state_command_from_IOP(){return 1;}    /* return to got o some kind of IDLE state T.B.D. */
uint8_t charge_signal_IOP_sharing_is_activated(){return 1;}              /* tell IOP sharing is activated - T.B.D. if needed*/

static bool    signal_sharing_dpmu_start_charging(void)
{
    return true;
};  /* tell slave (sharing) DPMU to start charging */

static uint8_t balance_energy_storange_bank(void)
{
    return 1;
};

static bool energy_bank_precharged(void)
{
    bool    return_value;
    uint8_t state_of_charge_energy_bank  = charge_measure_SoC_energy_bank();
    bool    state_of_charge_energy_cells = charge_measure_SoC_energy_cells(Min_Voltage_Energy_Cell);

    /* TODO - Is this enough? What about faulty cells and degraded cells? */
    return_value  = state_of_charge_energy_bank  > Min_Allowed_State_of_Charge_of_Energy_Bank;
    return_value |= state_of_charge_energy_cells;

    return_value = 1; /* to lock in while(1) before this is implemented */
    return return_value;
}

static uint8_t precharge_energy_storange_bank(void)
{
    return 1;
};

static uint8_t charge_energy_storange_bank(void)
{
    return 1;
};

static bool boost_mode(bool activate_boost_mode)
{
    return 1;
};         /* could/could_not activate boost mode */

#define margin 0.05 /* margin is to be 5% */    /* TODO - Is this needed? T.B.D. */
static bool energy_bank_charged(void)
{
    /* TODO - Is this enough? What about faulty cells and degraded cells? */
    return (charge_measure_SoC_energy_bank() >= (Max_Allowed_State_of_Charge_of_Energy_Bank * (1 - margin)));
}

void charge(void) {

    uint8_t sharing_charge_initiated = 0;   /* is sharing initiated */
    uint8_t sharing = 0;                    /* TODO - Read from xRAM, written by CPU1/CANA or CPU2/CANB */
    uint8_t Voltage_at_DC_Bus = 180;        /* TODO - Read from xRAM, written by CPU1/CANA or CPU2/CANB - nominal value for DC bus */

    /* do another DPMU need to charge in parallel? */
    if(sharing) {
        /* deactivate sharing bus */
        switches_Qsb(SW_OFF);

        /* signal other DPMU to start charging */
        sharing_charge_initiated = signal_sharing_dpmu_start_charging();    /* yes/{no,no_aswer} from slave DPMU */

        if(!sharing_charge_initiated) {
            ;   /* TODO - Error state */
        }
    }

    /* balance energy bank */
    balance_energy_storange_bank();

    /* reach minimum state of charge */
    while(!energy_bank_precharged()) {
        precharge_energy_storange_bank();
        balance_energy_storange_bank();
    }

    /*** energy bank reached (Min_Voltage_Energy_Cell & Min_Allowed_State_of_Charge_of_Energy_Bank) ***/

    /* balance energy bank */
    balance_energy_storange_bank();

    /* check state of charge */
    while(!energy_bank_charged()){
        charge_energy_storange_bank();
        balance_energy_storange_bank();
    }

    /*** energy bank reached Max_Allowed_State_of_Charge_of_Energy_Bank ***/

    /* start generating 'Voltage_at_DC_Bus' (nominal 180V DC) */
    if(!boost_mode(true)) {
        ;   /* TODO - Error state */
    }

    /* wait for voltage on 180V DC line to reach its nominal value */
    while(charge_measure_V_DC_bus() < Voltage_at_DC_Bus);

    /*** DC bus reached 'Voltage_at_DC_Bus' ***/

    if(sharing_charge_initiated) {
        while(!charge_slave_DPMU_reached_its_DCbus_value());

        /* signal to slave DPMU to turn activate sharing bus */
        while(!charge_slave_DPMU_activate_sharing());

        /* activate sharing bus */
        switches_Qsb(SW_ON);

        charge_signal_IOP_sharing_is_activated();
    }

//    charge_wait_for_change_of_state_command_from_IOP();


}
