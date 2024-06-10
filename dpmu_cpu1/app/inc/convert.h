/*
 * convert.h
 *
 *  Created on: 13 sep. 2023
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#ifndef APP_INC_CONVERT_H_
#define APP_INC_CONVERT_H_


#include <stdint.h>

#include "type_common.h"

enum {
    CONVERT_TO_ADC = 0,
    CONVERT_FROM_ADC,
};

/*** IOP -> DPMU ***/
float    convert_dc_bus_voltage_from_OD(uint8_t value);
uint8_t  convert_dc_bus_voltage_to_OD(float value);

float    convert_power_from_OD(uint16_t value);
uint16_t convert_power_to_OD(float value);

float    convert_voltage_energy_bank_from_OD(uint8_t value);
uint8_t  convert_voltage_energy_bank_to_OD(float value);
float    convert_min_voltage_applied_to_energy_bank_from_OD(uint8_t value);
uint8_t  convert_min_voltage_applied_to_energy_bank_to_OD(float value);

float    convert_voltage_energy_cell_from_OD(uint16_t value);
uint8_t  convert_voltage_energy_cell_to_OD(float value);
float    convert_ess_current_from_OD(uint16_t value);
int8_t  convert_ess_current_to_OD(float value);

/*** DPMU -> IOP ***/
uint16_t convert_soc_energy_bank_to_OD(float value);    /* state of charge */
float    convert_soh_energy_bank_from_OD(uint8_t value);   /* state of health */
uint8_t  convert_soh_energy_bank_to_OD(float value);
float    convert_energy_soc_energy_bank_from_OD(uint16_t value);
uint16_t convert_energy_soc_energy_bank_to_OD(float value);
uint16_t convert_remaining_energy_to_min_soc_energy_bank_to_OD(float remaining_SoC);
float convert_safety_threshold_soc_energy_bank_from_OD(uint16_t value);
uint16_t convert_safety_threshold_soc_energy_bank_to_OD(float value);

float    convert_soc_energy_cell_from_OD(uint16_t value);   /* state of charge */
uint8_t  convert_soc_energy_cell_to_OD(float value);
void     convert_soc_all_energy_cell_to_OD(float *cell, uint16_t *cell_converted);
float    convert_soh_energy_cell_from_OD(uint16_t value);      /* state of health */
uint8_t  convert_soh_energy_cell_to_OD(float value);
void     convert_soh_all_energy_cell_to_OD(float *cell, uint16_t *cell_converted);

float    convert_charge_time_to_seconds(uint16_t charge_time);

int8_t convert_dc_load_current_to_OD(float value);

#endif /* APP_INC_CONVERT_H_ */
