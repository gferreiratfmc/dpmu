/*
 ******************************************************************************
 * @file    DCDC.c
 * @brief   This file provides code for the PWM Control
 *          of the bidirectional DCDC Converter.

 ******************************************************************************
 *          Created on: 5 Dec 2022
 *          Author: Luyu Wang
 *
 ******************************************************************************
 */
#include <stdint.h>

#include "board.h"
#include "CLLC.h"
#include "DCDC.h"
#include "energy_storage.h"
#include "GlobalV.h"
#include "hal.h"
#include "sensors.h"
#include "state_machine.h"
#include "switches.h"

PiOutput_t VLoop_PiOutput = { 0 };
PiOutput_t ILoop_PiOutput = { 0 };

static PI_Parameters_t ILoopParamBuck = { 0 };
static PI_Parameters_t VLoopParamBoost = { 0 };
static PI_Parameters_t ILoopParamBoost = { 0 };

volatile bool eFuseInputCurrentOcurred = false;
volatile bool eFuseBuckBoostOcurred = false;

int Count = 1;

uint16_t trickleChargeRangeState = 0;
float trickleChargeRangeLevel[] = { 0.25, 0.5, 0.75, 1.0 };
float trickleChargeDutyCycle[] =  { 0.005, 0.01, 0.015, 0.02 };


float I_IN_LIMIT_RATE = 0.8;
uint16_t NUMBER_OF_CURRENT_SAMPLES = 25;

float trackSensorBuffer1[TRACK_SENSOR_BUFFER_SIZE];
float trackSensorBuffer2[TRACK_SENSOR_BUFFER_SIZE];
float trackSensorBuffer3[TRACK_SENSOR_BUFFER_SIZE];
float trackSensorBuffer4[TRACK_SENSOR_BUFFER_SIZE];

uint16_t trackSensorBufferCount = 0;

uint32_t INT_BEG_1_2Counter = 0;
uint32_t INT_BEG_1_2CounterSW = 0;
uint32_t INT_BEG_1_2CounterHW = 0;
uint32_t INT_GLOAD_4_3Counter = 0;



/**
 * @brief  Efuse buck-boost over-current trip zone interrupt
 */
//__interrupt void INT_BEG_1_2_TZ_ISR(void) {
__interrupt void INT_eFuseBB_XINT_ISR(void) {
        INT_BEG_1_2Counter++;
        eFuseBuckBoostOcurred = true;
        //Interrupt_clearACKGroup( INT_eFuseBB_XINT_INTERRUPT_ACK_GROUP );
}

/**
 * @brief  Efuse input over-current trip zone interrupt
 */
__interrupt void INT_GLOAD_4_3_TZ_ISR(void) {

    TurnOffGload_3();

    eFuseInputCurrentOcurred = true;

    INT_GLOAD_4_3Counter++;
    Interrupt_clearACKGroup( INT_GLOAD_4_3_TZ_INTERRUPT_ACK_GROUP );
}



void DCDC_current_buck_loop_float(void)
{
        uint16_t dutyCycle;

        float ISen2Float = sensorVector[ISen2fIdx].realValue;

        ILoop_PiOutput = Pi_ControllerBuckFloat( ILoopParamBuck,
                                                 ILoop_PiOutput,
                                                 -DCDC_VI.I_Ref_Real,
                                                 ISen2Float );

        //dutyCycle = BUCK_NORMAL_MODE_TIME_BASE_PERIOD - (BUCK_NORMAL_MODE_TIME_BASE_PERIOD * -ILoop_PiOutput.Output);
        dutyCycle = (BUCK_NORMAL_MODE_TIME_BASE_PERIOD * -ILoop_PiOutput.Output);

        HAL_PWM_setCounterCompareValue(BEG_1_2_BASE,
                                       EPWM_COUNTER_COMPARE_A,
                                       dutyCycle);

        ILoop_PiOutput.dutyCycle = dutyCycle;
}

void DCDC_voltage_boost_loop_float(void)
{
    uint16_t dutyCycle;

    VLoop_PiOutput = Pi_ControllerBoostFloat(VLoopParamBoost,
                                             VLoop_PiOutput,
                                             DCDC_VI.target_Voltage_At_DCBus * REG_TARGET_DC_BUS_VOLTAGE_RATIO,
                                             sensorVector[VBusIdx].realValue);
//    DCDC_VI.I_Ref_Real =  (VLoop_PiOutput.Output);

    dutyCycle = BOOST_TIME_BASE_PERIOD - (BOOST_TIME_BASE_PERIOD * VLoop_PiOutput.Output);

    HAL_PWM_setCounterCompareValue(BEG_1_2_BASE, EPWM_COUNTER_COMPARE_A, dutyCycle);

    VLoop_PiOutput.dutyCycle = dutyCycle;
}

