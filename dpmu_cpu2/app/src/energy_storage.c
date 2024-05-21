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
#include "state_machine.h"
#include "timer.h"



energy_bank_t energy_bank_settings = {0};

#pragma DATA_SECTION(ipc_soh_msg, "MSGRAM_CPU2_TO_CPU1")
char ipc_soh_msg[MAX_CPU2_DBG_LEN];

energy_bank_condition_t energy_bank_condition = { 0 };
energy_bank_condition_t new_energy_bank_condition = { 0 };
energy_bank_condition_t savedStateOfCharge = { .initializedSoCIndicator = 0xAA, .capacitance = 15, .initialCapacitance = 15, .stateOfHealthPercent=100.0 };

States_t sohState = { .State_Next = sohCalcWait, .State_Current = sohCalcWait };

uint32_t last_timer;
bool chargingFlag = false;
float avgChargingCurrent;
uint16_t avgChargingCurrentCount;

bool newEnergyBankConditionAvailable;

float cellVoltagesVector[30];

void startCalcStateOfCharge( void ) {
    new_energy_bank_condition.inititalVoltage = DCDC_VI.avgVStore;
    new_energy_bank_condition.last_timer = 0;
    new_energy_bank_condition.totalChargeTime = 0;
    last_timer = 0;
    avgChargingCurrent = 0.0;
    avgChargingCurrentCount = 0;
}

void calcAccumlatedCharge( void ) {

    float instant_current ;
    uint32_t current_timer, elapsed_time;


    current_timer = timer_get_ticks();
    elapsed_time = current_timer - energy_bank_condition.last_timer;

    if( elapsed_time >= 100 ) {
        instant_current =  fabsf( sensorVector[ISen2fIdx].realValue );
        avgChargingCurrent = avgChargingCurrent + instant_current;
        avgChargingCurrentCount++;
        new_energy_bank_condition.totalChargeTime = new_energy_bank_condition.totalChargeTime + ( elapsed_time / 1000 );
        energy_bank_condition.last_timer = current_timer;

    }

    if( avgChargingCurrentCount == NUMBER_OF_AVG_CHARGING_CURRENT_COUNT) {
        new_energy_bank_condition.accumulatedCharge = new_energy_bank_condition.accumulatedCharge + ( ( avgChargingCurrent ) / NUMBER_OF_AVG_CHARGING_CURRENT_COUNT );
        PRINT("\r\n===> accumulatedCharge:[%8.2f], totalChargeTime:[%8.2f],  avgChargingCurrent:[%8.2f]\r\n\r\n",
              new_energy_bank_condition.accumulatedCharge,
              new_energy_bank_condition.totalChargeTime,
              avgChargingCurrent );
        avgChargingCurrent = 0.0;
        avgChargingCurrentCount = 0;
    }
}


//void calcAccumlatedCharge( void ) {
//
//    static float last_measured_current, instant_current ;
//    uint32_t current_timer, elapsed_time;
//
//    if( last_timer == 0) {
//        last_timer = timer_get_ticks();
//        last_measured_current = fabsf( sensorVector[ISen2fIdx].realValue );
//        new_energy_bank_condition.accumulatedCharge = 0;
//        return;
//    }
//
//    current_timer = timer_get_ticks();
//    elapsed_time = current_timer - last_timer;
//
//    if( elapsed_time >= 1000 ) {
//        instant_current =  fabsf( sensorVector[ISen2fIdx].realValue );
//        new_energy_bank_condition.accumulatedCharge = new_energy_bank_condition.accumulatedCharge + ( ( instant_current + last_measured_current ) / 2 );
//        new_energy_bank_condition.totalChargeTime = new_energy_bank_condition.totalChargeTime + ( elapsed_time / 1000 );
//        last_measured_current = instant_current ;
//        PRINT("\r\n===> accumulatedCharge:[%8.2f], totalChargeTime:[%8.2f], last_measured_current:[%8.2f], instant_current:[%8.2f], elapsed_time:[%lu]\r\n\r\n",
//              new_energy_bank_condition.accumulatedCharge,
//              new_energy_bank_condition.totalChargeTime,
//              last_measured_current,
//              instant_current,
//              elapsed_time);
//        last_timer = current_timer;
//    }
//}

