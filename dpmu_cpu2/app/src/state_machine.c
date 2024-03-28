/*
 ******************************************************************************
 * @file    State_Machine.c
 * @brief   This file provides code for the State Machine.

 ******************************************************************************
 *          Created on: 8 Dec 2022
 *          Author: Luyu Wang
 *
 ******************************************************************************
 */

#include <error_handling.h>
#include <stdbool.h>

#include "balancing.h"
#include "cli_cpu2.h"
#include "CLLC.h"
#include "cli_cpu2.h"
#include "common.h"
#include "DCDC.h"
#include "debug_log.h"
#include "energy_storage.h"
#include "GlobalV.h"
#include "hal.h"
#include "sensors.h"
#include "shared_variables.h"
#include "state_machine.h"
#include "switches.h"
#include "switch_matrix.h"
#include "timer.h"

#define TRACK_STATE_BUFFER_SIZE 12

bool DPMUInitialized = false;
extern bool updateCPU2FirmwareFlag;

uint16_t trackingStates[TRACK_STATE_BUFFER_SIZE];
uint16_t trackStatesCount = 0;
extern uint16_t PhaseshiftCount;
uint16_t TestCellNr = 1;
//bool run_inrush_current_limiter = false;

extern bool test_update_of_error_codes;

float cellVoltagesVector[30];     /* used for first charge - SoH */
float energyBankVoltage;    /* used for first charge - SoH */
bool cellVoltageOverThreshold;

static uint8_t convert_current_state_to_OD(uint16_t state);

void CheckCommandFromIOP(void);
int DoneWithInrush(void);
void TrackStatesForDEBUG(void);
void EnableOrDisblePWM();
uint16_t ConvCellNrToIdx(uint16_t celNr);
inline void EnableEFuseBBToStopDCDC_EPWM();
void update_debug_log(void);

