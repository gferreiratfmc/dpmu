/*
 * error_handling.h
 *
 *  Created on: 18 de jun de 2024
 *      Author: gferreira
 */

#ifndef APP_INC_ERROR_HANDLING_CPU2_H_
#define APP_INC_ERROR_HANDLING_CPU2_H_

bool DpmuErrorOcurred();
void ResetDpmuErrorOcurred();
void HandleLoadOverCurrent(float max_allowed_load_current, uint16_t efuse_top_half_flag);
void HandleDCBusShortCircuit();
void HandleDCBusOverVoltage();
void HandleDCBusUnderVoltage();
void HandleOverTemperature();

#endif /* APP_INC_ERROR_HANDLING_CPU2_H_ */