void finallyCalcStateOfCharge() {

    float newCapacitance = 0.0;

    new_energy_bank_condition.finalVoltage = DCDC_VI.avgVStore;

    if( new_energy_bank_condition.totalChargeTime < MINIMUM_CHARGING_TIME_IN_SECS ) {
        PRINT("Charging time lower then 60s\r\n");
        return;
    }

    newCapacitance = new_energy_bank_condition.accumulatedCharge / (new_energy_bank_condition.finalVoltage - new_energy_bank_condition.inititalVoltage);

    PRINT("newCapacitance:[%8.2f]\r\n", newCapacitance);
    retriveStateOfChargeFromFlash();

    if( energy_bank_condition.initializedSoCIndicator == 0xFF ) {

        new_energy_bank_condition.stateOfHealthPercent= 100.0;
        new_energy_bank_condition.initialCapacitance = newCapacitance;
        new_energy_bank_condition.capacitance = newCapacitance;
        new_energy_bank_condition.initializedSoCIndicator = 0xAA;

        saveStateOfChargeToFlash(&new_energy_bank_condition);

    } else {

        if( newCapacitance >= energy_bank_condition.initialCapacitance ) {
            newCapacitance = energy_bank_condition.initialCapacitance;
        }
        energy_bank_condition.stateOfHealthPercent = 100.0 * ( newCapacitance / energy_bank_condition.initialCapacitance );
        energy_bank_condition.capacitance = newCapacitance;
        saveStateOfChargeToFlash(&energy_bank_condition);
    }

    newEnergyBankConditionAvailable = true;

}


void saveStateOfChargeToFlash(energy_bank_condition_t  *p_energy_bank_condition) {
    memcpy( &savedStateOfCharge, p_energy_bank_condition, sizeof(energy_bank_condition_t) );
    PRINT("CPU2 saveStateOfChargeToFlash\r\n");
    PRINT("CPU2 energy_bank_condition.stateOfHealthPercent[%8.2f]\r\n", p_energy_bank_condition->stateOfHealthPercent);
    PRINT("CPU2 new_energy_bank_condition.initialCapacitance[%8.2f]]\r\n", p_energy_bank_condition->initialCapacitance );
    PRINT("CPU2 new_energy_bank_condition.capacitance[%8.2f]]\r\n",p_energy_bank_condition->capacitance);
    PRINT("CPU2 new_energy_bank_condition.initializedSoCIndicator[%d]\r\n ",p_energy_bank_condition->initializedSoCIndicator);

    memcpy(ipc_soh_msg, p_energy_bank_condition, sizeof(energy_bank_condition_t) );

    PRINT("CPU2 add:[%08lX] data:[%lu]\r\n", (uint32_t)&ipc_soh_msg, sizeof(energy_bank_condition_t));

    IPC_sendCommand(IPC_CPU2_L_CPU1_R, IPC_FLAG_MESSAGE_CPU2_TO_CPU1,
                                false,
                                IPC_REQUEST_WRITE_SOH_TO_EXT_FLASH,
                                (uint32_t)&ipc_soh_msg, sizeof(energy_bank_condition_t) );


}


