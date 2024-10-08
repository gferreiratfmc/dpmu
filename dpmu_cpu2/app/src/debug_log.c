/*
 * debug_log.c
 *
 *  Created on: 18 de mar de 2024
 *      Author: gferreira
 */

#include <stdbool.h>
#include "common.h"
#include "debug_log.h"
#include "GlobalV.h"
#include "sensors.h"
#include "shared_variables.h"
#include "state_machine.h"
#include "switches.h"
#include "timer.h"


extern sharedVars_cpu2toCpu1_t sharedVars_cpu2toCpu1;
extern float cellVoltagesVector[30];

bool debug_log_enable_flag = false;
bool debug_log_force_update_flag = false ;
uint32_t debug_counter = 0;


uint32_t SetDebugLogPeriod(void) {
    uint32_t debug_period_in_ms = LOG_PERIOD_IDLE;
    if( debug_log_force_update_flag == true ) {
        debug_period_in_ms = 0;
        debug_log_force_update_flag = false;
    } else {
        switch( StateVector.State_Current ) {
            case Initialize:
            case SoftstartInitDefault:
            case Softstart:
                debug_period_in_ms = LOG_PERIOD_INITIALIZE;
                break;
            case TrickleChargeInit:
            case TrickleChargeDelay:
            case TrickleCharge:
                debug_period_in_ms = LOG_PERIOD_TRICKLE_CHARGE;
                break;
            case ChargeInit:
            case Charge:
            case ChargeStop:
            case ChargeConstantVoltageInit:
            case ChargeConstantVoltage:
            case BalancingInit:
            case Balancing:
            case CC_Charge:
                debug_period_in_ms = LOG_PERIOD_CHARGE;
                break;
            case RegulateInit:
            case Regulate:
            case RegulateStop:
            case RegulateVoltage:
            case RegulateVoltageInit:
            case RegulateVoltageStop:
                debug_period_in_ms = LOG_PERIOD_REGULATE;
                break;
            case StopEPWMs:
            case FaultDelay:
            case Fault:
            case Idle:
                debug_period_in_ms = LOG_PERIOD_IDLE;
                break;
            default:
                debug_period_in_ms = LOG_PERIOD_IDLE;
        }
        debug_log_enable_flag = true;
    }
    return debug_period_in_ms;
}

void DisableDebugLog(void){
    debug_log_enable_flag = false;
}

void EnableDebugLog(void){
    debug_log_enable_flag = true;
}


void ForceUpdateDebugLog(void) {
    debug_log_force_update_flag = true;
}


void UpdateDebugLog(void) {
    static uint32_t last_timer = 0;
    uint32_t current_timer, elapsed_time;
    static uint32_t debug_period_in_ms = LOG_PERIOD_IDLE;
    //uint16_t switches;
    debug_period_in_ms = SetDebugLogPeriod();
    if( debug_log_enable_flag == true ) {
        current_timer = timer_get_ticks();
        elapsed_time = current_timer - last_timer;
        if( current_timer < last_timer ) {
            current_timer = 0;
        } else if( ( elapsed_time >= debug_period_in_ms ) ) {
            debug_counter++;
            sharedVars_cpu2toCpu1.debug_log.counter = debug_counter;
            sharedVars_cpu2toCpu1.debug_log.ISen1 = sensorVector[ISen1fIdx].realValue * 10;     // Storage current sensor (supercap) x10
            sharedVars_cpu2toCpu1.debug_log.ISen2 = sensorVector[ISen2fIdx].realValue * 10;     // Output load current sensor x10
            sharedVars_cpu2toCpu1.debug_log.IF_1 = sensorVector[IF_1fIdx].realValue * 10;       // Input current x10
            sharedVars_cpu2toCpu1.debug_log.I_Dab2 = sensorVector[I_Dab2fIdx].realValue * 100;  // CLLC1 Current x100
            sharedVars_cpu2toCpu1.debug_log.I_Dab3 = sensorVector[I_Dab3fIdx].realValue * 100;  // CLLC2 Current x100
            sharedVars_cpu2toCpu1.debug_log.Vbus = sensorVector[VBusIdx].realValue * 10;        // VBus voltage x10
            sharedVars_cpu2toCpu1.debug_log.AvgVbus = DCDC_VI.avgVBus * 10;        // VBus voltage x10
            sharedVars_cpu2toCpu1.debug_log.VStore = sensorVector[VStoreIdx].realValue * 10;    // VStore voltage x10
            sharedVars_cpu2toCpu1.debug_log.AvgVStore  = DCDC_VI.avgVStore * 10;        // VBus voltage x10
            sharedVars_cpu2toCpu1.debug_log.RegulateAvgInputCurrent = DCDC_VI.RegulateAvgInputCurrent * 10;
            sharedVars_cpu2toCpu1.debug_log.RegulateAvgVStore = DCDC_VI.RegulateAvgVStore * 10;
            sharedVars_cpu2toCpu1.debug_log.RegulateAvgVbus = DCDC_VI.RegulateAvgVbus * 10;
            sharedVars_cpu2toCpu1.debug_log.RegulateAvgOutputCurrent = DCDC_VI.RegulateAvgOutputCurrent * 10;
            sharedVars_cpu2toCpu1.debug_log.RegulateIRef = DCDC_VI.I_Ref_Real * 100;
            sharedVars_cpu2toCpu1.debug_log.ILoop_PiOutput = ILoop_PiOutput.Output*100;


            sharedVars_cpu2toCpu1.debug_log.CurrentState = StateVector.State_Current;    // CPU2 current state of main state machine
            sharedVars_cpu2toCpu1.debug_log.elapsed_time = elapsed_time;
            for(int i=0; i<NUMBER_OF_CELLS; i++) {
                sharedVars_cpu2toCpu1.debug_log.cellVoltage[i] = cellVoltagesVector[i]*100;
            }

            //switches = 0;
            //switches = (GPIO_readPin(Qinb)==SW_ON) ? (switches |= QINB_MASK) : (switches &= ~QINB_MASK);
            //switches = (GPIO_readPin(Qlb)==SW_ON)  ? (switches |= QLB_MASK)  : (switches &= ~QLB_MASK);
            //switches = (GPIO_readPin(Qsb)==SW_ON) ? (switches |= QSB_MASK) : (switches &= ~QSB_MASK);
            //switches = (GPIO_readPin(168)==SW_ON) ? (switches |= QINRUSH_MASK) : (switches &= ~QINRUSH_MASK);

            //sharedVars_cpu2toCpu1.debug_log.Switches = switches;
            last_timer = current_timer;
        }
    }
}


