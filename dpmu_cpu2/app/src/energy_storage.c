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

States_t sohState = { .State_Next = sohCalcWait, .State_Current = sohCalcWait, .State_Before = sohCalcWait, .State_Before_Balancing = 0 };

bool chargingFlag = false;
float avgChargingCurrent;
float currentCapacitance;
float accumulatedCharge;
float finalVoltage;
float initialVoltage;
uint16_t avgChargingCurrentCount;
uint32_t totalChargeTime;

float instant_current ;
volatile uint32_t soh_last_timer;
volatile uint32_t soh_current_timer;
volatile uint32_t soh_elapsed_time;

bool startSaveCapacitance = false;


bool newEnergyBankConditionAvailable;

float cellVoltagesVector[30];



void calcAccumlatedCharge( void ) {

    soh_current_timer = timer_get_ticks();
    soh_elapsed_time = soh_current_timer - soh_last_timer;

    if( soh_elapsed_time >= 100 ) {
        instant_current =  fabsf( sensorVector[ISen2fIdx].realValue );
        avgChargingCurrent = avgChargingCurrent + instant_current;
        avgChargingCurrentCount = avgChargingCurrentCount + 1;
        totalChargeTime = totalChargeTime + soh_elapsed_time;
        soh_last_timer = soh_current_timer;
    }
    if( avgChargingCurrentCount == NUMBER_OF_AVG_CHARGING_CURRENT_COUNT) {
        avgChargingCurrent = avgChargingCurrent / (float)NUMBER_OF_AVG_CHARGING_CURRENT_COUNT;
        accumulatedCharge = accumulatedCharge + avgChargingCurrent;
        PRINT("\r\n===> AccumulatedCharge:[%8.2f], TotalChargeTime:[%lu]ms,  AvgChargingCurrent:[%8.2f]\r\n\r\n",
              accumulatedCharge,
              totalChargeTime,
              avgChargingCurrent );
        avgChargingCurrent = 0.0;
        avgChargingCurrentCount = 0;
    }

}