void DCDCInitializePWMForRegulateVoltage()
{
    uint16_t dutyCycle;


    dutyCycle = BOOST_TIME_BASE_PERIOD * ( DCDC_VI.avgVStore / DCDC_VI.avgVBus );

    HAL_PWM_setCounterCompareValue(BEG_1_2_BASE, EPWM_COUNTER_COMPARE_A, dutyCycle);
    HAL_StartPwmDCDC();
}

void DCDC_current_boost_loop_float(void)
{

    uint16_t dutyCycle;

    ILoop_PiOutput = Pi_ControllerBoostFloat(ILoopParamBoost,
                                             ILoop_PiOutput,
                                             DCDC_VI.I_Ref_Real,
                                             sensorVector[ISen2fIdx].realValue);

    dutyCycle = BOOST_TIME_BASE_PERIOD - (BOOST_TIME_BASE_PERIOD * ILoop_PiOutput.Output);

    HAL_PWM_setCounterCompareValue(BEG_1_2_BASE, EPWM_COUNTER_COMPARE_A, dutyCycle);

    ILoop_PiOutput.dutyCycle = dutyCycle;
}

bool calculate_boost_current(void)
{
    bool retValue = false;
    float BoostGain;
    static float sumInputCurrent=0;
    static float sumOutputCurrent=0;
    static float sumVStore=0;
    static float sumVbus=0;

    static uint16_t currentSampleCount = 0;

    if( currentSampleCount < NUMBER_OF_CURRENT_SAMPLES ){
        sumInputCurrent = sumInputCurrent + sensorVector[IF_1fIdx].realValue;
        sumOutputCurrent = sumOutputCurrent + sensorVector[ISen1fIdx].realValue;
        sumVbus = sumVbus + sensorVector[VBusIdx].realValue;
        sumVStore = sumVStore + sensorVector[VStoreIdx].realValue;
        currentSampleCount++;
        return false;
    } else {
        DCDC_VI.RegulateAvgInputCurrent = sumInputCurrent / NUMBER_OF_CURRENT_SAMPLES;
        DCDC_VI.RegulateAvgOutputCurrent = sumOutputCurrent / NUMBER_OF_CURRENT_SAMPLES;
        DCDC_VI.RegulateAvgVbus = sumVbus / NUMBER_OF_CURRENT_SAMPLES;
        DCDC_VI.RegulateAvgVStore = sumVStore / NUMBER_OF_CURRENT_SAMPLES;

        sumInputCurrent=0;
        sumOutputCurrent=0;
        sumVStore=0;
        sumVbus=0;

        currentSampleCount = 0;
    }

    //if( sensorVector[VStoreIdx].realValue > 30.0 ) {
    if( DCDC_VI.RegulateAvgVStore > 0.0 ) {
            //BoostGain =  sensorVector[VBusIdx].realValue / ( sensorVector[VStoreIdx].realValue );
            BoostGain =  DCDC_VI.RegulateAvgVbus / DCDC_VI.RegulateAvgVStore;
        if( BoostGain > 6.0 ) {
            BoostGain = 6.0;
        }
    } else {
        BoostGain = 0.0;
    }

    if( DCDC_VI.RegulateAvgOutputCurrent <= I_IN_LIMIT_RATE * DCDC_VI.iIn_limit ) {
        DCDC_VI.I_Ref_Real = 0.0;
        retValue = false;
    } else {
        DCDC_VI.I_Ref_Real = ( DCDC_VI.RegulateAvgOutputCurrent - ( I_IN_LIMIT_RATE * DCDC_VI.iIn_limit ) ) * BoostGain;
        retValue = true;

        if( DCDC_VI.I_Ref_Real >  MAX_INDUCTOR_BOOST_CURRENT ) {
            DCDC_VI.I_Ref_Real =  MAX_INDUCTOR_BOOST_CURRENT;
        }
     }

    TrackSensorValueForDEBUG( DCDC_VI.RegulateAvgInputCurrent, DCDC_VI.RegulateAvgOutputCurrent, DCDC_VI.RegulateAvgVStore, DCDC_VI.RegulateAvgVbus );

    return retValue;
}




PiOutput_t Pi_ControllerBuckFloat(PI_Parameters_t PI, PiOutput_t PIout,
                                  float Ref, float ValueRead)
{
    volatile float error;
    error = Ref - ValueRead;

    float P_out   = (float) error * PI.Pgain;
    PIout.Int_out = (float) error * PI.Igain + PIout.Int_out;
    PIout.Output = P_out + PIout.Int_out;
    /**
     * @brief Anti-windup
     */

    if (PIout.Output > PI.UpperLimit)
    {
        PIout.Output = PI.UpperLimit;
        PIout.Int_out = PI.UpperLimit - P_out;
    }
    
    if (PIout.Output < PI.LowerLimit)
    {
        PIout.Output = PI.LowerLimit;
        PIout.Int_out = PI.LowerLimit + P_out;
    }

    PIout.calculated_error = error;

    return PIout;
}


