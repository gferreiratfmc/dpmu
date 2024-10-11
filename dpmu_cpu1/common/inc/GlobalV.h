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


#define BOOST_TIME_BASE_PERIOD 714
#define BUCK_NORMAL_MODE_TIME_BASE_PERIOD 714
#define BUCK_PULSE_MODE_TIME_BASE_PERIOD 5000
#define MAX_INDUCTOR_BUCK_CURRENT 5.0
#define MAX_INDUCTOR_BOOST_CURRENT 15.0
#define NUMBER_OF_CELLS 30
#define MAX_VOLTAGE_ENERGY_BANK 90.0
#define MAX_VOLTAGE_ENERGY_CELL 3.0
#define MAX_DPMU_OUTPUT_CURRENT 20.0
#define DPMU_SHORT_CIRCUIT_CURRENT (MAX_DPMU_OUTPUT_CURRENT * 1.25)
#define DPMU_SUPERCAP_SHORT_CIRCUIT_CURRENT 25.0
#define MAGIC_NUMBER 0xDEADFACE
#define SERIAL_NUMBER_SIZE_IN_CHARS 30

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
    BalancingInit = 18,              // 18
    Balancing,                  // 19
    BalancingStop = 191,              // 191
    CC_Charge = 20,                  // 20
    StopEPWMs,                  // 21
    ChargeRamp,                 // 22
    PreInitialized = 255
};



typedef struct app_vars
{
    uint32_t MagicNumber;
    float initialCapacitance;
    float currentCapacitance;
    unsigned char serialNumber[SERIAL_NUMBER_SIZE_IN_CHARS];

} app_vars_t;

typedef struct debug_log
{
    uint32_t MagicNumber;
    int16_t ISen1;    // Output load current sensor x10
    int16_t ISen2;    // Storage current sensor (supercap) x10
    int16_t IF_1;     // Input current x10
    int16_t I_Dab2;   // CLLC1 Current x100
    int16_t I_Dab3;   // CLLC2 Current x100
    int16_t Vbus;      // VBus voltage x10
    int16_t VStore;    // VStore voltage x10
    int16_t AvgVbus;
    int16_t AvgVStore;
    int16_t BaseBoardTemperature;
    int16_t MainBoardTemperature;
    int16_t MezzanineBoardTemperature;
    int16_t PowerBankBoardTemperature;
    int16_t RegulateAvgInputCurrent;
    int16_t RegulateAvgOutputCurrent;
    int16_t RegulateAvgVStore;
    int16_t RegulateAvgVbus;
    int16_t RegulateIRef;
    uint16_t ILoop_PiOutput;
    int16_t cellVoltage[NUMBER_OF_CELLS];
    int16_t CurrentState; // State of CPU2 state machine
    uint32_t counter;
    uint32_t CurrentTime;
    uint16_t elapsed_time;
    uint32_t address;
    //uint16_t Switches;
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


// All control related variables shall be done in real values not in counts.
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
extern PiOutput_t VLoop_PiOutput;
extern PiOutput_t ILoop_PiOutput;
extern States_t StateVector;




#endif /* APP_INC_GLOBALV_H_ */