void StateMachine(void)
{
    static bool first_CC_charge_stage_done = false;
    static bool cellVoltageOverThreshold = false;
    static float I_Ref_Real_Final = 0.0;
    static bool EPMWStarted = false;

    ConvertCountsToRealOfAllSensors();
    CalculateAvgVStore();
    CalculateAvgVBus();

    switch (StateVector.State_Current)
    {
    case Initialize: /* Initial state, initialization of the parameters*/
        if( !DPMUInitialized) {
            if( CalibrateZeroVoltageOffsetOfSensors() ) {
                test_update_of_error_codes = true;
                HAL_DcdcNormalModePwmSetting();
                StateVector.State_Next = SoftstartInit;

            }
        } else {
            StateVector.State_Next = StateVector.State_Before;
        }

        break;

    case SoftstartInit:

        //Close output Switch
        switches_Qlb(SW_ON);
        switches_Qsb(SW_ON);

        // Initialize and start Inrush PWM
        HAL_PWM_setCounterCompareValue(InrushCurrentLimit_BASE, EPWM_COUNTER_COMPARE_A, INRUSH_DUTY_CYLE_INCREMENT);
        HAL_StartPwmInrushCurrentLimit();
        CounterGroup.InrushCurrentLimiterCounter = 0;
        StateVector.State_Next = Softstart;
        break;

    case Softstart: /* Soft start of the 200V Bus*/

        if( DoneWithInrush() ) {
            EnableEFuseBBToStopDCDC_EPWM();
            switches_Qinb(SW_ON);
            //Stop In-rush PWM
            HAL_PWM_setCounterCompareValue(InrushCurrentLimit_BASE, EPWM_COUNTER_COMPARE_A, 1);
            HAL_StopPwmInrushCurrentLimit();
            DPMUInitialized = true;
            EnableContinuousReadCellVoltages();
            StateVector.State_Next = Idle;
        }
        break;

    case TrickleChargeInit:
        if ( DCDC_VI.avgVStore >= energy_bank_settings.preconditional_threshold ) {
            StateVector.State_Next = ChargeInit;
        } else {
            HAL_DcdcPulseModePwmSetting();
            HAL_StartPwmDCDC();
            CounterGroup.PrestateCounter = DELAY_50_SM_CYCLES;
            StateVector.State_Next = TrickleChargeDelay;
            trickleChargeRangeState = 0;
        }
        break;

    case TrickleChargeDelay:
        CounterGroup.PrestateCounter--;
        if (CounterGroup.PrestateCounter == 0)
        {
            StateVector.State_Next = TrickleCharge;
        }
        break;

    case TrickleCharge: /* Pulse charge state for when the super capacitor bank voltage is low */
        IncreasePulseStateDutyToSuperCapsVoltage();
        if ( DCDC_VI.avgVStore >= energy_bank_settings.preconditional_threshold)
        {
            HAL_StopPwmDCDC();
            StateVector.State_Next = ChargeInit;
        }
        if ( ReadCellVoltagesDone() == true){
            if( cellVoltageOverThreshold ){
                StateVector.State_Next = StopEPWMs;
            }
        }
        break;

    case ChargeInit: /* Initialize the buck state */


        /* if needed, store the Voltage of energy bank and of each energy cell,
         * do this before going into CC Charge for correct values
         * do this before starting any related PWMs
         * */
//        if( save_voltages_before_first_cc_charge( &first_CC_charge_stage_done, cellVoltageVector,&energyBankVoltage ) )
//        {
            /* save_voltages_before_first_cc_charge() returns true if initial
             * values have been stored
             *
             * save_voltages_before_first_cc_charge() returns true if we are not
             * discharged enough to be able to do a new SoH calculation
             *
             * save_voltages_before_first_cc_charge() returns false if we are in
             * a state there we can't jump directly to state Charge due to
             * needed recalculation of SoH and for that we need to store new
             * Voltage measurements before jumping to state Charge
             */
            HAL_DcdcNormalModePwmSetting();
            ILoop_PiOutput.Int_out = 0;

            I_Ref_Real_Final = 0.5 * (  DCDC_VI.target_Voltage_At_DCBus * DCDC_VI.iIn_limit / energy_bank_settings.max_voltage_applied_to_energy_bank );

            if( I_Ref_Real_Final >  MAX_INDUCTOR_BUCK_CURRENT ) {
                I_Ref_Real_Final = MAX_INDUCTOR_BUCK_CURRENT;
            }
            DCDC_VI.I_Ref_Real = I_Ref_Real_Final / 100.0;

            //DCDC_current_buck_loop_float();
            HAL_PWM_setCounterCompareValue( BEG_1_2_BASE, EPWM_COUNTER_COMPARE_A, 0.5*EPWM_getTimeBasePeriod(BEG_1_2_BASE) );
            //HAL_StartPwmDCDC();
            EPMWStarted = false;
            DCDC_VI.counter = 0;
            StateVector.State_Next = ChargeRamp;
//        }

        break;

    case ChargeRamp:
        if( !EPMWStarted ) {
            HAL_StartPwmDCDC();
            EPMWStarted = true;
        }
        DCDC_current_buck_loop_float();
        if( DCDC_VI.counter == DELAY_50_SM_CYCLES) {
            DCDC_VI.counter = 0;
            if( -(sensorVector[ISen2fIdx].realValue) >= DCDC_VI.I_Ref_Real ) {
                DCDC_VI.I_Ref_Real = DCDC_VI.I_Ref_Real + ( I_Ref_Real_Final / 100.0 );
                if( DCDC_VI.I_Ref_Real >= I_Ref_Real_Final ) {
                    DCDC_VI.I_Ref_Real = I_Ref_Real_Final;
                    StateVector.State_Next = Charge;
                }
            }
        } else {
            DCDC_VI.counter++;
        }
        break;

    case Charge:

        if( DCDC_VI.avgVStore < energy_bank_settings.max_voltage_applied_to_energy_bank ) {
            DCDC_current_buck_loop_float();
        } else {
            StateVector.State_Next = ChargeStop;
        }

        if( ReadCellVoltagesDone() == true) {
            if( cellVoltageOverThreshold == true ) {
                //StateVector.State_Next = BalancingInit; //Normal operation
                StateVector.State_Next = ChargeStop;   // Endurance operation. Balaning not completely implemented
            }
        }

        if(!first_CC_charge_stage_done)
            /* increment charge timer */
            sharedVars_cpu2toCpu1.energy_bank.chargeTime++;
        break;

    case ChargeStop:
            DCDC_VI.I_Ref_Real = 0.0;
            DCDC_current_buck_loop_float();
            if( sensorVector[ISen2fIdx].realValue < 0.25 ) {
                StateVector.State_Next = StopEPWMs;
            }
          break;

    case BalancingInit:
        DCDC_VI.I_Ref_Real = 0.0;
        DCDC_current_buck_loop_float();
        DisableContinuousReadCellVoltages();

        /************** Testing CLLC BEGIN ********/
        if( TestCellNr == BAT_15_N ) {
            TestCellNr = BAT_16;
        }
        switch_matrix_reset();
        switch_matrix_connect_cell( TestCellNr );
        switch_matrix_set_cell_polarity( TestCellNr );
        if( TestCellNr <= BAT_15 ) {
            HAL_StopPwmCllcCellDischarge2();
            HAL_StartPwmCllcCellDischarge1();
        } else {
            HAL_StopPwmCllcCellDischarge1();
            HAL_StartPwmCllcCellDischarge2();
        }
        /************** Testing CLLC END ********/

        if( sensorVector[ISen2fIdx].realValue < 0.25 ) {
            StateVector.State_Before_Balancing = StateVector.State_Before;
            HAL_StopPwmDCDC();
            StateVector.State_Next = Balancing;
        }


        break;

    case Balancing: // Balancing state supposed to run in parallel with Charge state.

        /************** Testing CLLC BEGIN ********/
        if( TestCellNr <= BAT_15 ) {
            HAL_PWM_setPhaseShift(QABPWM_6_7_BASE, PhaseshiftCount);
        } else {
            HAL_PWM_setPhaseShift(QABPWM_14_15_BASE, PhaseshiftCount);
        }
        /************** Testing CLLC END ********/

//        if( BalancingAllCells( &cellVoltagesVector[0] ) == true ) {
//           StateVector.State_Next = StateVector.State_Before_Blanancing;
//        }
        break;

    case Keep:
        // TODO: Change the threshold to re-charge to the correct variable variable
        if (sensorVector[VStoreIdx].realValue <  DCDC_VI.avgVBus * 0.90 )
        {
            StateVector.State_Next = ChargeInit;
        }

        break;

    case RegulateInit: /* Initialize the boost state */
        DCDC_VI.I_Ref_Real = 0.0;
        StateVector.State_Next = Regulate;
        HAL_StopPwmDCDC();
        HAL_DcdcRegulateModePwmSetting();
        break;

    case Regulate:
        calculate_boost_current();
        DCDC_current_boost_loop_float();
        EnableOrDisblePWM(DCDC_VI.I_Ref_Real);
        if( sensorVector[VStoreIdx].realValue < energy_bank_settings.min_voltage_applied_to_energy_bank ) {
            StateVector.State_Next = RegulateStop;
        }
        break;

    case RegulateStop:
        DCDC_VI.I_Ref_Real = 0.0;
        DCDC_current_boost_loop_float();
        if( sensorVector[ISen2fIdx].realValue < 0.25 ) {
            StateVector.State_Next = StopEPWMs;
        }

        break;

    case Fault: /* deal with the Fault */

        HAL_StopPwmDCDC();

        switches_Qlb(    SW_OFF);
        switches_Qsb(    SW_OFF);
        switches_Qinb(   SW_OFF);

        CounterGroup.PrestateCounter = 0;

        test_update_of_error_codes = false;

        DPMUInitialized = false;

        StateVector.State_Next = StopEPWMs;

        break;

    case StopEPWMs:

        HAL_StopPwmDCDC();
        HAL_StopPwmCllcCellDischarge1();
        HAL_StopPwmCllcCellDischarge2();
        HAL_PWM_setCounterCompareValue(InrushCurrentLimit_BASE, EPWM_COUNTER_COMPARE_A, 0); /* last one to turn off, least impact */
        StateVector.State_Next = Idle;
        break;
        
    case Idle: /* Idle do nothing */
        if( StatusContinousReadCellVoltages() == false) {
            EnableContinuousReadCellVoltages();
        }
        break;
    default:
        /* should never enter here */
        break;
    }

    ReadCellVoltagesStateMachine( &cellVoltagesVector[0], &energyBankVoltage, &cellVoltageOverThreshold);

    /*** commands from CPU1/IOP ***/
    /* check if IOP request for  a change of state */
    CheckCommandFromIOP();
    //TrackStatesForDEBUG();
    /* print next state then state changes */
    if(StateVector.State_Current  != StateVector.State_Next) {
        ForceUpdateDebugLog();
        PRINT("Next state %02d\r\n", StateVector.State_Next);
    }

    /* update current state */
    StateVector.State_Before = StateVector.State_Current;
    StateVector.State_Current = StateVector.State_Next;

    /* reflect state in CANopen OD */
    sharedVars_cpu2toCpu1.current_state = convert_current_state_to_OD(StateVector.State_Current);

    CounterGroup.StateMachineCounter++;


}

