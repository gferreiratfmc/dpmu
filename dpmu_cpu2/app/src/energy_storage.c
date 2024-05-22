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



energy_bank_condition_t energy_bank_condition = { 0 };
energy_bank_condition_t new_energy_bank_condition = { 0 };

States_t sohState = { .State_Next = sohCalcWait, .State_Current = sohCalcWait };

uint32_t last_timer;
bool chargingFlag = false;
float avgChargingCurrent;
float newCapacitance;
float accumulatedCharge;
float finalVoltage;
float initialVoltage;
uint16_t avgChargingCurrentCount;
float totalChargeTime;

float instant_current ;
volatile uint32_t current_timer;
volatile uint32_t elapsed_time;

bool newEnergyBankConditionAvailable;

float cellVoltagesVector[30];

void startCalcStateOfCharge( void ) {
    initialVoltage = DCDC_VI.avgVStore;
    accumulatedCharge = 0.0;
    avgChargingCurrent = 0.0;
    avgChargingCurrentCount = 0;
    totalChargeTime = 0.0;
    last_timer = timer_get_ticks();
}

void calcAccumlatedCharge( void ) {

    current_timer = timer_get_ticks();
    elapsed_time = current_timer - last_timer;

    if( elapsed_time >= 100 ) {
        instant_current =  fabsf( sensorVector[ISen2fIdx].realValue );
        avgChargingCurrent = avgChargingCurrent + instant_current;
        avgChargingCurrentCount = avgChargingCurrentCount + 1;
        totalChargeTime = totalChargeTime + ( (float)elapsed_time / 1000.0 );
        last_timer = current_timer;

        if( avgChargingCurrentCount == NUMBER_OF_AVG_CHARGING_CURRENT_COUNT) {
            avgChargingCurrent = avgChargingCurrent / (float)NUMBER_OF_AVG_CHARGING_CURRENT_COUNT;
            accumulatedCharge = accumulatedCharge + avgChargingCurrent;
            PRINT("\r\n===> accumulatedCharge:[%8.2f], totalChargeTime:[%8.2f],  avgChargingCurrent:[%8.2f]\r\n\r\n",
                  accumulatedCharge,
                  totalChargeTime,
                  avgChargingCurrent );
            avgChargingCurrent = 0.0;
            avgChargingCurrentCount = 0;
        }
    }
}




bool finallyCalcStateOfCharge() {

    float newCapacitance = 0.0;

    newCapacitance = accumulatedCharge / (finalVoltage - initialVoltage);

    PRINT("newCapacitance:[%8.2f]\r\n", newCapacitance);

    if( retriveStateOfChargeFromFlash(&energy_bank_condition) == true ) {

        if( energy_bank_condition.initializedSoCIndicator != 0xAAAA ) {

            PRINT("SOH FIRST CALC OF INITIAL CAPACITANCE \r\n");

            new_energy_bank_condition.stateOfHealthPercent= 100.0;
            new_energy_bank_condition.initialCapacitance = newCapacitance;
            new_energy_bank_condition.capacitance = newCapacitance;
            new_energy_bank_condition.initializedSoCIndicator = 0xAAAA;

            saveStateOfChargeToFlash(&new_energy_bank_condition);

        } else {

            PRINT("RETRIVED NEW SOH from flash energy_bank_condition.initialCapacitance :[%8.2f]\r\n",
                  energy_bank_condition.initialCapacitance );

            if( newCapacitance >= energy_bank_condition.initialCapacitance ) {
                newCapacitance = energy_bank_condition.initialCapacitance;
            }
            energy_bank_condition.stateOfHealthPercent = 100.0 * ( newCapacitance / energy_bank_condition.initialCapacitance );
            energy_bank_condition.capacitance = newCapacitance;
            saveStateOfChargeToFlash(&energy_bank_condition);
        }

        newEnergyBankConditionAvailable = true;
        return true;

    } else {

        return false;

    }
}


