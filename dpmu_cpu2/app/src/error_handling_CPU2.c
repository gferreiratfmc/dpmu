/*
 * error_handling.c
 *
 *  Created on: 18 de jun de 2024
 *      Author: gferreira
 */

#include <stdbool.h>

#include "common.h"
#include "cli_cpu2.h"
#include "DCDC.h"
#include "error_handling.h"
#include "error_handling_CPU2.h"
#include "sensors.h"
#include "shared_variables.h"
#include "state_machine.h"
#include "switches.h"
#include "timer.h"

bool dpmuErrorOcurredFlag = false;


bool DpmuErrorOcurred() {
    return dpmuErrorOcurredFlag;
}

void ResetDpmuErrorOcurred() {
    dpmuErrorOcurredFlag = false;
}


void HandleLoadOverCurrent(float max_allowed_load_current, uint16_t efuse_top_half_flag) {

    //LOC => Load Over Current
    enum { LOCInit, LOCWait, LOCStopMainStateMachine, LOCEnd };

    static uint32_t lastTime = 0;
    static States_t stateLOC = {0};



    switch( stateLOC.State_Current ) {
        case LOCInit:
            lastTime = timer_get_ticks();

            if (sensorVector[ISen1fIdx].realValue > (float) max_allowed_load_current ) {
                sharedVars_cpu2toCpu1.error_code |= (1UL << ERROR_LOAD_OVER_CURRENT);
                stateLOC.State_Next = LOCWait;
            } else {
                sharedVars_cpu2toCpu1.error_code &= ~(1UL << ERROR_LOAD_OVER_CURRENT);
            }
            break;

        case LOCWait:
            if( ( timer_get_ticks() - lastTime ) > 1000 ) {
                if (sensorVector[ISen1fIdx].realValue > (float) max_allowed_load_current ) {
                    stateLOC.State_Next = LOCStopMainStateMachine;
                } else {
                    stateLOC.State_Next = LOCInit;
                }
            }
            break;

        case LOCStopMainStateMachine:
            lastTime = timer_get_ticks();
            PRINT("sensorVector[ISen1fIdx]:[%5.2f] max_allowed_load_current:[%5.2f] ",sensorVector[ISen1fIdx].realValue, max_allowed_load_current);
            // Sends main state_machine to Fault state
            dpmuErrorOcurredFlag = true;
            stateLOC.State_Next = LOCEnd;
            break;
        case LOCEnd:
            if( dpmuErrorOcurredFlag == false ) {
                stateLOC.State_Next = LOCInit;
            }
            break;

        default:
            stateLOC.State_Next = LOCInit;
    }
    if( efuse_top_half_flag == true) {
        sharedVars_cpu2toCpu1.error_code |= (1UL << ERROR_LOAD_OVER_CURRENT);
        stateLOC.State_Next = LOCStopMainStateMachine;
    }
    stateLOC.State_Current = stateLOC.State_Next;

    // Original OLC Handler
    //    /* check for DC bus load current */
    //    if (sensorVector[ISen1fIdx].realValue > (float) max_allowed_load_current|| efuse_top_half_flag == true) {
    //        PRINT("sensorVector[ISen1fIdx]:[%5.2f] max_allowed_load_current:[%5.2f] ",sensorVector[ISen1fIdx].realValue, max_allowed_load_current);
    //        sharedVars_cpu2toCpu1.error_code |= (1UL << ERROR_LOAD_OVER_CURRENT);
    //        dpmuErrorOcurredFlag = true;
    //    } else {
    //        sharedVars_cpu2toCpu1.error_code &= ~(1UL << ERROR_LOAD_OVER_CURRENT);
    //    }
}


