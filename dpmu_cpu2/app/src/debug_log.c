/*
 * debug_log.c
 *
 *  Created on: 18 de mar de 2024
 *      Author: gferreira
 */

#include <stdbool.h>
#include "cli_cpu2.h"
#include "common.h"
#include "debug_log.h"
#include "GlobalV.h"
#include "log_queue.h"
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

debug_log_t current_debug_log_message;

QUEUE queue;

void InitDebugLogQueue() {
    init_queue(queue);
}

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
            case RegulateVoltageWait:
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


//void UpdateDebugLog(void)
bool CreateDebugLogMessage(void) {

    bool retVal = false;
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
            current_debug_log_message.counter = debug_counter;
            current_debug_log_message.ISen1 = sensorVector[ISen1fIdx].realValue * 10;     // Storage current sensor (supercap) x10
            current_debug_log_message.ISen2 = sensorVector[ISen2fIdx].realValue * 10;     // Output load current sensor x10
            current_debug_log_message.IF_1 = sensorVector[IF_1fIdx].realValue * 10;       // Input current x10
            current_debug_log_message.I_Dab2 = sensorVector[I_Dab2fIdx].realValue * 100;  // CLLC1 Current x100
            current_debug_log_message.I_Dab3 = sensorVector[I_Dab3fIdx].realValue * 100;  // CLLC2 Current x100
            current_debug_log_message.Vbus = sensorVector[VBusIdx].realValue * 10;        // VBus voltage x10
            current_debug_log_message.AvgVbus = DCDC_VI.avgVBus * 10;        // VBus voltage x10
            current_debug_log_message.VStore = sensorVector[VStoreIdx].realValue * 10;    // VStore voltage x10
            current_debug_log_message.AvgVStore  = DCDC_VI.avgVStore * 10;        // VBus voltage x10
            current_debug_log_message.RegulateAvgInputCurrent = DCDC_VI.RegulateAvgInputCurrent * 10;
            current_debug_log_message.RegulateAvgVStore = DCDC_VI.RegulateAvgVStore * 10;
            current_debug_log_message.RegulateAvgVbus = DCDC_VI.RegulateAvgVbus * 10;
            current_debug_log_message.RegulateAvgOutputCurrent = DCDC_VI.RegulateAvgOutputCurrent * 10;
            current_debug_log_message.RegulateIRef = DCDC_VI.I_Ref_Real * 100;
            current_debug_log_message.ILoop_PiOutput = ILoop_PiOutput.Output*100;


            current_debug_log_message.CurrentState = StateVector.State_Current;    // CPU2 current state of main state machine
            current_debug_log_message.elapsed_time = elapsed_time;
            for(int i=0; i<NUMBER_OF_CELLS; i++) {
                current_debug_log_message.cellVoltage[i] = cellVoltagesVector[i]*100;
            }

            //switches = 0;
            //switches = (GPIO_readPin(Qinb)==SW_ON) ? (switches |= QINB_MASK) : (switches &= ~QINB_MASK);
            //switches = (GPIO_readPin(Qlb)==SW_ON)  ? (switches |= QLB_MASK)  : (switches &= ~QLB_MASK);
            //switches = (GPIO_readPin(Qsb)==SW_ON) ? (switches |= QSB_MASK) : (switches &= ~QSB_MASK);
            //switches = (GPIO_readPin(168)==SW_ON) ? (switches |= QINRUSH_MASK) : (switches &= ~QINRUSH_MASK);

            //current_debug_log.Switches = switches;
            last_timer = current_timer;
            retVal = true;
        }
    }
    return retVal;
}

