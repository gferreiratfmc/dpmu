/*
 * error_handling.h
 *
 *  Created on: 18 de jun de 2024
 *      Author: gferreira
 */

#ifndef APP_INC_ERROR_HANDLING_CPU2_H_
#define APP_INC_ERROR_HANDLING_CPU2_H_

typedef enum dpmu_error_class {
    DPMU_ERROR_CLASS_NO_ERROR = 0,
    DPMU_ERROR_CLASS_SHORT_CIRCUT
} dpmu_error_class_t;

bool DpmuErrorOcurred();
dpmu_error_class_t DpmuErrorOcurredClass();
void ResetDpmuErrorOcurred();
void HandleLoadOverCurrent(float max_allowed_load_current, uint16_t efuse_top_half_flag);
void HandleDCBusShortCircuit();
void HandleDCBusOverVoltage();
void HandleDCBusUnderVoltage();
void HandleOverTemperature();
void SignalFaultStateToCPU1();
void HandleFaultStateAckFromCPU1();

#endif /* APP_INC_ERROR_HANDLING_CPU2_H_ */
