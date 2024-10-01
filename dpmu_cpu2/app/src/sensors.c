/*
 * sensors.c
 *
 *  Created on: 23 de out de 2023
 *      Author: gferreira
 */

#include <string.h>

#include "board.h"
#include "CLLC.h"
#include "DCDC.h"
#include "GlobalV.h"
#include "hal.h"
#include "sensors.h"
#include "state_machine.h"
#include "switch_matrix.h"

Sensor_t sensorVector[NumOfSensors];

/**
 * @brief  ADC B Interrupt 4 Function (former Current_Ov interrupt)
 */
__interrupt void INT_ADCINB_4_ISR(void) {

    sensorVector[ISen2fIdx].counts = ADC_readResult(ADCBRESULT_BASE, ADC_SOC_NUMBER0);
    sensorVector[ISen2fIdx].newADCReady = true;
    sensorVector[ISen2fIdx].convertedReady   = false;

    sensorVector[IF_1fIdx].counts = ADC_readResult(ADCBRESULT_BASE, ADC_SOC_NUMBER1);
    sensorVector[IF_1fIdx].newADCReady = true;
    sensorVector[IF_1fIdx].convertedReady   = false;

    StateMachine();
    //runStateMachineFlag = true;

    ADC_clearInterruptStatus(ADCB_BASE, ADC_INT_NUMBER4);
    Interrupt_clearACKGroup( INT_ADCINB_4_INTERRUPT_ACK_GROUP );


}

/**
 * @brief  ADC A Interrupt 2 Function
 */
void INT_ADCINA_2_ISR(void)
{

    sensorVector[VBusIdx].counts = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER0);
    sensorVector[VBusIdx].newADCReady = true;
    sensorVector[VBusIdx].convertedReady = false;

    sensorVector[VStoreIdx].counts = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER1);
    sensorVector[VStoreIdx].newADCReady = true;
    sensorVector[VStoreIdx].convertedReady = false;

    ADC_clearInterruptStatus(ADCINA_BASE, ADC_INT_NUMBER2);

    Interrupt_clearACKGroup( INT_ADCINA_2_INTERRUPT_ACK_GROUP );

}

/**
 * @brief  ADC C Interrupt 1 Function
 */

__interrupt void INT_ADCINC_1_ISR(void) {

    sensorVector[ISen1fIdx].counts = ADC_readResult(ADCCRESULT_BASE, ADC_SOC_NUMBER0);
    sensorVector[ISen1fIdx].newADCReady = true;
    sensorVector[ISen1fIdx].convertedReady   = false;

    ADC_clearInterruptStatus(ADCC_BASE, ADC_INT_NUMBER1);

    Interrupt_clearACKGroup(INT_ADCINC_1_INTERRUPT_ACK_GROUP );

}

/**
 * @brief  ADC D Interrupt 3 Function
 */

__interrupt void INT_ADCIND_3_ISR(void){

    sensorVector[V_DwnfIdx].counts = ADC_readResult(ADCDRESULT_BASE, ADC_SOC_NUMBER0);
    sensorVector[V_UpfIdx].counts = ADC_readResult(ADCDRESULT_BASE, ADC_SOC_NUMBER1);
    sensorVector[I_Dab2fIdx].counts = ADC_readResult(ADCDRESULT_BASE, ADC_SOC_NUMBER2);
    sensorVector[I_Dab3fIdx].counts = ADC_readResult(ADCDRESULT_BASE, ADC_SOC_NUMBER3);

    sensorVector[V_UpfIdx].newADCReady   = true;
    sensorVector[V_DwnfIdx].newADCReady  = true;
    sensorVector[I_Dab2fIdx].newADCReady = true;
    sensorVector[I_Dab3fIdx].newADCReady = true;

    sensorVector[V_UpfIdx].convertedReady   = false;
    sensorVector[V_DwnfIdx].convertedReady  = false;
    sensorVector[I_Dab2fIdx].convertedReady = false;
    sensorVector[I_Dab3fIdx].convertedReady = false;

    ADC_clearInterruptStatus(ADCD_BASE, ADC_INT_NUMBER3);

    Interrupt_clearACKGroup(INT_ADCIND_3_INTERRUPT_ACK_GROUP );

}

