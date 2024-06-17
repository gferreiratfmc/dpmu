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



typedef enum Balancing_state
{
    BALANCE_INIT = 0,
    BALANCE_VERIFY_HIGH_THRESHOLD,
    BALANCE_READ_CELL_VOLTAGES,
    BALANCE_CONNECT,
    BALANCE_DISCHARGE,
    BALANCE_DISCHARGE_READ_CELL_VOLTAGES,
    BALANCE_VERIFY_LOW_THRESHOLD,
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

typedef struct Counters
{
    uint32_t StateMachineCounter;
    int16_t PrestateCounter;
    uint16_t InrushCurrentLimiterCounter;
} Counters_t;

extern Counters_t CounterGroup ;

void StateMachineInit(void);
void StateMachine(void);
void CalculateAvgVStore();
void CalculateAvgVBus();
void CheckMainStateMachineIsRunning();
void VerifyDPMUSwitchesOK(void);
bool DPMUInitialized();

#endif /* APP_INC_STATE_MACHINE_H_ */
