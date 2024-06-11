/*
 ******************************************************************************
 * @file    CLLC.c
 * @brief   This file provides code for the Frequency Control
 *          of the CLLC Converter.

 ******************************************************************************
 *          Created on: 24 January 2023
 *          Author: Luyu Wang
 *
 ******************************************************* ***********************
 */

#include "common.h"
#include "CLLC.h"
#include "cli_cpu2.h"
#include "GlobalV.h"
#include "hal.h"
#include "sensors.h"
#include "switch_matrix.h"
#include "timer.h"



int16_t PhaseshiftCount = 0;
int16_t Phaseshift[60];
int16_t Phasepointer = 0;
extern uint16_t PhaseShiftCount;
uint16_t Switchthing = 0;

int16_t CurrentError = 0;
PI_Parameters_t CellDischargePiParameter = { 0.0867, 9.09, 1, 0 };
float CLLC_Discharge_I_Ref = 1.0;
PiOutput_t CllcPIout = {0};

void StartCllcControlLoop(uint16_t cellNr)
{
    CllcPIout.Int_out = 0.0;
    CllcPIout.Output = 0.0;
    CllcPIout.calculated_error = 0.0;
    CllcPIout.dutyCycle = 0;

    if (cellNr <= BAT_15)
    {
        HAL_StopPwmCllcCellDischarge2();
        HAL_StartPwmCllcCellDischarge1();
    }
    else
    {
        HAL_StopPwmCllcCellDischarge1();
        HAL_StartPwmCllcCellDischarge2();
    }
}

void StopCllcControlLoop()
{
    HAL_StopPwmCllcCellDischarge2();
    HAL_StopPwmCllcCellDischarge1();
}


void CllcControlLoop(uint16_t cellNr ) {


    uint16_t PhaseshiftCount = 0;


    if( cellNr <= BAT_15 ) {

        CllcPIout = Pi_ControllerCllCFloat( CellDischargePiParameter,
                                            CllcPIout,
                                            CLLC_Discharge_I_Ref,
                                            -sensorVector[I_Dab2fIdx].realValue );
        PhaseshiftCount = CLLC_PHASE_180 - CllcPIout.Output;
        HAL_PWM_setPhaseShift(QABPWM_6_7_BASE, PhaseshiftCount);

    } else {

        CllcPIout = Pi_ControllerCllCFloat( CellDischargePiParameter,
                                            CllcPIout,
                                            CLLC_Discharge_I_Ref,
                                            -sensorVector[I_Dab3fIdx].realValue );
        PhaseshiftCount = CLLC_PHASE_180 - CllcPIout.Output;
        HAL_PWM_setPhaseShift(QABPWM_14_15_BASE, PhaseshiftCount);

    }
//    if( ( timer_get_ticks()%250 ) == 0) {
//        PRINT( "PhaseshiftCount[%3d], CllcPIout.calculated_error[%8.2f], [I_Dab2fIdx]:[%8.2f]\r\n", PhaseshiftCount, CllcPIout.calculated_error, -sensorVector[I_Dab2fIdx].realValue );
//    }
}

PiOutput_t Pi_ControllerCllCFloat(PI_Parameters_t PI, PiOutput_t PIout, float Ref, float ValueRead)
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


/* brief: Converter current of CLLC current measurement depends on the power direction
*
* details: Control discharge current according to Current reference
*
* requirements: none
*
* argument: none
*
* return: none
*
* note:
*
* presumptions:
*/
int16_t convert_current(uint16_t AdRawValue, uint16_t PowerDirection,
                        uint16_t offset)
{
    int16_t ConvertedCurrent;
    
    if(1 == PowerDirection)
    {
        ConvertedCurrent = (int16_t)(AdRawValue - offset);
    }

    if(0 == PowerDirection)
    {
        ConvertedCurrent = (int16_t)(offset - AdRawValue);
    }

    return ConvertedCurrent;
}