static uint8_t convert_current_state_to_OD(uint16_t state)
{
    uint16_t retVal;

    switch(state)
    {
    case Idle:
    case StopEPWMs:
        retVal = Idle;
        break;
    case Initialize:         // 1
    case SoftstartInit:          // 2
    case Softstart:              // 3
        retVal = Initialize;
        break;
    case TrickleChargeInit:      // 4
    case TrickleChargeDelay:     // 5
    case TrickleCharge:          // 6
    case ChargeRamp:
    case ChargeStop:
    case ChargeInit:             // 7
    case Charge:                 // 8
    case ChargeConstantVoltage:  // 9
    case BalancingInit:          // 10
    case Balancing:              // 11
        retVal = Charge;
        break;
    case RegulateInit:           // 12
    case Regulate:               // 13
    case RegulateStop:               // 13
        retVal = Regulate;
        break;
    case Fault:                  // 14
    case FaultDelay:             // 15
        retVal = Fault;
        break;
    case Keep:                   // 16
        retVal = Keep;
        break;
    default:
        break;
    }

    return retVal & 0xff;
}



int DoneWithInrush(void)
{
    uint16_t inrushComplete = 0;
    uint16_t InrushPWMCompareAValue = 1;

    // Intensify in-rush current gradually increasing In-rush PWM duty cycle
    if (CounterGroup.InrushCurrentLimiterCounter > DELAY_100_SM_CYCLES)
    {
        InrushPWMCompareAValue = HAL_PWM_getCounterCompareValue( InrushCurrentLimit_BASE, EPWM_COUNTER_COMPARE_A) + 5;

        // Limit the duty cycle of the in-rush PWM to 99%
        if ( InrushPWMCompareAValue > HAL_EPWM_getTimeBasePeriod(InrushCurrentLimit_BASE) * MAX_INRUSH_DUTY_CYCLE )
        {
            InrushPWMCompareAValue = HAL_EPWM_getTimeBasePeriod( InrushCurrentLimit_BASE) * MAX_INRUSH_DUTY_CYCLE;

            inrushComplete = 1;
        }
        HAL_PWM_setCounterCompareValue(InrushCurrentLimit_BASE, EPWM_COUNTER_COMPARE_A, InrushPWMCompareAValue);
        CounterGroup.InrushCurrentLimiterCounter = 0;
    }
    else
    {
        CounterGroup.InrushCurrentLimiterCounter++;
    }
    return inrushComplete;
}



