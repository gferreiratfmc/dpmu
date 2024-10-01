/**
  ******************************************************************************
  * @file    DCDC.h
  * @brief   This file contains all the function prototypes for
  *          the CLLC.c file
  ******************************************************************************
  *          Created on: 24 Jan 2023
  *          Author: Luyu Wang
  *
  ******************************************************************************
 */

#ifndef APP_INC_CLLC_H_
#define APP_INC_CLLC_H_


#include <math.h>
#include <stdint.h>

#include "board.h"
#include "GlobalV.h"
#include "main.h"


#define CLLC_TIME_BASE_PERIOD 386
#define CLLC_PHASE_180 (CLLC_TIME_BASE_PERIOD/2)


void StartCllcControlLoop(uint16_t cellNr);
void CllcControlLoop(uint16_t cellNr );
void StopCllcControlLoop();
PiOutput_t Pi_ControllerCllCFloat(PI_Parameters_t PI, PiOutput_t PIout, float Ref, float ValueRead);
extern PI_Parameters_t CellDischargePiParameter;


#endif /* APP_INC_CLLC_H_ */
