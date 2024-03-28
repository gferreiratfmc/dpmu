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

//void ApplyPhaseShift(uint32_t PWMBase, uint16_t Period, float Phase);
//uint16_t ConvertCurent(uint16_t ADRawvalue,uint16_t Power_direction,uint16_t offset);
void CLLC_h_to_l_1(void);
void CLLC_h_to_l_2(void);
void CLLC_cell_discharge_1(void);
void CLLC_cell_discharge_2(void);
void ClearIntegrationErrorCellDischarge1(void);
void ClearIntegrationErrorCellDischarge2(void);


int16_t convert_current(uint16_t AdRawValue, uint16_t PowerDirection,
                              uint16_t offset);
PiOutput_t Pi_Controller(PI_Parameters_t PI, PiOutput_t PIout, uint16_t Ref, uint16_t ADCReading);
extern PI_Parameters_t CellDischargePiParameter;

#endif /* APP_INC_CLLC_H_ */
