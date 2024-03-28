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


#include <GlobalV.h>
#include <math.h>
#include <stdint.h>

#include "board.h"
#include "main.h"

//void ApplyPhaseShift(uint32_t PWMBase, uint16_t Period, float Phase);
//uint16_t ConvertCurent(uint16_t ADRawvalue,uint16_t Power_direction,uint16_t offset);
void CLLC_h_to_l_1(void);
void CLLC_h_to_l_2(void);
void CLLC_cell_discharge_1(void);
void CLLC_cell_discharge_2(void);
void ClearIntegrationErrorCellDischarge1(void);
void ClearIntegrationErrorCellDischarge2(void);
void InitCellDischargePiParameter(void);
PiOutput Pi_Controller(PI_Parameters PI, PiOutput PIout, uint16_t Ref, uint16_t ADCReading);


#endif /* APP_INC_CLLC_H_ */