/* brief: Current control loop of cell discharge function of one of the CLLC converters
*
* details: Control discharge current according to Current reference
*
* requirements: none
*
* argument: none
*
* return: none
*
* note: If the current can not reach the current reference the phase shift will
*       be maximum (180 degree phase angle). When you discharge the cells, make
*       sure you monitor the cell voltage and discharge it just enough.
*
* presumptions:
*/
//void CLLC_cell_discharge_1(void)  // This function has been tested on P1 HW
//{
//    int16_t IDAB3_RAW = HAL_ADC_readResult(ADCDRESULT_BASE, ADC_SOC_NUMBER0);
//    CLLC_VI.IDAB2_RAW = convert_current(IDAB3_RAW, 1, CLLC_VI.CurrentOffset1);
//
//    //CellDischrgePiParameter is initialized in DCDC_converter_init at DCDC.c
//    PiOutputCellDischarge1 = Pi_Controller(CellDischargePiParameter,
//                                           PiOutputCellDischarge1,
//                                           CLLC_VI.I_Ref_Raw,
//                                           CLLC_VI.IDAB2_RAW);
//
//    PhaseshiftCount = (int)(196-PiOutputCellDischarge1.Output);
//    HAL_PWM_setPhaseShift(QABPWM_6_7_BASE, PhaseshiftCount);
//}

/* brief: Current control loop of cell discharge function of one of the CLLC converters
*
* details: Control discharge current according to Current reference
*
* requirements: none
*
* argument: none
*
* return: none
*
* note: If the current can not reach the current reference the phase shift will
*       be maximum (180 degree phase angle). When you discharge the cells, make
*       sure you monitor the cell voltage and discharge it just enough.
*
* presumptions:
*
*/
//void CLLC_cell_discharge_2(void)   // Only CLLC_cell_discharge_1 has been tested at P1 HW, please pay attention for CLLC_cell_discharge_2
//{
//    int16_t IDAB3_RAW = HAL_ADC_readResult(ADCDRESULT_BASE, ADC_SOC_NUMBER1);
//    CLLC_VI.IDAB3_RAW = convert_current(IDAB3_RAW, 1, CLLC_VI.CurrentOffset2);
//
//    //CellDischrgePiParameter   is initialized in DCDC_converter_init at DCDC.c
//    PiOutputCellDischarge2 = Pi_Controller(CellDischargePiParameter,
//                                               PiOutputCellDischarge2,
//                                               CLLC_VI.I_Ref_Raw,
//                                               CLLC_VI.IDAB3_RAW);
//
//    PhaseshiftCount = (int)(196-PiOutputCellDischarge1.Output);
//    HAL_PWM_setPhaseShift(QABPWM_14_15_BASE, PhaseshiftCount);
//}

/* brief: PI controller with  Anti-windup
*
* details:
*
* requirements: none
*
* argument: none
*
* return: PiOutput_t
*
* note: none
*
* presumptions:
*
*/
//PiOutput_t Pi_Controller(PI_Parameters_t PI, PiOutput_t PIout,
//                         uint16_t Ref, uint16_t ADCReading)
//{
//    int16_t error = Ref - ADCReading;
//    CurrentError = error;
//    float P_out   = (float) error * PI.Pgain;
//    PIout.Int_out = (float) error * PI.Igain + PIout.Int_out;
//    PIout.Output = P_out + PIout.Int_out;
//
////Anti-windup
//
//    if (PIout.Output > PI.UpperLimit)
//    {
//        PIout.Output = PI.UpperLimit;
//        PIout.Int_out = PI.UpperLimit - P_out;
//    }
//
//    if (PIout.Output < PI.LowerLimit)
//    {
//        PIout.Output = PI.LowerLimit;
//        PIout.Int_out = PI.LowerLimit - P_out;
//    }
//
//    return PIout;
//}

/* brief: reset integration error
*
* details:
*
* requirements: none
*
* argument: none
*
* return: none
*
* note: none
*
* presumptions:
*
*/
//void ClearIntegrationErrorCellDischarge1(void)
//{
//    PiOutputCellDischarge1.Int_out = 0;
//}

/* brief: reset integration error
*
* details:
*
* requirements: none
*
* argument: none
*
* return: none
*
* note: none
*
* presumptions:
*
*/
//void ClearIntegrationErrorCellDischarge2(void)
//{
//    PiOutputCellDischarge2.Int_out = 0;
//}
