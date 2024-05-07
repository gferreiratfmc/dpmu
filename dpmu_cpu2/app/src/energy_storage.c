/*
 * energy_storage.c
 *
 *  Created on: 25 okt. 2023
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#include <error_handling.h>
#include <stdbool.h>
#include <math.h>

#include "cli_cpu2.h"
#include "common.h"
#include "energy_storage.h"
#include "shared_variables.h"
#include "sensors.h"
#include "timer.h"



energy_bank_t energy_bank_settings = {0};

state_of_charge_t stateOfCharge;

float cellVoltagesVector[30];


void initCalcStateOfCharge( void ) {
    stateOfCharge.inititalVoltage = DCDC_VI.avgVStore;
    stateOfCharge.last_timer = 0;
    stateOfCharge.totalChargeTime = 0;
}

void calcAccumlatedCharge( void ) {

    static float last_measured_current, instant_current ;
    uint32_t current_timer, elapsed_time;

    if( stateOfCharge.last_timer == 0) {
        stateOfCharge.last_timer = timer_get_ticks();
        last_measured_current = fabsf( sensorVector[ISen2fIdx].realValue );
        stateOfCharge.accumulatedCharge = 0;
        return;
    }

    current_timer = timer_get_ticks();
    elapsed_time = current_timer - stateOfCharge.last_timer;

    if( elapsed_time >= 1000 ) {
        instant_current =  fabsf( sensorVector[ISen2fIdx].realValue );
        stateOfCharge.accumulatedCharge = stateOfCharge.accumulatedCharge + ( ( instant_current + last_measured_current ) / 2 );
        stateOfCharge.totalChargeTime = stateOfCharge.totalChargeTime + ( elapsed_time / 1000 );
        last_measured_current = instant_current ;
    }
}

void finallyCalcStateOfCharge() {
    stateOfCharge.finalVoltage = DCDC_VI.avgVStore;
    stateOfCharge.totalCapacitance = stateOfCharge.accumulatedCharge / (stateOfCharge.finalVoltage - stateOfCharge.inititalVoltage);

}


void energy_storage_update_settings(void)
{
    /* set max Voltage applied to energy bank */
    energy_bank_settings.max_voltage_applied_to_energy_bank = sharedVars_cpu1toCpu2.max_voltage_applied_to_energy_bank;
    DCDC_VI.target_Voltage_At_VStore = (float) sharedVars_cpu1toCpu2.max_voltage_applied_to_energy_bank;

    /* set CV, constant Voltage, threshold */
    energy_bank_settings.constant_voltage_threshold = sharedVars_cpu1toCpu2.constant_voltage_threshold;

    /* set min state of charge of energy bank */
    energy_bank_settings.min_voltage_applied_to_energy_bank = sharedVars_cpu1toCpu2.min_voltage_applied_to_energy_bank;

    /* set CC, constant current, preconditional threshold */
    energy_bank_settings.preconditional_threshold = sharedVars_cpu1toCpu2.preconditional_threshold;

    /*TODO MinimumBoostVoltageSuperCaps in GlobalV.h should be calculated from set min state of charge of energy bank
     *     Remove from GlobalV.h!!
     *
     *  HB I do not fully agree.
     *              There is a minimum Voltage that we can boost to 180V.
     *              There might be a minimum Voltage for the Lithium batteries.
     *              Both of these are unrelated to energy levels.
     *
     *     Specification, after discussion, have been changed to Volt as unit
     *     for min state of charge of energy bank, also changed name to
     *     min_voltage_applied_to_energy_bank.
     *
     *     MinimumBoostVoltageSuperCaps have been removed from GlobalV.h.
     */

    /* set safety threshold for state of charge */
    energy_bank_settings.safety_threshold_state_of_charge = sharedVars_cpu1toCpu2.safety_threshold_state_of_charge;

    /*TODO FulllyChargedVoltageLevel in GlobalV.h is min Voltage of energy cell
     *     Remove from GlobalV.h!!
     *
     *  HB I do not understand the first line in this todo. How can fully be
     *  equal to min. Removed from GlobalV.h.
     */

    /* set max Voltage on storage cell */
    if( sharedVars_cpu1toCpu2.max_allowed_voltage_energy_cell <= 3.0 ) {
        energy_bank_settings.max_allowed_voltage_energy_cell = sharedVars_cpu1toCpu2.max_allowed_voltage_energy_cell;
    } else {
        energy_bank_settings.max_allowed_voltage_energy_cell = 3.0;;
    }


    /* set min Voltage of energy cell */
    energy_bank_settings.min_allowed_voltage_energy_cell = sharedVars_cpu1toCpu2.min_allowed_voltage_energy_cell;

    /* set max charge current of energy cells */
    energy_bank_settings.ESS_Current = sharedVars_cpu1toCpu2.ess_current;
}

void energy_storage_check(void)
{
    static uint16_t cellCount = 0;

    /* several of possible checks for this function is handled in CPU1 in
     * checks_CPU2() in check_cpu2.c
     */

    sharedVars_cpu2toCpu1.soc_energy_cell[cellCount] = cellVoltagesVector[cellCount];
    cellCount++;
    if(cellCount == NUMBER_OF_CELLS) {
        cellCount=0;
    }

    if( stateOfCharge.newSoCAvailable == true ) {
        PRINT( "Last State Of Charge:[%8.2f]F\r\n", stateOfCharge.totalCapacitance );
        stateOfCharge.newSoCAvailable = false;
    }

}