bool saveStateOfChargeToFlash(energy_bank_condition_t  *p_energy_bank_condition) {
    static uint8_t state = 0;
    uint8_t nextState = 0;
    bool retVal = false;

    PRINT("CPU2 saveStateOfChargeToFlash\r\n");
    PRINT("CPU2 energy_bank_condition.stateOfHealthPercent[%8.2f]\r\n", p_energy_bank_condition->stateOfHealthPercent);
    PRINT("CPU2 new_energy_bank_condition.initialCapacitance[%8.2f]]\r\n", p_energy_bank_condition->initialCapacitance );
    PRINT("CPU2 new_energy_bank_condition.capacitance[%8.2f]]\r\n",p_energy_bank_condition->capacitance);
    PRINT("CPU2 new_energy_bank_condition.initializedSoCIndicator[%d]\r\n ",p_energy_bank_condition->initializedSoCIndicator);

    memcpy(ipc_soh_msg, p_energy_bank_condition, sizeof(energy_bank_condition_t) );

    PRINT("CPU2 add:[%08lX] data:[%lu]\r\n", (uint32_t)&ipc_soh_msg, sizeof(energy_bank_condition_t));


    switch( state ) {
        case 0:
            IPC_sendCommand(IPC_CPU2_L_CPU1_R, IPC_FLAG_MESSAGE_CPU2_TO_CPU1,
                                           false,
                                           0x0000000D, //IPC_REQUEST_WRITE_SOH_TO_EXT_FLASH,
                                           (uint32_t)&ipc_soh_msg, sizeof(energy_bank_condition_t) );

            nextState = 1;
            break;
        case 1:
            if( !IPC_isFlagBusyLtoR(IPC_CPU2_L_CPU1_R, IPC_FLAG_MESSAGE_CPU2_TO_CPU1) ) {
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


bool retriveStateOfChargeFromFlash(energy_bank_condition_t *p_energy_bank_condition) {

    static uint8_t state = 0;
    uint8_t nextState = 0;
    bool retVal = false;
//    energy_bank_condition_t energy_bank_condition_from_flash;

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
                memcpy( p_energy_bank_condition, &energy_bank_condition_from_flash, sizeof(energy_bank_condition_t) );
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
    energy_bank_condition_t *p_energy_bank_condition;

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

//    if( timer_get_ticks() - last_time >= 7500 ) {
//        last_time = timer_get_ticks();
//        PRINT( "stateOfChargePercent:[%8.2f]%% \r\n", energy_bank_condition.stateOfChargePercent );
//        PRINT( "remaining_energy_to_min_soc_energy_bank:[%8.2f]J\r\n", sharedVars_cpu2toCpu1.remaining_energy_to_min_soc_energy_bank );
//        PRINT( "energy_bank_condition.capacitance:[%8.2f]F\r\n", energy_bank_condition.capacitance);
//    }

    switch( sohState.State_Current ) {

        case sohCalcWait:
            if( retriveStateOfChargeFromFlash( &energy_bank_condition ) == true) {

                if( energy_bank_condition.initializedSoCIndicator != 0xAAAA ) {

                    sharedVars_cpu2toCpu1.soh_energy_bank = 0.0;
                    sharedVars_cpu2toCpu1.remaining_energy_to_min_soc_energy_bank = 0.0;

                } else {
                    sharedVars_cpu2toCpu1.soh_energy_bank = energy_bank_condition.stateOfHealthPercent;
                    if( ( sharedVars_cpu1toCpu2.min_voltage_applied_to_energy_bank > 0.0 ) && ( DCDC_VI.avgVStore > 0 ) ) {
                        sharedVars_cpu2toCpu1.remaining_energy_to_min_soc_energy_bank = 0.5 * ( energy_bank_condition.capacitance *
                                ( powf( DCDC_VI.avgVStore, 2 ) - powf( sharedVars_cpu1toCpu2.min_voltage_applied_to_energy_bank , 2 ) ) );
                    } else {
                        sharedVars_cpu2toCpu1.remaining_energy_to_min_soc_energy_bank = 0.0;
                    }

                }
                if( StateVector.State_Current == Charge ) {
                    sohState.State_Next = sohCalcInit;
                }
            }

            break;

        case sohCalcInit:

            startCalcStateOfCharge();
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

            if( totalChargeTime < MINIMUM_CHARGING_TIME_IN_SECS ) {
                PRINT("Charging time lower then 60s\r\n");
                sohState.State_Next = sohCalcWait;
            } else {
                newCapacitance = accumulatedCharge / (finalVoltage - initialVoltage);
                sohState.State_Next = verifySoHFromFlash;
            }
            break;

        case verifySoHFromFlash:
            if( retriveStateOfChargeFromFlash(&energy_bank_condition) == true ) {
                if( energy_bank_condition.initializedSoCIndicator != 0xAAAA ) {
                    PRINT("SOH FIRST CALC OF INITIAL CAPACITANCE \r\n");

                    new_energy_bank_condition.stateOfHealthPercent= 100.0;
                    new_energy_bank_condition.initialCapacitance = newCapacitance;
                    new_energy_bank_condition.capacitance = newCapacitance;
                    new_energy_bank_condition.initializedSoCIndicator = 0xAAAA;

                    PRINT("new_energy_bank_condition.stateOfHealthPercent [%8.2f]\r\n",new_energy_bank_condition.stateOfHealthPercent);
                    PRINT("new_energy_bank_condition.initialCapacitance [%8.2f]\r\n",new_energy_bank_condition.initialCapacitance);
                    PRINT("new_energy_bank_condition.capacitance [%8.2f]\r\n",new_energy_bank_condition.capacitance);
                    PRINT("new_energy_bank_condition.initializedSoCIndicator [%04X]\r\n",new_energy_bank_condition.initializedSoCIndicator);

                    sohState.State_Next = saveNewEnergyConditionToFlash;

                } else {

                    PRINT("RETRIVED SOH from flash energy_bank_condition.initialCapacitance :[%8.2f]\r\n",
                          energy_bank_condition.initialCapacitance );

                    if( newCapacitance >= energy_bank_condition.initialCapacitance ) {
                        newCapacitance = energy_bank_condition.initialCapacitance;
                    }
                    energy_bank_condition.stateOfHealthPercent = 100.0 * ( newCapacitance / energy_bank_condition.initialCapacitance );
                    energy_bank_condition.capacitance = newCapacitance;

                    p_energy_bank_condition = &energy_bank_condition;
                    sohState.State_Next = saveEnergyConditionToFlash;
                }

            }
            break;

        case saveNewEnergyConditionToFlash:
                if( saveStateOfChargeToFlash( &new_energy_bank_condition ) == true ) {
                    sohState.State_Next = sohCalcWait;
                    PRINT( "stateOfHealthPercent:[%8.2f]%%\r\n", p_energy_bank_condition->stateOfHealthPercent );
                }
                break;


        case saveEnergyConditionToFlash:
                if( saveStateOfChargeToFlash( &energy_bank_condition ) == true ) {
                    sohState.State_Next = sohCalcWait;
                    PRINT( "stateOfHealthPercent:[%8.2f]%%\r\n", p_energy_bank_condition->stateOfHealthPercent );
                }
                break;
        default:
            sohState.State_Next = sohCalcWait;
    }
    if(  sohState.State_Current != sohState.State_Next ) {
        PRINT("sohState.State_Next:[%d] sohState.State_Current:[%d]\r\n", sohState.State_Next, sohState.State_Current);
    }
    sohState.State_Current = sohState.State_Next;

}




