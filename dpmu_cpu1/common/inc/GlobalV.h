/**
 * ************************************************************************
 * @file  GlobalV.h
 * @brief This file contains global variables used in DCDC and CLLC.
 * ************************************************************************
 *  Created on: 5 Dec 2022
 *      Author: Luyu Wang
 * ************************************************************************
 */

#ifndef APP_INC_GLOBALV_H_
#define APP_INC_GLOBALV_H_


#include <stdint.h>

#define MaximumCellVoltage      4000
#define MaintainanceChargeLevel 33800   // To be Changed
#define LowestVoltageforBoost   24920   // To be changed
#define MaximumInductorCurrent  15      // Maximum average continuous inductor current
#define FromAmpToTick           136.5f // ACS730KLCTR-20AB-S for the current patch board   100mV/A   0.1/3.3*4095
#define ADC_VREF 3.0
#define VOLTAGE_DIVIDER_GAIN 100.0 // 100-> 100=Resistive divisor ratio
#define BOOST_TIME_BASE_PERIOD 714
#define BUCK_NORMAL_MODE_TIME_BASE_PERIOD 714
#define BUCK_PULSE_MODE_TIME_BASE_PERIOD 5000
#define BUCK_PULSE_BOUNDARY_RATIO 0.55
#define PULSE_MODE_INITIAL_DUTY_CYCLE 175
#define MAX_INDUCTOR_BUCK_CURRENT 5.0
#define MAX_INDUCTOR_BOOST_CURRENT 19
#define UPDATE_CPU2_FIRMWARE_CODE 9999
#define NUMBER_OF_CELLS 30

typedef struct Counters
{
    uint32_t StateMachineCounter;
    int16_t PrestateCounter;
    uint16_t InrushCurrentLimiterCounter;
} Counters_t;

typedef struct debug_log
{
    int16_t ISen1;    // Output load current sensor x10
    int16_t ISen2;    // Storage current sensor (supercap) x10
    int16_t IF_1;     // Input current x10
    int16_t I_Dab2;   // CLLC1 Current x100
    int16_t I_Dab3;   // CLLC2 Current x100
    int16_t Vbus;      // VBus voltage x10
    int16_t VStore;    // VStore voltage x10
    uint16_t AvgVbus;
    uint16_t AvgVStore;
    uint16_t CurrentState; // State of CPU2 state machine
    uint16_t counter;
    uint16_t elapsed_time;
    uint16_t cellVoltage[NUMBER_OF_CELLS];
    int16_t BaseBoardTemperature;
    int16_t MainBoardTemperature;
    int16_t MezzanineBoardTemperature;
    int16_t PowerBankBoardTemperature;
    uint16_t RegulateAvgInputCurrent;
    uint16_t RegulateavgOutputCurrent;
    uint16_t RegulateAvgVStore;
    uint16_t RegulateAvgVbus;
    uint16_t RegulateIRef;
    uint32_t CurrentTime;
} debug_log_t;



typedef struct States
{
    uint16_t State_Before;      // State before current state
    uint16_t State_Current;     // Current state
    uint16_t State_Next;        // Next state
    uint16_t State_Before_Balancing; // State before Balancing state
} States_t;

typedef struct PiOutput
{
    float Output;
    float Int_out;
    float calculated_error;
    uint16_t dutyCycle;
} PiOutput_t;

typedef struct PI_Parameters
{
    float Igain;        // Integral gain
    float Pgain;        // Proportion gain
    float UpperLimit;   // PI Output Upper Limit
    float LowerLimit;   // PI Output Lower Limit
} PI_Parameters_t;

typedef struct CLLC_Parameters
{
    int16_t IDAB2_RAW;
    int16_t IDAB3_RAW;
    uint16_t VHalfC_U_Raw;
    uint16_t VHalfC_U_Filter[9]; //reserved for filtering  VBUS_F[8] stores the filtered value
    uint16_t VHalfC_D_Raw;
    uint16_t VHalfC_D_Filter[9];
    uint16_t VCell_U_Raw;
    uint16_t VCell_U_Filter[9];  //reserved for filtering  VSTORE_F[8] stores the filtered value
    uint16_t VCell_D_Raw;
    uint16_t VCell_D_Filter[9];
    uint16_t CellVoltage_Ref_Raw;
    uint16_t CurrentOffset1;
    uint16_t CurrentOffset2;
    uint16_t I_Ref_Raw;
    uint16_t Counter;            // Counter for Digi filter
    uint16_t PeriodTick;         // Period
    uint16_t PhaseTick;          // Phase shift
    uint16_t PowerDirection;
    uint16_t CellDischargePeriod;
    uint16_t DischargeFlag1;
    uint16_t DischargeFlag2;
    uint16_t CelldischargeNumber1;
    uint16_t CelldischargeNumber2;
} CLLC_Parameters_t;



// ADC Values shall not be stored in this struct. Now it is sotred in the sensorVector updated by interrupts. See sensor.c
// All control related variables shall be done in real values not in counts. *_Raw values should be removed from this Structure
typedef struct DCDC_Parameters
{

    float I_Ref_Real;
    float iIn_limit;
    float target_Voltage_At_DCBus;
    float target_Voltage_At_VStore;
    float avgVStore;
    float avgVBus;
    float RegulateAvgInputCurrent;
    float RegulateAvgOutputCurrent;
    float RegulateAvgVStore;
    float RegulateAvgVbus;
    uint16_t counter;
} DCDC_Parameters_t;


extern DCDC_Parameters_t DCDC_VI;
extern CLLC_Parameters_t CLLC_VI;
extern PiOutput_t VLoop_PiOutput;
extern PiOutput_t ILoop_PiOutput;
extern States_t StateVector;
extern uint16_t T_delay ;
extern uint16_t CellNumber ;
extern uint16_t MatrixReseted ;
extern uint16_t pleaseChangecell ;
extern uint16_t CellVoltages[30];
extern Counters_t CounterGroup ;
extern uint16_t PhaseShiftCount;




#if 0
uint16_t A1= 2032;
uint16_t A2= 3514;
float dutya=0.5;
#endif


#endif /* APP_INC_GLOBALV_H_ */
