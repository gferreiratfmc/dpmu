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

States_t StateVector =  {
                          .State_Before = PreInitialized,
                          .State_Current = PreInitialized,
                          .State_Next = PreInitialized,
                          .State_Before_Balancing = PreInitialized
                        };