void HandleDCBusShortCircuit()
{
    /* check for DC bus shortage verifying all current sensors 25% above maximum DPMU current*/

    if (    fabsf(sensorVector[ISen1fIdx].realValue) > DPMU_SHORT_CIRCUIT_CURRENT ||
            sensorVector[ISen2fIdx].realValue > DPMU_SUPERCAP_SHORT_CIRCUIT_CURRENT ||
            fabsf(sensorVector[IF_1fIdx].realValue) > DPMU_SHORT_CIRCUIT_CURRENT ||
            efuse_top_half_flag == true) {
        switches_Qlb( SW_OFF );
        switches_Qsb( SW_OFF );
        switches_Qinb( SW_OFF );
        StopAllEPWMs();
        sharedVars_cpu2toCpu1.error_code |= (1UL << ERROR_BUS_SHORT_CIRCUIT);
        dpmuErrorOcurredFlag = true;
        PRINT("ISen1f:[%5.2f]  or ISen2f:[%5.2f] or IF_1fIdx:[%5.2f] > SHORT_CIRCUIT_CURRENT:[%5.2f]\r\n",
                     sensorVector[ISen1fIdx].realValue,
                     sensorVector[ISen2fIdx].realValue,
                     sensorVector[IF_1fIdx].realValue,
                     DPMU_SHORT_CIRCUIT_CURRENT);
    } else {
        sharedVars_cpu2toCpu1.error_code &= ~(1UL << ERROR_BUS_SHORT_CIRCUIT);
    }

    // Original DCBUS ShortCircuit Handler
    //    if (sensorVector[VBusIdx].realValue
    //            < (float) sharedVars_cpu1toCpu2.vdc_bus_short_circuit_limit)
    //        sharedVars_cpu2toCpu1.error_code |= (1UL << ERROR_BUS_SHORT_CIRCUIT);
    //    else
    //        sharedVars_cpu2toCpu1.error_code &= ~(1UL << ERROR_BUS_SHORT_CIRCUIT);


}


void HandleDCBusOverVoltage() {
    // BOV => DCBus Over Voltage
    enum { BOVInit, BOVWait, BOVStopMainStateMachine, BOVEnd };

    static uint32_t lastTime = 0;
    static States_t stateBOV = {0};



    switch( stateBOV.State_Current ) {
        case BOVInit:
            lastTime = timer_get_ticks();

            if ( DCDC_VI.avgVBus > sharedVars_cpu1toCpu2.max_allowed_dc_bus_voltage)  {
                sharedVars_cpu2toCpu1.error_code |= (1UL << ERROR_BUS_OVER_VOLTAGE);
                switches_Qlb( SW_OFF );
                stateBOV.State_Next = BOVWait;
            } else {
                sharedVars_cpu2toCpu1.error_code &= ~(1UL << ERROR_BUS_OVER_VOLTAGE);
            }
            break;

        case BOVWait:
            if( ( timer_get_ticks() - lastTime ) > 1000 ) {
                if (DCDC_VI.avgVBus > sharedVars_cpu1toCpu2.max_allowed_dc_bus_voltage ) {
                    switches_Qinb( SW_OFF );
                    switches_Qsb( SW_OFF );
                    StopAllEPWMs();
                    stateBOV.State_Next = BOVStopMainStateMachine;
                } else {
                    stateBOV.State_Next = BOVInit;
                }
            }
            break;

        case BOVStopMainStateMachine:
            lastTime = timer_get_ticks();
            PRINT("DCDC_VI.avgVBus[%5.2f] max_allowed_dc_bus_voltage:[%5.2f]\r\n ",
                  DCDC_VI.avgVBus, sharedVars_cpu1toCpu2.max_allowed_dc_bus_voltage);
            // Sends main state_machine to Fault state
            dpmuErrorOcurredFlag = true;
            stateBOV.State_Next = BOVEnd;
            break;
        case BOVEnd:
            if( dpmuErrorOcurredFlag == false ) {
                stateBOV.State_Next = BOVInit;
            }
            break;

        default:
            stateBOV.State_Next = BOVInit;
    }
    stateBOV.State_Current = stateBOV.State_Next;
}

void HandleDCBusUnderVoltage() {
    if (DCDC_VI.avgVBus < (float) sharedVars_cpu1toCpu2.min_allowed_dc_bus_voltage) {
        sharedVars_cpu2toCpu1.error_code |= (1UL << ERROR_BUS_UNDER_VOLTAGE);
    } else {
        sharedVars_cpu2toCpu1.error_code &= ~(1UL << ERROR_BUS_UNDER_VOLTAGE);
    }
}

void HandleOverTemperature() {
    enum { TEMPInit, TEMPWait };
    static States_t stateTemp = {0};

    switch( stateTemp.State_Current ) {
        case TEMPInit:
            if( sharedVars_cpu1toCpu2.temperatureMaxLimitReachedFlag == true ) {
                PRINT("******* Absolute Max Limit Reached detected.\r\n"
                        "******* Send main state machine to FAULT state\r\n");
                stateTemp.State_Next = TEMPWait;
                }
        break;
        case TEMPWait:
            if( sharedVars_cpu1toCpu2.temperatureMaxLimitReachedFlag == false ) {
                PRINT("******* Normal limit Reached detected.\r\n");
                stateTemp.State_Next = TEMPInit;
            }
    }
    stateTemp.State_Current = stateTemp.State_Next;
}


