/**
 ******************************************************************************
 * @file    State_Machine.h
 * @brief   This file contains all the function prototypes for
 *          the State_Machine.c file
 ******************************************************************************
 *          Created on: 8 Dec 2022
 *          Author: Luyu Wang
 *
 ******************************************************************************
 */

#ifndef APP_INC_STATE_MACHINE_H_
#define APP_INC_STATE_MACHINE_H_


#include <math.h>

#include "main.h"
#include "board.h"

#define DELAY_50_SM_CYCLES 50
#define DELAY_100_SM_CYCLES 100
#define MAX_INRUSH_DUTY_CYCLE 1.0
#define INRUSH_DUTY_CYLE_INCREMENT 5

enum Operating_state
{
    Idle = 0,
    Initialize,             // 1
    SoftstartInitDefault,              // 2
    SoftstartInitRedundant = 201,
    Softstart = 3,                  // 3
    TrickleChargeInit,          // 4
    TrickleChargeDelay,         // 5
    TrickleCharge,              // 6
    ChargeInit,                 // 7
    Charge,                     // 8
    ChargeStop,                 // 9
    ChargeConstantVoltageInit,  // 10
    ChargeConstantVoltage,      // 11
    RegulateInit,               // 12
    Regulate,                   // 13
    RegulateStop,               // 14
    RegulateVoltageInit = 140,
    RegulateVoltage = 141,
    RegulateVoltageStop = 142,
    Fault = 15,                      // 15
    FaultDelay,                 // 16
    Keep,                       // 17
    BalancingInit,              // 18
    Balancing,                  // 19
    CC_Charge,                  // 20
    StopEPWMs,                  // 21
    ChargeRamp,                 // 22
};

typedef enum Balancing_state
{
    BALANCE_INIT = 0,
    BALANCE_VERIFY_THRESHOLD,
    BALANCE_CONNECT,
    BALANCE_DISCHARGE,
    BALANCE_READ_CELL_VOLTAGES,
    BALANCE_NEXT_ITERATION,
    BALANCE_DONE,
}Balancing_state_t;

typedef enum Read_cell_state
{
    READ_CELL_INIT = 0,
    READ_CELL_CONNECT,
    READ_CELL_CONNECT_DELAY,
    READ_CELL_VALUE,
    READ_NEXT_CELL,
    READ_NEXT_ITERATION,
    READ_CELL_DONE,
}Read_cell_state_t;



void StateMachineInit(void);
void StateMachine(void);
void CalculateAvgVStore();
void CalculateAvgVBus();
void CheckMainStateMachineIsRunning();
void VerifyDPMUSwitchesOK(void);
bool DPMUInitialized();

#endif /* APP_INC_STATE_MACHINE_H_ */