bool requestCPU1ToSaveCapacitancesToFlash(float  initialCapacitance, float currentCapacitance ) {
    static uint8_t currState = 0;
    static uint8_t nextState = 0;
    bool retVal = false;

    switch( currState ) {
        case 0:
            if( startSaveCapacitance == true ) {
                nextState = 1;
            }
            break;
        case 1:
            sharedVars_cpu2toCpu1.initialCapacitance = initialCapacitance;
            sharedVars_cpu2toCpu1.currentCapacitance = currentCapacitance;
            sharedVars_cpu2toCpu1.newCapacitanceAvailable = true;
            nextState = 2;
            break;
        case 2:
            if( sharedVars_cpu1toCpu2.newCapacitanceSaved == true ) {
                sharedVars_cpu2toCpu1.newCapacitanceAvailable = false;
                startSaveCapacitance = false;
                retVal = true;
                nextState = 0;
            }
            break;
        default:
            break;
    }
    currState = nextState;
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
    static uint16_t cellCount = 0;

    sharedVars_cpu2toCpu1.soc_energy_cell[cellCount] = cellVoltagesVector[cellCount];
    cellCount++;
    if(cellCount == NUMBER_OF_CELLS) {
        cellCount=0;
    }

    if( sharedVars_cpu1toCpu2.max_voltage_applied_to_energy_bank > 0.0 ) {
        sharedVars_cpu2toCpu1.soc_energy_bank = 100.0 * powf( ( DCDC_VI.avgVStore /
                    ( sharedVars_cpu1toCpu2.max_voltage_applied_to_energy_bank * MAX_ENERGY_BANK_VOLTAGE_RATIO ) ), 2 );
    } else {

        sharedVars_cpu2toCpu1.soc_energy_bank = 0.0;
    }

    if( sharedVars_cpu1toCpu2.initialCapacitance > 0.0 ) {
        sharedVars_cpu2toCpu1.soh_energy_bank = 100.0 * ( sharedVars_cpu1toCpu2.currentCapacitance / sharedVars_cpu1toCpu2.initialCapacitance );
    } else {
        sharedVars_cpu2toCpu1.soh_energy_bank = 0.0;
    }

    if( ( sharedVars_cpu1toCpu2.min_voltage_applied_to_energy_bank > 0.0 ) &&
        ( DCDC_VI.avgVStore > sharedVars_cpu1toCpu2.min_voltage_applied_to_energy_bank ) ) {

        sharedVars_cpu2toCpu1.remaining_energy_to_min_soc_energy_bank = 0.5 * ( sharedVars_cpu1toCpu2.currentCapacitance *
                ( powf( DCDC_VI.avgVStore, 2 ) - powf( sharedVars_cpu1toCpu2.min_voltage_applied_to_energy_bank , 2 ) ) );

    } else {

        sharedVars_cpu2toCpu1.remaining_energy_to_min_soc_energy_bank = 0.0;
    }


    switch( sohState.State_Current ) {

        case sohCalcWait:

            if( StateVector.State_Current == Charge ) {
                sohState.State_Next = sohCalcInit;
            }

            break;

        case sohCalcInit:


            initialVoltage = DCDC_VI.avgVStore;
            accumulatedCharge = 0.0;
            avgChargingCurrent = 0.0;
            avgChargingCurrentCount = 0;
            totalChargeTime = 0.0;
            soh_last_timer = timer_get_ticks();
            soh_current_timer = soh_last_timer;
            soh_elapsed_time = 0;
            sohState.State_Next = sohCalcCapacitance;
            break;

        case sohCalcCapacitance:
            if( StateVector.State_Current != Charge ) {
                finalVoltage = DCDC_VI.avgVStore;
                sohState.State_Next = sohCalcEnd;
            } else {
                calcAccumlatedCharge();
            }
            break;

        case sohCalcEnd:

            if( totalChargeTime < MINIMUM_CHARGING_TIME_IN_MILISECS ) {
                PRINT("Charging time lower then 60s\r\n");
                sohState.State_Next = sohCalcWait;
            } else {
                currentCapacitance = accumulatedCharge / (finalVoltage - initialVoltage);
                sohState.State_Next = verifySoHFromFlash;
            }
            break;

        case verifySoHFromFlash:
            if( sharedVars_cpu1toCpu2.initialCapacitance == 0.0 ) {
                PRINT("SOH FIRST CALC OF INITIAL CAPACITANCE \r\n");

                sharedVars_cpu2toCpu1.soh_energy_bank = 100.0;

                PRINT("sharedVars_cpu2toCpu1.soh_energy_bank [%8.2f]\r\n",sharedVars_cpu2toCpu1.soh_energy_bank);
                startSaveCapacitance = true;
                sohState.State_Next = saveNewEnergyConditionToFlash;

            } else {

                PRINT("RETRIVED SOH from flash initialCapacitance:[%8.2f]\r\n",
                      sharedVars_cpu1toCpu2.initialCapacitance );

                if( currentCapacitance >= sharedVars_cpu1toCpu2.initialCapacitance ) {
                    currentCapacitance = sharedVars_cpu1toCpu2.initialCapacitance;
                }

                sharedVars_cpu2toCpu1.soh_energy_bank = 100.0 * ( currentCapacitance / sharedVars_cpu1toCpu2.initialCapacitance );
                startSaveCapacitance = true;
                sohState.State_Next = saveCurrentEnergyConditionToFlash;
            }
            break;

        case saveNewEnergyConditionToFlash:
            // Initial Capacitance is the current capacitance When saving for the first time
            if( requestCPU1ToSaveCapacitancesToFlash( currentCapacitance, currentCapacitance ) == true ) {
                sohState.State_Next = sohCalcWait;
                PRINT( "initialCapacitance saved to flash:[%8.2f]%%\r\n",  currentCapacitance);
            }
            break;

        case saveCurrentEnergyConditionToFlash:
            if( requestCPU1ToSaveCapacitancesToFlash( sharedVars_cpu1toCpu2.initialCapacitance, currentCapacitance ) == true ) {
                sohState.State_Next = sohCalcWait;
                PRINT( "initialCapacitance saved to flash:[%8.2f]%%\r\n",  currentCapacitance);
            }
            break;

        default:
            sohState.State_Next = sohCalcWait;
            break;
    }
    if(  sohState.State_Current != sohState.State_Next ) {
        PRINT("sohState.State_Next:[%d] sohState.State_Current:[%d]\r\n", sohState.State_Next, sohState.State_Current);
    }
    sohState.State_Current = sohState.State_Next;

}




