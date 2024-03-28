/*
 * sensor.h
 *
 *  Created on: 23 de out de 2023
 *      Author: gferreira
 */

#ifndef APP_INC_SENSOR_H_
#define APP_INC_SENSOR_H_


#include "stdbool.h"
#include "stdint.h"


typedef struct Sensor {
    char* name;
    float realValue;
    uint16_t counts;
    bool newADCReady;
    uint16_t maxCounts;
    float zeroVoltageOffset;
    float adcReference;
    float gain;
    bool invertedGain;
    bool differentialADC;
    bool convertedReady;
} Sensor_t;


enum sensorIndex
{
    ISen1fIdx=0,
    ISen2fIdx,
    IF_1fIdx,
    V_UpfIdx,
    V_DwnfIdx,
    I_Dab2fIdx,
    I_Dab3fIdx,
    VBusIdx,
    VStoreIdx,
    NumOfSensors,
    FirstSensorIdx = ISen1fIdx,
    LastSensorIdx = VStoreIdx
};


void ConvertCountsToReal( Sensor_t *sensor);
void ConvertCountsToRealOfAllSensors();
void InitializeSensorParameters();
void ReadVbusVstoreV24f();
int CalibrateZeroVoltageOffsetOfSensors();

extern Sensor_t sensorVector[NumOfSensors];
extern bool runStateMachineFlag;
#endif /* APP_INC_SENSOR_H_ */