void ConvertCountsToReal( Sensor_t *sensor) {
    if( sensor->newADCReady ) {

        if( sensor->differentialADC ) {
            sensor->realValue =  sensor->adcReference * ( ( 2 * (float)sensor->counts / (float)sensor->maxCounts ) -1 );
        } else {
            sensor->realValue = ( sensor->adcReference * ( (float)sensor->counts / (float)sensor->maxCounts ) );
        }
        if( sensor->invertedGain ) {
            sensor->realValue = ( sensor->zeroVoltageOffset - sensor->realValue ) * sensor->gain;
        } else {
            sensor->realValue = ( sensor->realValue - sensor->zeroVoltageOffset ) * sensor->gain;
        }
        sensor->newADCReady = false;
        sensor->convertedReady = true;
    }
}



/**
 * @brief  Calibrates the zero voltage offset of the current sensors
 * Runs only once in the INITIALIZE state of main state_machine
 */

int CalibrateZeroVoltageOffsetOfSensors() {
    static uint16_t IsensorsIdxList[] = { ISen1fIdx, ISen2fIdx, IF_1fIdx, I_Dab2fIdx, I_Dab3fIdx, V_UpfIdx, V_DwnfIdx}; //, VBusIdx, VStoreIdx};
    Sensor_t *sensor;
    static float avgZeroVoltageOffset = 0.0;
    static uint16_t calibrationComplete = 0;
    static uint16_t avgCounter = 0;
    static uint16_t idxCounter = 0;

    if ( calibrationComplete == 0 ) {
        switch_matrix_reset();
        DEVICE_DELAY_US(1000);
        sensor = &sensorVector[IsensorsIdxList[idxCounter]];
        // Calculates the average of the voltages read while there is no current flowing through sensors
        if( avgCounter < 50 ) {
            if( sensor->convertedReady == true) {
                if( sensor->differentialADC ) {
                    avgZeroVoltageOffset  = avgZeroVoltageOffset + sensor->adcReference * ( ( 2 * (float)sensor->counts / (float)sensor->maxCounts ) -1 );
                } else {
                    avgZeroVoltageOffset  = avgZeroVoltageOffset + sensor->adcReference * ( (float)sensor->counts / (float)sensor->maxCounts );
                }
                avgCounter++;
            }
        } else {
            // Save the voltage average as the zero voltage offset of the sensor
            sensor->zeroVoltageOffset = avgZeroVoltageOffset / avgCounter;
            avgCounter = 0;
            avgZeroVoltageOffset = 0.0;
            idxCounter++;
        }
        // After all sensors in the list is set completes the calibration. It doesn't run anymore. Only with a reset
        if(idxCounter == sizeof(IsensorsIdxList) ){
            calibrationComplete = 1;
        }
    }
    return calibrationComplete;
}

inline void ConvertSensorsCountsToReal() {
    uint16_t sensorIdx = 0;

    sensorIdx++;
    for(sensorIdx = 0; sensorIdx < NumOfSensors; sensorIdx++){
        ConvertCountsToReal( &sensorVector[sensorIdx] );
    }
}