void UpdateDebugLogFake(void) {
    static uint32_t last_timer = 0;
    uint32_t current_timer, elapsed_time;
    static uint32_t debug_period_in_ms = LOG_PERIOD_IDLE;
    debug_period_in_ms = SetDebugLogPeriod();
    if( debug_log_enable_flag == true ) {
        current_timer = timer_get_ticks();
        elapsed_time = current_timer - last_timer;
        if( current_timer < last_timer ) {
            current_timer = 0;
        } else if( ( elapsed_time >= debug_period_in_ms ) ) {
            debug_counter++;
            sharedVars_cpu2toCpu1.debug_log.ISen1 = 0x0011;     // Storage current sensor (supercap) x10
            sharedVars_cpu2toCpu1.debug_log.ISen2 = 0x1122;     // Output load current sensor x10
            sharedVars_cpu2toCpu1.debug_log.IF_1 =  0x3344;       // Input current x10
            sharedVars_cpu2toCpu1.debug_log.I_Dab2 = 0x5566;  // CLLC1 Current x100
            sharedVars_cpu2toCpu1.debug_log.I_Dab3 = 0x7788;  // CLLC2 Current x100
            sharedVars_cpu2toCpu1.debug_log.Vbus = 0x0900;        // VBus voltage x10
            sharedVars_cpu2toCpu1.debug_log.VStore = 0x0011;    // VStore voltage x10
            sharedVars_cpu2toCpu1.debug_log.AvgVbus = 0x9122;        // VBus voltage x10
            sharedVars_cpu2toCpu1.debug_log.AvgVStore  = 0x2233;        // VBus voltage x10
            sharedVars_cpu2toCpu1.debug_log.BaseBoardTemperature  = 0x3344;        // VBus voltage x10
            sharedVars_cpu2toCpu1.debug_log.MainBoardTemperature  = 0x4455;        // VBus voltage x10
            sharedVars_cpu2toCpu1.debug_log.MezzanineBoardTemperature  = 0x5566;        // VBus voltage x10
            sharedVars_cpu2toCpu1.debug_log.PowerBankBoardTemperature  = 0x6677;        // VBus voltage x10
            sharedVars_cpu2toCpu1.debug_log.RegulateAvgInputCurrent = 0x7788;
            sharedVars_cpu2toCpu1.debug_log.RegulateAvgOutputCurrent = 0x8899;
            sharedVars_cpu2toCpu1.debug_log.RegulateAvgVStore = 0x9900;
            sharedVars_cpu2toCpu1.debug_log.RegulateAvgVbus = 0x0011;
            sharedVars_cpu2toCpu1.debug_log.RegulateIRef = 0x1122;
            for(int i=0; i<NUMBER_OF_CELLS; i++) {
                //sharedVars_cpu2toCpu1.debug_log.cellVoltage[i] = cellVoltagesVector[i];
                sharedVars_cpu2toCpu1.debug_log.cellVoltage[i] = i;
            }
            sharedVars_cpu2toCpu1.debug_log.CurrentState = 0x6677;    // CPU2 current state of main state machine
            sharedVars_cpu2toCpu1.debug_log.counter = debug_counter;
            sharedVars_cpu2toCpu1.debug_log.CurrentTime = 0x2222;
            sharedVars_cpu2toCpu1.debug_log.elapsed_time = 0x8899; //sharedVars_cpu2toCpu1.debug_log.elapsed_time = elapsed_time;
            last_timer = current_timer;
        }
    }
}