PiOutput_t Pi_ControllerBoostFloat(PI_Parameters_t PI, PiOutput_t PIout,
                                   float Ref, float ValueRead)
{
    volatile float error;
    error = Ref - ValueRead;

    float P_out   = (float) error * PI.Pgain;
    PIout.Int_out = (float) error * PI.Igain + PIout.Int_out;
    PIout.Output = P_out + PIout.Int_out;
    /**
     * @brief Anti-windup
     */

    if (PIout.Output > PI.UpperLimit)
    {
        PIout.Output = PI.UpperLimit;
        PIout.Int_out = PI.UpperLimit - P_out;
    }
    
    if (PIout.Output < PI.LowerLimit)
    {
        PIout.Output = PI.LowerLimit;
        PIout.Int_out = PI.LowerLimit + P_out;
    }

    PIout.calculated_error = error;

    return PIout;
}


void DCDCConverterInit(void)
{

    /* Stop all PWMs at initialization*/

    HAL_StopPwmCllcCellDischarge1();
    HAL_StopPwmCllcCellDischarge2();
    HAL_StopPwmInrushCurrentLimit();

    eFuseBuckBoostOcurred = false;
    eFuseInputCurrentOcurred = false;


    ILoopParamBuck.Pgain = 0.002;
    ILoopParamBuck.Igain = 0.00167256;
    ILoopParamBuck.UpperLimit = -0.05;
    ILoopParamBuck.LowerLimit = -0.95 ;

    ILoopParamBoost.Pgain = 0.002;
    ILoopParamBoost.Igain = 0.00167256;
    ILoopParamBoost.UpperLimit = 0.99;
    ILoopParamBoost.LowerLimit = 0.01;

    /*Init Voltage boost Loop PI parameters */
    VLoopParamBoost.Pgain = 0.8761f * 0.0050354f;         /* 100*3.3/2^16            */
    VLoopParamBoost.Igain = 57.55f * 0.00000050354f ;     /* 100*3.3/2^16 * 1/10000  */
    //VLoopParamBoost.UpperLimit = 19.0;
    VLoopParamBoost.UpperLimit = 0.83;
    VLoopParamBoost.LowerLimit = 0.1;

    /*Init Counters */

    CounterGroup.InrushCurrentLimiterCounter = 0;
    CounterGroup.PrestateCounter = 0;
    CounterGroup.StateMachineCounter = 0;

    CellDischargePiParameter.Pgain = 0.0077747;
    CellDischargePiParameter.Igain = 0.0046648;
    CellDischargePiParameter.LowerLimit = CLLC_PHASE_180*0.02;
    CellDischargePiParameter.UpperLimit = CLLC_PHASE_180*0.98;

    DCDC_VI.I_Ref_Real = 0.50;

}


void StopAllEPWMs(void)
{
    HAL_StopPwmDCDC();
    HAL_StopPwmCllcCellDischarge1();
    HAL_StopPwmCllcCellDischarge2();
//    HAL_StopPwmInrushCurrentLimit();
    HAL_PWM_setCounterCompareValue(InrushCurrentLimit_BASE,
                                       EPWM_COUNTER_COMPARE_A, 0); /* last one to turn off, least impact */
}


void ResetPulseStateAdjust() {
    trickleChargeRangeState = 0;
}