void UpdateDebugLog(void) {

    enum UDLSMStates { UDLCreateDebugMessage = 0, UDLStartEnqueueMessage, UDLWaitEnqueueMessage, UDLStartDequeMessage, UDLWaitDequeMessage };
    static States_t UDLSM = { 0 };

    switch( UDLSM.State_Current ) {

        case UDLCreateDebugMessage:
            if( CreateDebugLogMessage() == true) {
                UDLSM.State_Next = UDLStartEnqueueMessage;
                PRINT("LOG MESSAGE CREATED [%lu]\r\n", current_debug_log_message.counter);
            } else {
                UDLSM.State_Next = UDLStartDequeMessage;
            }
            break;

        case UDLStartEnqueueMessage:
            if( strtEnque(&queue) == true) {
                //PRINT("strtEnque queue.current_load=[%d]\r\n",queue.current_load );
                UDLSM.State_Next = UDLWaitEnqueueMessage;
            } else {
                UDLSM.State_Next = UDLStartDequeMessage;
            }
            break;

        case UDLWaitEnqueueMessage:
            if( copyMessageToQueDone( &queue, &current_debug_log_message ) == true ) {
                    UDLSM.State_Next = UDLStartDequeMessage;
                    PRINT("LOG MESSAGE ENQUEUED [%lu] queue.current_load=[%d]\r\n", current_debug_log_message.counter, queue.current_load);
            }
            break;

        case UDLStartDequeMessage:
            UDLSM.State_Next = UDLCreateDebugMessage;
            if( sharedVars_cpu1toCpu2.log_external_flash_busy == false ) {

                if( startDeque(&queue) == true) {
                    //PRINT("startDeque queue.current_load=[%d]\r\n",queue.current_load );
                    UDLSM.State_Next = UDLWaitDequeMessage;
                }
            }
            break;

        case UDLWaitDequeMessage:
            if( copyMessageFromQueDone(&queue, &sharedVars_cpu2toCpu1.debug_log ) == true ) {
                    UDLSM.State_Next = UDLStartDequeMessage;
                    PRINT("LOG MESSAGE DEQUEUED [%lu] queue.current_load[%d]r\n\r\n", sharedVars_cpu2toCpu1.debug_log.counter, queue.current_load);
            }
            break;

        default:
            UDLSM.State_Next = UDLCreateDebugMessage;
    }
    UDLSM.State_Current = UDLSM.State_Next;
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
            current_debug_log_message.ISen1 = 0x0011;     // Storage current sensor (supercap) x10
            current_debug_log_message.ISen2 = 0x1122;     // Output load current sensor x10
            current_debug_log_message.IF_1 =  0x3344;       // Input current x10
            current_debug_log_message.I_Dab2 = 0x5566;  // CLLC1 Current x100
            current_debug_log_message.I_Dab3 = 0x7788;  // CLLC2 Current x100
            current_debug_log_message.Vbus = 0x0900;        // VBus voltage x10
            current_debug_log_message.VStore = 0x0011;    // VStore voltage x10
            current_debug_log_message.AvgVbus = 0x9122;        // VBus voltage x10
            current_debug_log_message.AvgVStore  = 0x2233;        // VBus voltage x10
            current_debug_log_message.BaseBoardTemperature  = 0x3344;        // VBus voltage x10
            current_debug_log_message.MainBoardTemperature  = 0x4455;        // VBus voltage x10
            current_debug_log_message.MezzanineBoardTemperature  = 0x5566;        // VBus voltage x10
            current_debug_log_message.PowerBankBoardTemperature  = 0x6677;        // VBus voltage x10
            current_debug_log_message.RegulateAvgInputCurrent = 0x7788;
            current_debug_log_message.RegulateAvgOutputCurrent = 0x8899;
            current_debug_log_message.RegulateAvgVStore = 0x9900;
            current_debug_log_message.RegulateAvgVbus = 0x0011;
            current_debug_log_message.RegulateIRef = 0x1122;
            for(int i=0; i<NUMBER_OF_CELLS; i++) {
                //current_debug_log_message.cellVoltage[i] = cellVoltagesVector[i];
                current_debug_log_message.cellVoltage[i] = i;
            }
            current_debug_log_message.CurrentState = 0x6677;    // CPU2 current state of main state machine
            current_debug_log_message.counter = debug_counter;
            current_debug_log_message.CurrentTime = 0x2222;
            current_debug_log_message.elapsed_time = 0x8899; //current_debug_log.elapsed_time = elapsed_time;
            last_timer = current_timer;
        }
    }
}

