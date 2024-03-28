/*
 * GlobalV.c
 *
 *  Created on: 28 feb. 2023
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#include "GlobalV.h"
#include "main.h"

DCDC_Parameters_t DCDC_VI = { 0 };
CLLC_Parameters_t CLLC_VI = { 0 };
PiOutput_t VLoop_PiOutput = { 0 };
PiOutput_t ILoop_PiOutput = { 0 };
States_t StateVector = { 0 };
uint16_t T_delay = 1 ;
uint16_t CellVoltages[30];
Counters_t CounterGroup = { 0 };
uint16_t PhaseShiftCount = 0;
