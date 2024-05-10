/*
 * energy_storage.h
 *
 *  Created on: 25 okt. 2023
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#ifndef APP_INC_ENERGY_STORAGE_H_
#define APP_INC_ENERGY_STORAGE_H_


#include <stdint.h>

#include "CLLC.h"
#include "shared_variables.h"

#define MAX_ENERGY_VOLTAGE_RATIO 0.8733

typedef struct energy_bank {
    float max_voltage_applied_to_energy_bank;
    float constant_voltage_threshold;
    float min_voltage_applied_to_energy_bank;
    float preconditional_threshold;
    float safety_threshold_state_of_charge;      /* energy needed for safe parking */
    float max_allowed_voltage_energy_cell;
    float min_allowed_voltage_energy_cell;
    float ESS_Current;                           /* max charging current */
} energy_bank_t;

typedef struct energy_bank_condition {
    float stateOfHealthPercent;
    float stateOfChargePercent;
    float capacitance;
    float inititalVoltage;
    float finalVoltage;
    float accumulatedCharge;
    float initialCapacitance;
    float totalChargeTime;
    uint32_t last_timer;
    uint8_t initializedSoCIndicator;
} energy_bank_condition_t;

extern energy_bank_t energy_bank_settings;
extern energy_bank_condition_t energy_bank_condition;
extern bool newSoCAvailable;

void energy_storage_update_settings(void);
void energy_storage_check(void);

void initCalcStateOfCharge(void);
void calcAccumlatedCharge(void);
void finallyCalcStateOfCharge(void);
void saveStatOfChargeToFlash(void);
void retriveStateOfChargeFromFlash(void);

//static bool balancing(float cellvoltages[30]);
//static void SortCellVoltageIndex(float cellvoltages[], uint16_t Position);

#endif /* APP_INC_ENERGY_STORAGE_H_ */
