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

#define MAX_ENERGY_BANK_VOLTAGE_RATIO 0.86
#define MINIMUM_CHARGING_TIME_IN_MILISECS 60000
#define NUMBER_OF_AVG_CHARGING_CURRENT_COUNT 10

enum sohStates { sohCalcWait = 0,
                 sohCalcInit,
                 sohCalcCapacitance,
                 sohCalcEnd,
                 verifySoHFromFlash,
                 saveNewEnergyConditionToFlash,
                 saveCurrentEnergyConditionToFlash};

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

extern bool startSaveCapacitance;

extern energy_bank_t energy_bank_settings;

void energy_storage_update_settings(void);
void energy_storage_check(void);

void calcAccumlatedCharge(void);
bool requestCPU1ToSaveCapacitancesToFlash(float  initialCapacitance, float currentCapacitance);


#endif /* APP_INC_ENERGY_STORAGE_H_ */