void AdjustPulseBasedOnSupercapVoltage( void ) {

    static float trickChargeDutyCycle = 0.005;


    switch( trickleChargeRangeState ) {

        case 0:
            trickChargeDutyCycle = trickleChargeDutyCycle[0] * EPWM_getTimeBasePeriod(BEG_1_2_BASE);
            HAL_PWM_setCounterCompareValue(BEG_1_2_BASE, EPWM_COUNTER_COMPARE_A, trickChargeDutyCycle );
            trickleChargeRangeState = 1;
            break;

        case 1:
            if ( DCDC_VI.avgVStore > trickleChargeRangeLevel[0] * energy_bank_settings.preconditional_threshold )
            {
                trickleChargeRangeState = 2;
                trickChargeDutyCycle = trickleChargeDutyCycle[1] * EPWM_getTimeBasePeriod(BEG_1_2_BASE);
                HAL_PWM_setCounterCompareValue(BEG_1_2_BASE, EPWM_COUNTER_COMPARE_A, trickChargeDutyCycle );
            }
            break;

        case 2:
            if ( DCDC_VI.avgVStore > trickleChargeRangeLevel[1] * energy_bank_settings.preconditional_threshold )
            {
                trickleChargeRangeState = 3;
                trickChargeDutyCycle = trickleChargeDutyCycle[2] * EPWM_getTimeBasePeriod(BEG_1_2_BASE);
                HAL_PWM_setCounterCompareValue(BEG_1_2_BASE, EPWM_COUNTER_COMPARE_A, trickChargeDutyCycle );
            }
            break;

        case 3:
            if ( DCDC_VI.avgVStore > trickleChargeRangeLevel[2] * energy_bank_settings.preconditional_threshold )
            {
                trickleChargeRangeState = 4;
                trickChargeDutyCycle = trickleChargeDutyCycle[3] * EPWM_getTimeBasePeriod(BEG_1_2_BASE);
                HAL_PWM_setCounterCompareValue(BEG_1_2_BASE, EPWM_COUNTER_COMPARE_A, trickChargeDutyCycle );
            }
            break;

        case 4:
            default:

            break;
    }
}

void IncreasePulseStateDutyToSuperCapsVoltageOri( void )
{
    static float trickChargeDutyCycle = 0.00;

    if( DCDC_VI.avgVStore <= trickleChargeRangeLevel[trickleChargeRangeState] * energy_bank_settings.preconditional_threshold  ) {

        trickChargeDutyCycle = trickleChargeDutyCycle[0]; //0.005

    } else if (DCDC_VI.avgVStore > trickleChargeRangeLevel[0] * energy_bank_settings.preconditional_threshold
            && ( DCDC_VI.avgVStore < trickleChargeRangeLevel[1] * energy_bank_settings.preconditional_threshold ) ) {

        trickChargeDutyCycle = trickleChargeDutyCycle[1]; //0.01

    } else {

        trickChargeDutyCycle = trickleChargeDutyCycle[2]; //0.015;
    }

    HAL_PWM_setCounterCompareValue(BEG_1_2_BASE, EPWM_COUNTER_COMPARE_A, trickChargeDutyCycle * EPWM_getTimeBasePeriod(BEG_1_2_BASE) );
}

void TrackSensorValueForDEBUG(float sensorValue1, float sensorValue2, float sensorValue3, float sensorValue4)
{
    /*** For debuging purposes ***/

    trackSensorBuffer1[trackSensorBufferCount] = sensorValue1;
    trackSensorBuffer2[trackSensorBufferCount] = sensorValue2;
    trackSensorBuffer3[trackSensorBufferCount] = sensorValue3;
    trackSensorBuffer4[trackSensorBufferCount] = sensorValue4;
    trackSensorBufferCount++;
    if (trackSensorBufferCount == TRACK_SENSOR_BUFFER_SIZE)
    {
        trackSensorBufferCount=0;
    }
}

inline void handleEfuseVinOccurence()
{
    static uint16_t eFuseVinCounter = 0;
    static float avgInputCurrent = 0.0;

    if( eFuseVinCounter < 2) {
        avgInputCurrent = avgInputCurrent + sensorVector[IF_1fIdx].realValue;
        eFuseVinCounter++;
    } else {
        avgInputCurrent  = avgInputCurrent / eFuseVinCounter;
        if ( abs(avgInputCurrent) > MAXIMUM_INPUT_CURRENT ) {
            switches_Qinb(SW_OFF);
            efuse_top_half_flag = true;
            StateVector.State_Next = Fault;
        }
        eFuseVinCounter = 0;
        avgInputCurrent = 0.0;
        eFuseInputCurrentOcurred = false;
    }
}

inline void handleEFuseBBOccurence()
{
    static uint16_t eFuseBBCounter = 0;
    static float avgBBCurrent = 0.0;

    // Measure the current for 2 state machine cycles and then decides if it is a fault and not an overshoot
    if( eFuseBBCounter < 2) {
        avgBBCurrent = avgBBCurrent + sensorVector[ISen2fIdx].realValue;
        eFuseBBCounter++;
    } else {
        avgBBCurrent  = avgBBCurrent / eFuseBBCounter;
        //The average of the two current sample is above maximum limits, it is a FAULT.
        if ( abs(avgBBCurrent) > MAXIMUM_BB_CURRENT ) {
            HAL_StopPwmDCDC();
            //If generated by the eFuse changes the state to Fault
            StateVector.State_Next = Fault;
            INT_BEG_1_2CounterHW++;
            efuse_top_half_flag = true;
        }
        eFuseBBCounter = 0;
        avgBBCurrent = 0.0;
        eFuseBuckBoostOcurred = false;
    }
}
