/*
 ******************************************************************************
 * @file    CLLC.c
 * @brief   This file provides code for the Phase Control
 *          of the CLLC Converter.

 ******************************************************************************
 *          Created on: 13 Junho 2023
 *          Author: Gustavo Ferreira / Mauricio Dalla Vecchia
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
float CLLC_Discharge_I_Ref = 2.0;
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
