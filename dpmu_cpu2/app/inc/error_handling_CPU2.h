/*
 * error_handling.h
 *
 *  Created on: 18 de jun de 2024
 *      Author: gferreira
 */

#ifndef APP_INC_ERROR_HANDLING_CPU2_H_
#define APP_INC_ERROR_HANDLING_CPU2_H_

typedef enum error_handling_type {
    NO_HANDLING_ERROR = 0,
    LOAD_OVER_CURRENT_HANDLING_ERROR,
    BUS_SHORT_CIRCUIT_HANDLING_ERROR,
    SUPERCAP_OVER_CURRENT_HANDLING_ERROR,
    INPUT_SHORT_CIRCUIT_HANDLING_ERROR,
    LOAD_SHORT_CIRCUIT_HANDLING_ERROR,
    SUPERCAP_SHORT_CIRCUIT_HANDLING_ERROR,
    BUS_UNDER_VOLTAGE_HANDLING_ERROR,
    BUS_OVER_VOLTAGE_HANDLING_ERROR,
    HIGH_TEMPERATURE_HANDLING_ERROR,
    OVER_TEMPERATURE_HANDLING_ERROR,
    DISCHARGING_HANDLING_ERROR,
    CHARGING_HANDLING_ERROR,
    NUMBER_OF_HANDLING_ERRORS
} error_handling_type_t;


error_handling_type_t DpmuHandlingErrorOcurred();
bool DpmuErrorOcurred();
void ResetDpmuErrorOcurred();
void HandleLoadOverCurrent(float max_allowed_load_current, uint16_t efuse_top_half_flag);
void HandleDCBusShortCircuit();
void HandleDCBusOverVoltage();
void HandleDCBusUnderVoltage();
void HandleOverTemperature();



#endif /* APP_INC_ERROR_HANDLING_CPU2_H_ */
