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
#include "common.h"
#include "DCDC.h"
#include "debug_log.h"
#include "energy_storage.h"
#include "error_handling.h"
#include "error_handling_CPU2.h"
#include "GlobalV.h"
#include "hal.h"
#include "sensors.h"
#include "shared_variables.h"
#include "state_machine.h"
#include "switches.h"
#include "switch_matrix.h"
#include "timer.h"

#define TRACK_STATE_BUFFER_SIZE 12

bool DPMUInitializedFlag = false;
bool DPMUSwitchesStatesOK = false;
extern bool updateCPU2FirmwareFlag;

uint16_t trackingStates[TRACK_STATE_BUFFER_SIZE];
uint16_t trackStatesCount = 0;
extern uint16_t PhaseshiftCount;
uint16_t TestCellNr = 1;

Counters_t CounterGroup = { 0 };

extern bool test_update_of_error_codes;

extern float cellVoltagesVector[30];
float energyBankVoltage;
bool cellVoltageOverThreshold;


void CheckCommandFromIOP(void);
void DefineDPMUSafeState(void);
int DoneWithInrush(void);
void TrackStatesForDEBUG(void);
void EnableOrDisblePWM();
uint16_t ConvCellNrToIdx(uint16_t celNr);
inline void EnableEFuseBBToStopDCDC_EPWM();
void update_debug_log(void);