void TrackStatesForDEBUG(void)
{
    /*** For debugging purposes ***/
    if (StateVector.State_Current != StateVector.State_Next)
    {
        trackingStates[trackStatesCount] = StateVector.State_Next;
        trackStatesCount++;
        if (trackStatesCount == TRACK_STATE_BUFFER_SIZE)
        {
            trackStatesCount = 0;
        }
    }
}

void StateMachineInit(void)
{
    StateVector.State_Before = Idle;
    StateVector.State_Current = Idle;
    StateVector.State_Next = Idle;

    CounterGroup.PrestateCounter = 0;
    CounterGroup.StateMachineCounter = 0;
}

void CheckCommandFromIOP(void)
{
    /*** commands from CPU1/IOP ***/
    /* check if IOP request for  a change of state */
    if (IPC_isFlagBusyRtoL(IPC_CPU1_L_CPU2_R, IPC_IOP_REQUEST_CHANGE_OF_STATE))
    {
        StateVector.State_Next = (uint16_t) (sharedVars_cpu1toCpu2.iop_operation_request_state);
        if( StateVector.State_Next == Fault) {
            IPC_ackFlagRtoL(IPC_CPU1_L_CPU2_R, IPC_IOP_REQUEST_CHANGE_OF_STATE);
            return;
        }
        if ( !DPMUInitialized ) {
            if( StateVector.State_Next != Initialize ) {
                StateVector.State_Next = Idle;
            }
        } else {
            switch( StateVector.State_Next )
            {
                case Idle:
                    switch ( StateVector.State_Current )
                    {
                        case Idle:
                            /* No change if it is in Idle already */
                        break;
                        case RegulateInit:
                        case Regulate:
                            StateVector.State_Next = RegulateStop;
                            break;
                        case TrickleCharge:
                        case TrickleChargeDelay:
                        case TrickleChargeInit:
                        case ChargeInit:
                        case Charge:
                            StateVector.State_Next = ChargeStop;
                            break;
                        default:
                            StateVector.State_Next = StopEPWMs;
                            break;
                    }
                    break;

                case Initialize:
                case TrickleChargeInit:
                case RegulateInit:
                    if( StateVector.State_Current != Idle ) {
                        StateVector.State_Next = StateVector.State_Current;
                    }
                    break;
                case Fault:
                    break;
                default:
                    StateVector.State_Next = StateVector.State_Current;
                    break;
            }
        }
        IPC_ackFlagRtoL(IPC_CPU1_L_CPU2_R, IPC_IOP_REQUEST_CHANGE_OF_STATE);
    }
    /* check if CPU1 requires an emergency turn off */
    if (IPC_isFlagBusyRtoL(IPC_CPU1_L_CPU2_R, IPC_CPU1_REQUIERS_EMERGECY_SHUT_DOWN))
    {
        StateVector.State_Next = Fault;
        IPC_ackFlagRtoL(IPC_CPU1_L_CPU2_R, IPC_CPU1_REQUIERS_EMERGECY_SHUT_DOWN);
    }
}