void InitializeSensorParameters() {


    sensorVector[ISen1fIdx].maxCounts = 65535;
    sensorVector[ISen1fIdx].zeroVoltageOffset = 1.52;
    sensorVector[ISen1fIdx].adcReference = 3.0;
    sensorVector[ISen1fIdx].gain = (1/0.05);
    sensorVector[ISen1fIdx].invertedGain = true;
    sensorVector[ISen1fIdx].differentialADC = false;
    sensorVector[ISen1fIdx].name = "Output current";

    sensorVector[ISen2fIdx].maxCounts = 65535;
    sensorVector[ISen2fIdx].zeroVoltageOffset = 1.5;
    sensorVector[ISen2fIdx].adcReference = 3.0;
    sensorVector[ISen2fIdx].gain = (1 / 0.05);
    //sensorVector[ISen2fIdx].gain = (1 / 0.055);
    sensorVector[ISen2fIdx].invertedGain = true;
    sensorVector[ISen2fIdx].differentialADC = false;
    sensorVector[ISen2fIdx].name = "Store current";

    sensorVector[IF_1fIdx].maxCounts = 65535;
    sensorVector[IF_1fIdx].zeroVoltageOffset = 1.05;
    sensorVector[IF_1fIdx].adcReference = 3.0;
    sensorVector[IF_1fIdx].gain = (1/0.05);
    //    sensorVector[IF_1fIdx].gain = (1/0.1); // Only for an old DPMU DSP Card
    sensorVector[IF_1fIdx].invertedGain = true;
    sensorVector[IF_1fIdx].differentialADC = false;
    sensorVector[IF_1fIdx].name = "Input Current";

    sensorVector[V_UpfIdx].maxCounts = 65535;
    sensorVector[V_UpfIdx].zeroVoltageOffset = 1.237;
    sensorVector[V_UpfIdx].adcReference = 3.0;
    sensorVector[V_UpfIdx].gain = 9.3; //2*4.187;
    sensorVector[V_UpfIdx].invertedGain = false;
    sensorVector[V_UpfIdx].differentialADC = false;
    sensorVector[V_UpfIdx].name = "Cell_V_up";

    sensorVector[V_DwnfIdx].maxCounts = 65535;
    sensorVector[V_DwnfIdx].zeroVoltageOffset = 1.237;
    sensorVector[V_DwnfIdx].adcReference = 3.0;
    sensorVector[V_DwnfIdx].gain = 9.3; //2*4.187;
    sensorVector[V_DwnfIdx].invertedGain = false;
    sensorVector[V_DwnfIdx].differentialADC = false;
    sensorVector[V_DwnfIdx].name = "Cell_V_down";

    sensorVector[I_Dab2fIdx].maxCounts = 65535;
    sensorVector[I_Dab2fIdx].zeroVoltageOffset = 0.9;
    sensorVector[I_Dab2fIdx].adcReference = 3.0;
    sensorVector[I_Dab2fIdx].gain = (1 / 0.2);
    sensorVector[I_Dab2fIdx].invertedGain = true;
    sensorVector[I_Dab2fIdx].differentialADC = false;
    sensorVector[I_Dab2fIdx].name = "CLLC I_1";

    sensorVector[I_Dab3fIdx].maxCounts = 65535;
    sensorVector[I_Dab3fIdx].zeroVoltageOffset = 0.9;
    sensorVector[I_Dab3fIdx].adcReference = 3.0;
    sensorVector[I_Dab3fIdx].gain = (1 / 0.2);
    sensorVector[I_Dab3fIdx].invertedGain = true;
    sensorVector[I_Dab3fIdx].differentialADC = false;
    sensorVector[I_Dab3fIdx].name = "CLLC I_2";

    sensorVector[VBusIdx].maxCounts = 65535;
    sensorVector[VBusIdx].zeroVoltageOffset = 0.0;
    sensorVector[VBusIdx].adcReference = 3.0;
    sensorVector[VBusIdx].gain = 100.0;
    sensorVector[VBusIdx].invertedGain = false;
    sensorVector[VBusIdx].differentialADC = true;
    sensorVector[VBusIdx].name = "Vbus";

    sensorVector[VStoreIdx].maxCounts = 65535;
    sensorVector[VStoreIdx].zeroVoltageOffset = 0.0;
    sensorVector[VStoreIdx].adcReference = 3.0;
    sensorVector[VStoreIdx].gain = 100.0;
    sensorVector[VStoreIdx].invertedGain = false;
    sensorVector[VStoreIdx].differentialADC = true;
    sensorVector[VStoreIdx].name = "VStore";

}