void StateMachine(void)
{
    static bool cellVoltageOverThreshold = false;
    static float I_Ref_Real_Final = 0.0;
    static bool EPMWStarted = false;

    ConvertCountsToRealOfAllSensors();
    CalculateAvgVStore();
    CalculateAvgVBus();

    switch (StateVector.State_Current)
    {
        case PreInitialized:
            break;

        case Initialize: /* Initial state, initialization of the parameters*/
            if( !DPMUInitializedFlag && (sharedVars_cpu1toCpu2.DPMUAppInfoInitializedFlag == true) ) {
                if( CalibrateZeroVoltageOffsetOfSensors() ) {
                    test_update_of_error_codes = true;
                    HAL_DcdcNormalModePwmSetting();

                    switches_Qinb( SW_OFF );
                    switches_Qlb( SW_OFF );
                    switches_Qsb( SW_OFF );

                    if( sharedVars_cpu1toCpu2.dpmu_default_flag == true ) {
                        StateVector.State_Next = SoftstartInitDefault;
                    } else {
                        StateVector.State_Next = SoftstartInitRedundant;
                    }
                }
            } else {
                StateVector.State_Next = StateVector.State_Before;
            }

            break;

        case SoftstartInitDefault:
            //Close output Switch
            switches_Qlb(SW_ON);
            switches_Qsb(SW_ON);

            // Initialize and start Inrush PWM
            HAL_PWM_setCounterCompareValue(InrushCurrentLimit_BASE, EPWM_COUNTER_COMPARE_A, INRUSH_DUTY_CYLE_INCREMENT);
            HAL_StartPwmInrushCurrentLimit();
            CounterGroup.InrushCurrentLimiterCounter = 0;
            StateVector.State_Next = Softstart;
            break;

        case SoftstartInitRedundant:
            //Opens output Switch
            switches_Qlb(SW_OFF);
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
                switches_Qsb(SW_ON);
                //Stop In-rush PWM
                HAL_PWM_setCounterCompareValue(InrushCurrentLimit_BASE, EPWM_COUNTER_COMPARE_A, 1);
                HAL_StopPwmInrushCurrentLimit();
                DPMUInitializedFlag = true;
                //EnableContinuousReadCellVoltages();
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

            HAL_DcdcNormalModePwmSetting();
            ILoop_PiOutput.Int_out = 0;

            I_Ref_Real_Final = 0.5 * (  DCDC_VI.target_Voltage_At_DCBus * DCDC_VI.iIn_limit / energy_bank_settings.max_voltage_applied_to_energy_bank );

            if( I_Ref_Real_Final >  MAX_INDUCTOR_BUCK_CURRENT ) {
                I_Ref_Real_Final = MAX_INDUCTOR_BUCK_CURRENT;
            }
            DCDC_VI.I_Ref_Real = I_Ref_Real_Final / 100.0;

            HAL_PWM_setCounterCompareValue( BEG_1_2_BASE, EPWM_COUNTER_COMPARE_A, 0.5*EPWM_getTimeBasePeriod(BEG_1_2_BASE) );
            EPMWStarted = false;
            DCDC_VI.counter = 0;

            EnableContinuousReadCellVoltages();
            StateVector.State_Next = ChargeRamp;

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

            if( DCDC_VI.avgVStore < energy_bank_settings.max_voltage_applied_to_energy_bank * MAX_ENERGY_BANK_VOLTAGE_RATIO ) {
                DCDC_current_buck_loop_float();
            } else {
                StateVector.State_Next = ChargeStop;
            }

            if( ReadCellVoltagesDone() == true) {
                if( cellVoltageOverThreshold == true ) {
                    StateVector.State_Next = BalancingInit; //Normal operation
    //                StateVector.State_Next = ChargeStop;   // Endurance operation. Balancing not completely implemented
                }
            }

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

            if( sensorVector[ISen2fIdx].realValue < 0.25 ) {
                HAL_StopPwmDCDC();
                StateVector.State_Next = Balancing;
            }
            break;

        case Balancing: // Balancing state supposed to run in parallel with Charge state.

            if( BalancingAllCells( &cellVoltagesVector[0] ) == true ) {
               StateVector.State_Next = ChargeInit;
            }
            break;

        case BalancingStop:
            switch_matrix_reset();
            StateVector.State_Next = StopEPWMs;
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
            //TODO: Commented only for endurance. Verify if it's should be uncommented for production version.
//            if( DCDC_VI.avgVBus < REG_MIN_DC_BUS_VOLTAGE_RATIO * DCDC_VI.target_Voltage_At_DCBus ) {
//                if( sensorVector[ISen1fIdx].realValue >= MIN_OUTPUT_CURRENT_TO_REGULATE_VOLTAGE ) {
//                        StateVector.State_Next = RegulateVoltageInit;
//                }
//            }
            break;

        case RegulateStop:
            DCDC_VI.I_Ref_Real = 0.0;
            DCDC_current_boost_loop_float();
            if( sensorVector[ISen2fIdx].realValue < 0.25 ) {
                StateVector.State_Next = StopEPWMs;
            }
            break;

        case RegulateVoltageInit:
//            DCDC_VI.I_Ref_Real = 0.0;
//            DCDC_current_boost_loop_float();
            switches_Qinb( SW_OFF );
            DCDCInitializePWMForRegulateVoltage();
            StateVector.State_Next = RegulateVoltage;
            break;

        case RegulateVoltage:
            DCDC_voltage_boost_loop_float();
            if( DCDC_VI.avgVStore < energy_bank_settings.min_voltage_applied_to_energy_bank ) {
                StateVector.State_Next = RegulateVoltageStop;

            }
            if(  DCDC_VI.avgVBus > sharedVars_cpu1toCpu2.max_allowed_dc_bus_voltage ) {
                StateVector.State_Next = RegulateVoltageStop;
            }
            break;

        case RegulateVoltageStop:
            DPMUInitializedFlag = false;
            StateVector.State_Next = StopEPWMs;
            break;

        case Fault:
            HAL_StopPwmDCDC();

            switches_Qlb( SW_OFF );
            switches_Qsb( SW_OFF );
            switches_Qinb( SW_OFF );

            CounterGroup.PrestateCounter = 0;

            test_update_of_error_codes = false;

            DPMUInitializedFlag = false;

            StopAllEPWMs();

            ResetDpmuErrorOcurred();

            StateVector.State_Next = PreInitialized;

            break;

        case StopEPWMs:

            StopAllEPWMs();
            StateVector.State_Next = Idle;
            break;

        case Idle:
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



    if( !DPMUSwitchesStatesOK ) {
        DefineDPMUSafeState();
    }
    //TrackStatesForDEBUG();
    /* print next state then state changes */
    if(StateVector.State_Current  != StateVector.State_Next) {
        ForceUpdateDebugLog();
        PRINT("Next state %02d\r\n", StateVector.State_Next);
    }

    if( DpmuErrorOcurred() == true ) {
        StateVector.State_Next = Fault;
    }

    /* update current state */
    StateVector.State_Before = StateVector.State_Current;
    StateVector.State_Current = StateVector.State_Next;



    /* reflect state in CANopen OD */
//    sharedVars_cpu2toCpu1.current_state = convert_current_state_to_OD(StateVector.State_Current);
    sharedVars_cpu2toCpu1.current_state = StateVector.State_Current;

    CounterGroup.StateMachineCounter++;
}


void VerifyDPMUSwitchesOK(void) {

    if( sharedVars_cpu1toCpu2.dpmu_default_flag == true ) {
        if( ( GPIO_readPin(Qinb) == SW_ON ) &&
            ( GPIO_readPin(Qlb) == SW_ON ) &&
            ( GPIO_readPin(Qsb) == SW_ON ) ) {
            DPMUSwitchesStatesOK = true;
        } else {
            DPMUSwitchesStatesOK = false;
        }
    } else {
        if( ( GPIO_readPin(Qinb) == SW_ON ) &&
            ( GPIO_readPin(Qlb) == SW_OFF ) &&
            ( GPIO_readPin(Qsb) == SW_ON ) ) {
            DPMUSwitchesStatesOK = true;
        } else {
            DPMUSwitchesStatesOK = false;
        }
    }
}

void DefineDPMUSafeState( void ) {

    switch( StateVector.State_Current ) {
        case Idle:
            DPMUInitializedFlag = false;
            break;
        case TrickleChargeInit:
        case TrickleChargeDelay:
        case TrickleCharge:
        case ChargeInit:
        case ChargeRamp:
        case Charge:
            DPMUInitializedFlag = false;
            StateVector.State_Next = ChargeStop;
            break;

        case RegulateInit:
        case Regulate:
            DPMUInitializedFlag = false;
            StateVector.State_Next = RegulateStop;
            break;

        default:
            break;
    }
}

bool DPMUInitialized() {
    return DPMUInitializedFlag;
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
    StateVector.State_Before = PreInitialized;
    StateVector.State_Current = PreInitialized;
    StateVector.State_Next = PreInitialized;

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
        if ( !DPMUInitializedFlag ) {
            if( StateVector.State_Current == PreInitialized ) {
                    if( StateVector.State_Next != Initialize ) {
                        StateVector.State_Next = PreInitialized;
                    }
                } else {
                    StateVector.State_Next = StateVector.State_Current;
                }
        } else {
            switch( StateVector.State_Next )
            {
                case Idle:
                    switch ( StateVector.State_Current )
                    {
                        case PreInitialized:
                            StateVector.State_Next = PreInitialized;
                            break;
                        case Initialize:
                              StateVector.State_Next = StateVector.State_Current;
                              break;
                        case Idle:
                            /* No change if it is in Idle already */
                            break;
                        case RegulateInit:
                        case Regulate:
                            StateVector.State_Next = RegulateStop;
                            break;
                        case RegulateVoltageInit:
                        case RegulateVoltage:
                            StateVector.State_Next = RegulateVoltageStop;
                            break;

                        case TrickleCharge:
                        case TrickleChargeDelay:
                        case TrickleChargeInit:
                        case ChargeInit:
                        case Charge:
                            StateVector.State_Next = ChargeStop;
                            break;

                        case Balancing:
                        case BalancingInit:
                            StateVector.State_Next = BalancingStop;
                            break;

                        default:
                            StateVector.State_Next = StopEPWMs;
                            break;
                    }
                    break;

                case Initialize:
                    StateVector.State_Next = StateVector.State_Current;
                    break;
                case TrickleChargeInit:
                case RegulateInit:
                    if( StateVector.State_Current != Idle ) {
                        StateVector.State_Next = StateVector.State_Current;
                    }
                    break;
                case Fault:
                    if( StateVector.State_Current == PreInitialized ) {
                        StateVector.State_Next = PreInitialized;
                    }
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

    VerifyDPMUSwitchesOK();
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