inline void EnableEFuseBBToStopDCDC_EPWM()
{
    Interrupt_clearACKGroup(INT_eFuseBB_XINT_INTERRUPT_ACK_GROUP);
}

inline void CalculateAvgVStore(){

    static float avgValue = 0.0;
    static uint16_t avgVSotreCount = 0;

    if( sensorVector[VStoreIdx].convertedReady ) {
        if( avgVSotreCount < 10 ) {
            avgValue = avgValue + sensorVector[VStoreIdx].realValue;
            avgVSotreCount++;
        } else {
            DCDC_VI.avgVStore = avgValue  /  avgVSotreCount;
            avgVSotreCount = 0;
            avgValue =  0.0;
        }
    }
}


inline void CalculateAvgVBus(){

    static float avgValue = 0.0;
    static uint16_t avgVBusCount = 0;

    if( sensorVector[VBusIdx].convertedReady ) {
        if( avgVBusCount < 10 ) {
            avgValue = avgValue + sensorVector[VBusIdx].realValue;
            avgVBusCount++;
        } else {
            DCDC_VI.avgVBus = avgValue  /  avgVBusCount;
            avgVBusCount = 0;
            avgValue = 0;
        }
    }
}


void EnableOrDisblePWM()
{
    static bool PWMDCDCAlreadyStartedFlag = false;
    static bool PWMDCDCAlreadyStoppedFlag = false;

    if (DCDC_VI.I_Ref_Real > 0.01)
    {
        if (!PWMDCDCAlreadyStartedFlag)
        {
            HAL_StartPwmDCDC();
            PWMDCDCAlreadyStartedFlag = true;
            PWMDCDCAlreadyStoppedFlag = false;
        }
    }
    else
    {
        if (!PWMDCDCAlreadyStoppedFlag)
        {
            HAL_StopPwmDCDC();
            PWMDCDCAlreadyStoppedFlag = true;
            PWMDCDCAlreadyStartedFlag = false;
        }
    }
}

