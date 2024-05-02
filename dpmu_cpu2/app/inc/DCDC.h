/**
 ******************************************************************************
 * @file    DCDC.h
 * @brief   This file contains all the function prototypes for
 *          the DCDC.c file
 ******************************************************************************
 *          Created on: 5 Dec 2022
 *          Author: Luyu Wang
 *
 ******************************************************************************
 */

#ifndef APP_INC_DCDC_H_
#define APP_INC_DCDC_H_


#include <math.h>
#include <stdint.h>

#include "board.h"
#include "GlobalV.h"
#include "main.h"


#define TRACK_SENSOR_BUFFER_SIZE 128

#define MAXIMUM_INPUT_CURRENT 18.5
#define MAXIMUM_BB_CURRENT 18.5
#define REG_MIN_DC_BUS_VOLTAGE_RATIO 0.85
#define REG_TARGET_DC_BUS_VOLTAGE_RATIO 0.95
#define MIN_OUTPUT_CURRENT_TO_REGULATE_VOLTAGE  0.5

void DCDC_current_buck_loop_float( void );
void DCDC_voltage_boost_loop_float( void );
void DCDC_current_boost_loop_float( void );
void DCDC_voltage_pure_boost_loop_float( void );
bool calculate_boost_current(void);
PiOutput_t Pi_ControllerBoostFloat(PI_Parameters_t PI, PiOutput_t PIout, float Ref, float ValueRead);
PiOutput_t Pi_ControllerBuckFloat(PI_Parameters_t PI, PiOutput_t PIout, float Ref, float ValueRead  );


void DCDCConverterInit(void);
void IncreasePulseStateDutyToSuperCapsVoltage( void );
uint16_t CalculateCurrentOffset(uint16_t Offsetinput);
void StopAllPWMs(void);
void TrackSensorValueForDEBUG(float sensorValue1, float sensorValue2, float sensorValue3, float sensorValue4);

inline void handleEfuseVinOccurence();
inline void handleEFuseBBOccurence();



extern uint16_t MaximumDuty;
extern uint16_t MinimumDuty;
extern uint16_t PulseDutyOne;
extern uint16_t PulseDutyTwo;
extern uint16_t PulseDutyThree;
extern float  PulseVoltageRangeOne;
extern float  PulseVoltageRangeTwo;
extern float PulseBuckBoundary;



extern volatile uint16_t efuse_top_half_flag;
extern volatile bool eFuseInputCurrentOcurred;
extern volatile bool eFuseBuckBoostOcurred;

extern uint16_t trickleChargeRangeState;
#endif /* APP_INC_DCDC_H_ */
