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

energy_bank_condition_t energy_bank_condition;
energy_bank_condition_t savedStateOfCharge = { .initializedSoCIndicator = 0xFF };

bool newSoCAvailable;

float cellVoltagesVector[30];

void initCalcStateOfCharge( void ) {
    energy_bank_condition.inititalVoltage = DCDC_VI.avgVStore;
    energy_bank_condition.last_timer = 0;
    energy_bank_condition.totalChargeTime = 0;
}

void calcAccumlatedCharge( void ) {

    static float last_measured_current, instant_current ;
    uint32_t current_timer, elapsed_time;

    if( energy_bank_condition.last_timer == 0) {
        energy_bank_condition.last_timer = timer_get_ticks();
        last_measured_current = fabsf( sensorVector[ISen2fIdx].realValue );
        energy_bank_condition.accumulatedCharge = 0;
        return;
    }

    current_timer = timer_get_ticks();
    elapsed_time = current_timer - energy_bank_condition.last_timer;

    if( elapsed_time >= 1000 ) {
        instant_current =  fabsf( sensorVector[ISen2fIdx].realValue );
        energy_bank_condition.accumulatedCharge = energy_bank_condition.accumulatedCharge + ( ( instant_current + last_measured_current ) / 2 );
        energy_bank_condition.totalChargeTime = energy_bank_condition.totalChargeTime + ( elapsed_time / 1000 );
        last_measured_current = instant_current ;
    }
}

void finallyCalcStateOfCharge() {

    float newCapacitance = 0.0;

    energy_bank_condition.finalVoltage = DCDC_VI.avgVStore;

    newCapacitance = energy_bank_condition.accumulatedCharge / (energy_bank_condition.finalVoltage - energy_bank_condition.inititalVoltage);

    retriveStateOfChargeFromFlash();

    if( energy_bank_condition.initializedSoCIndicator == 0xFF ) {

        energy_bank_condition.stateOfHealthPercent= 100.0;
        energy_bank_condition.initialCapacitance = newCapacitance;
        energy_bank_condition.capacitance = newCapacitance;

    } else {

        energy_bank_condition.stateOfHealthPercent = 100.0 * ( newCapacitance / energy_bank_condition.initialCapacitance );
        energy_bank_condition.capacitance = newCapacitance;

    }

    if( sharedVars_cpu1toCpu2.max_voltage_applied_to_energy_bank >= 0.0 ) {
        energy_bank_condition.stateOfChargePercent =100.0 *
            powf( ( energy_bank_condition.finalVoltage /
                    ( sharedVars_cpu1toCpu2.max_voltage_applied_to_energy_bank * MAX_ENERGY_VOLTAGE_RATIO )
                  ), 2 );
    } else {

        energy_bank_condition.stateOfChargePercent = 0.0;

    }

    energy_bank_condition.initializedSoCIndicator = 0xAA;
    saveStatOfChargeToFlash();
    newSoCAvailable = true;

}


void saveStatOfChargeToFlash(void) {
    memcpy( &savedStateOfCharge, &energy_bank_condition, sizeof(energy_bank_condition_t) );

}


void retriveStateOfChargeFromFlash(void) {
    memcpy( &energy_bank_condition, &savedStateOfCharge, sizeof(energy_bank_condition_t) );
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

    /* set safety threshold for state of charge */
    energy_bank_settings.safety_threshold_state_of_charge = sharedVars_cpu1toCpu2.safety_threshold_state_of_charge;

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

void energy_storage_check(void) {
    static uint32_t last_time = 0;
    static uint16_t cellCount = 0;

    /* several of possible checks for this function is handled in CPU1 in
     * checks_CPU2() in check_cpu2.c
     */

    sharedVars_cpu2toCpu1.soc_energy_cell[cellCount] = cellVoltagesVector[cellCount];
    cellCount++;
    if(cellCount == NUMBER_OF_CELLS) {
        cellCount=0;
    }



    sharedVars_cpu2toCpu1.soc_energy_bank = energy_bank_condition.stateOfChargePercent;

    if( timer_get_ticks() - last_time >= 5000 ) {
        last_time = timer_get_ticks();
        PRINT( "stateOfChargePercent:[%8.2f]%%\r\n", energy_bank_condition.stateOfChargePercent );
    }

    retriveStateOfChargeFromFlash();

    if( newSoCAvailable == true ) {

        sharedVars_cpu2toCpu1.soh_energy_bank = energy_bank_condition.stateOfHealthPercent;
        PRINT( "stateOfHealthPercent:[%8.2f]%%\r\n", energy_bank_condition.stateOfHealthPercent );

        newSoCAvailable = false;

    }

    if( energy_bank_condition.initializedSoCIndicator == 0xFF ) {

        sharedVars_cpu2toCpu1.soh_energy_bank = 0.0;
        sharedVars_cpu2toCpu1.remaining_energy_to_min_soc_energy_bank = 0.0;

    } else {

        sharedVars_cpu2toCpu1.remaining_energy_to_min_soc_energy_bank = 0.5 * ( energy_bank_condition.capacitance *
                ( powf( DCDC_VI.avgVStore, 2 ) - powf( sharedVars_cpu1toCpu2.min_voltage_applied_to_energy_bank , 2 ) ) );

    }

}