void CheckMainStateMachineIsRunning()
{
    static uint32_t stateMachineLastCount = 1;
    static uint32_t stateMachineStoppedCounter = 0;
    if ( (timer_get_ticks() % 100)==0 )
    {
        if (CounterGroup.StateMachineCounter == stateMachineLastCount)
        {
            ADC_clearInterruptStatus(ADCB_BASE, ADC_INT_NUMBER4);
            Interrupt_clearACKGroup( INT_ADCINB_4_INTERRUPT_ACK_GROUP );
            stateMachineStoppedCounter++;
        }
        stateMachineLastCount = CounterGroup.StateMachineCounter;
    }
}


/* CPU1 can start the inrush current limiter,
 * but not stop it
 */
//void start_inrush_current_limiter(uint8_t state)
//{
//    /* if not running
//     * in the meaning of not incrementing the duty cycle
//     * */
//    if(false == run_inrush_current_limiter)
//    {
//        HAL_PWM_setCounterCompareValue(InrushCurrentLimit_BASE, EPWM_COUNTER_COMPARE_A, 0);
//
//        if(state)
//            run_inrush_current_limiter = true;
//    }
//}


//void soft_start(void)
//{
//    /* do not run function again until command from CPU1 */
//    run_inrush_current_limiter = false;
//
//    /* signal to CPU1 the inrush current limiter is done */
////    IPC_setFlagLtoR(IPC_CPU2_L_CPU1_R, IPC_SWITCHES_QIRS_DONE);
//}

//void soft_start_old(void)
//{
//    static uint32_t time_start;
//    static uint32_t time_stop;
//    uint16_t InrushPWMCompareAValue =
//            HAL_PWM_getCounterCompareValue( InrushCurrentLimit_BASE, EPWM_COUNTER_COMPARE_A);
//    uint16_t InrushPWMCompareAValueLimit =
//            HAL_EPWM_getTimeBasePeriod(InrushCurrentLimit_BASE);
//
//    /* started by IPC flag set by CPU1 */
//    if(true == run_inrush_current_limiter)
//    {
//        /* is inrush current limiter started */
//        if(0 == InrushPWMCompareAValue)
//        {
//            // Initialize and start Inrush PWM
//            HAL_PWM_setCounterCompareValue(InrushCurrentLimit_BASE,
//                                           EPWM_COUNTER_COMPARE_A, 5);
//            HAL_StartPwmInrushCurrentLimit();
//
//            time_start = timer_get_ticks();
//        }
//        else
//        {
//            /* is delay expired */
//            if (CounterGroup.InrushCurrentLimiterCounter > DELAY_100_SM_CYCLES)
//            {
//                /* Intensify in-rush current gradually
//                 * increasing In-rush PWM duty cycle
//                 */
//                InrushPWMCompareAValue += 5;
//
//                /* is PWM duty cycle limit reached */
//                if(InrushPWMCompareAValue > InrushPWMCompareAValueLimit)
//                {
//                    /* limit the duty cycle
//                     * _must_ be +1 to get true 100% */
//                    HAL_PWM_setCounterCompareValue(InrushCurrentLimit_BASE, EPWM_COUNTER_COMPARE_A, InrushPWMCompareAValueLimit + 1);
//
//                    time_stop = timer_get_ticks();
//                    uint32_t time_total = (time_stop - time_start);
//                    PRINT("IRCL total time: %ld [ms]\r\n", time_total);
//                    //TODO race condition with CPU1
//
//                    /* do not run function again until command from CPU1 */
//                    run_inrush_current_limiter = false;
//
//                    /* signal to CPU1 the inrush current limiter is done */
//                    IPC_setFlagLtoR(IPC_CPU2_L_CPU1_R, IPC_SWITCHES_QIRS_DONE);
//                }
//                else
//                {
//                    HAL_PWM_setCounterCompareValue(InrushCurrentLimit_BASE, EPWM_COUNTER_COMPARE_A, InrushPWMCompareAValue);
//                }
//
//                /* reset delay */
//                CounterGroup.InrushCurrentLimiterCounter = 0;
//            }
//            else
//            {
//                /* increment delay */
//                CounterGroup.InrushCurrentLimiterCounter++;
//            }
//        }
//    }
//}