bool retriveStateOfChargeFromFlash(void) {

    static uint8_t state = 0;
    uint8_t nextState = 0;
    bool retVal = false;

//    memcpy( &energy_bank_condition, &savedStateOfCharge, sizeof(energy_bank_condition_t) );

    switch( state ) {
        case 0:
            IPC_sendCommand(IPC_CPU2_L_CPU1_R, IPC_FLAG_MESSAGE_CPU2_TO_CPU1,
                                    false,
                                    IPC_REQUEST_READ_SOH_FROM_EXT_FLASH,
                                    (uint32_t)&energy_bank_condition_from_flash, sizeof(energy_bank_condition_t) );
            nextState = 1;
            break;
        case 1:
            if( !IPC_isFlagBusyLtoR(IPC_CPU2_L_CPU1_R, IPC_FLAG_MESSAGE_CPU2_TO_CPU1) ) {
                memcpy( &energy_bank_condition, &energy_bank_condition_from_flash, sizeof(energy_bank_condition_t) );
                retVal = true;
                nextState = 0;
            }
            break;
        default:
            break;
    }
    state = nextState;
    return retVal;
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

    if( sharedVars_cpu1toCpu2.max_voltage_applied_to_energy_bank > 0.0 ) {
        energy_bank_condition.stateOfChargePercent = 100.0 * powf( ( DCDC_VI.avgVStore /
                    ( sharedVars_cpu1toCpu2.max_voltage_applied_to_energy_bank * MAX_ENERGY_VOLTAGE_RATIO ) ), 2 );
    } else {

        energy_bank_condition.stateOfChargePercent = 0.0;

    }

    sharedVars_cpu2toCpu1.soc_energy_bank = energy_bank_condition.stateOfChargePercent;

    if( timer_get_ticks() - last_time >= 7500 ) {
        last_time = timer_get_ticks();
        PRINT( "stateOfChargePercent:[%8.2f]%% \r\n", energy_bank_condition.stateOfChargePercent );
        PRINT( "remaining_energy_to_min_soc_energy_bank:[%8.2f]J\r\n", sharedVars_cpu2toCpu1.remaining_energy_to_min_soc_energy_bank );
        PRINT( "energy_bank_condition.capacitance:[%8.2f]F\r\n", energy_bank_condition.capacitance);
    }

    switch( sohState.State_Current ) {

        case sohCalcWait:

            if( StateVector.State_Current == Charge ) {
                sohState.State_Next = sohCalcInit;
            }
            break;

        case sohCalcInit:

            startCalcStateOfCharge();
            sohState.State_Next = sohCalcCapacitance;
            break;

        case sohCalcCapacitance:
            if( StateVector.State_Current == ChargeStop ) {
                sohState.State_Next = sohCalcEnd;
            } else {
                calcAccumlatedCharge();
            }
            break;

        case sohCalcEnd:
            finallyCalcStateOfCharge();
            sohState.State_Next = sohCalcWait;
            break;

        default:
            sohState.State_Next = sohCalcWait;
    }
    if(  sohState.State_Current != sohState.State_Next ) {
        PRINT("sohState.State_Next:[%d] sohState.State_Current:[%d]\r\n", sohState.State_Next, sohState.State_Current);
    }
    sohState.State_Current = sohState.State_Next;

    if( newEnergyBankConditionAvailable == true ) {

        retriveStateOfChargeFromFlash();

        sharedVars_cpu2toCpu1.soh_energy_bank = energy_bank_condition.stateOfHealthPercent;

        PRINT( "stateOfHealthPercent:[%8.2f]%%\r\n", energy_bank_condition.stateOfHealthPercent );

        newEnergyBankConditionAvailable = false;

    }

    if( energy_bank_condition.initializedSoCIndicator == 0xFF ) {

        sharedVars_cpu2toCpu1.soh_energy_bank = 0.0;
        sharedVars_cpu2toCpu1.remaining_energy_to_min_soc_energy_bank = 0.0;

    } else {

        sharedVars_cpu2toCpu1.remaining_energy_to_min_soc_energy_bank = 0.5 * ( energy_bank_condition.capacitance *
                ( powf( DCDC_VI.avgVStore, 2 ) - powf( sharedVars_cpu1toCpu2.min_voltage_applied_to_energy_bank , 2 ) ) );

    }

}




