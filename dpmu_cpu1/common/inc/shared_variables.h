/*
 * shared_variables.h
 *
 *  Created on: 20 juni 2023
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#ifndef COMMON_INC_SHARED_VARIABLES_H_
#define COMMON_INC_SHARED_VARIABLES_H_

#include <dpmu_type.h>
#include "device.h"
#include "GlobalV.h"




typedef struct sharedVars_cpu1toCpu2_t // commonCpu1ToCpu2
{
    uint8_t iop_operation_request_state;        /* First section in the group */
//    uint8_t DPMU_RequestEmergecyStop;           /* Handled with IPC flag - Request to emergency stop */

    float max_allowed_dc_bus_voltage;
    float target_voltage_at_dc_bus;
    float min_allowed_dc_bus_voltage;
    float vdc_bus_short_circuit_limit;
    float vdroop;                            /* the VDROOP setting */

    float max_allowed_load_power;            /* max_allowed_load_power, set */
    float available_power_budget_dc_input;   /* from our INPUT bus */
    float use_power_budget_dc_input;         /* include pwr budget from INPUT bus in calculation */
    float use_power_budget_dc_shared;        /* include pwr budget from SHARED bus in calculation */

    /* for safe parking we need to allow for total drainage of external energy
     * storage
     * this variable is set to non-zero if this is allowed
     */
    uint16_t safe_parking_allowed;

    float max_voltage_applied_to_energy_bank;
    float safety_threshold_state_of_charge;
    float min_voltage_applied_to_energy_bank;
//    uint16_t safe_parking_allowed;

    float max_allowed_voltage_energy_cell;
    float constant_voltage_threshold;
    float min_allowed_voltage_energy_cell;
    float preconditional_threshold;
    float ess_current;

    bool having_battery;    /* true for Lithium Battery, false for Super Capacitors */

    bool debug_log_disable_flag;

    bool dpmu_default_flag;

    bool QsbSwitchRequestState;
    bool QinbSwitchRequestState;
    bool QlbSwitchRequestState;
    bool QinrushSwitchRequestState;

    float initialCapacitance;
    float currentCapacitance;
    bool newCapacitanceSaved;
    bool DPMUAppInfoInitializedFlag;

    bool temperatureMaxLimitReachedFlag;


} sharedVars_cpu1toCpu2_t;

extern struct sharedVars_cpu1toCpu2_t sharedVars_cpu1toCpu2;

/* for calculating SoH */
typedef struct shared_energy_bank
{
    float    energyBankVoltageBeforeFirstCharge;
    float    energyBankVoltageAfterFirstCharge;
    float    chargeCurrent;
    uint32_t chargeTime;
    float    cellVoltageBeforeFirstCharge[30];
    float    cellVoltageAfterFirstCharge[30];
} shared_energy_bank_t;

typedef struct sharedVars_cpu2toCpu1_t // commonCpu2ToCpu1
{
    debug_log_t debug_log;
    uint16_t current_state;
    uint16_t error_code;

    float voltage_at_dc_bus;                 /* last measured DC bus Voltage */
    float voltage_at_storage_bank;              /* last measured super capacitor bank */
    uint16_t vdroop;                            /* the VDROOP used */
    uint16_t power_consumed_by_load;            /* last calculated */
    uint16_t power_from_dc_input;               /* last calculated */

    uint16_t available_power_budget_dc_share;   /* from our SHARED bus */


    bool newCapacitanceAvailable;
    float initialCapacitance;
    float currentCapacitance;

    float    soc_energy_bank;                   /* last measured state of charge */
    float    soh_energy_bank;                   /* last calculated state of health */
    float    remaining_energy_to_min_soc_energy_bank;   /* usable energy */

    float    soc_energy_cell[30];               /* last measured state of charge [Volt] */
//    uint16_t soh_energy_cell[30];               /* last calculated state of health */

    /* these values are in use in the control algorithm
     * here in case we want to compare it with IOP settings */

    float current_charging_limit;            /* ESS current */
    float current_charging_current;          /* last measured charging current */
    float current_max_allowed_load_power;    /* max_allowed_load_power, read */
    float current_load_current;              /* last measured load current */
    float current_input_current;
    shared_energy_bank_t energy_bank;
} sharedVars_cpu2toCpu1_t;



extern struct sharedVars_cpu2toCpu1_t sharedVars_cpu2toCpu1;

extern char ipc_soh_msg[128];

#endif /* COMMON_INC_SHARED_VARIABLES_H_ */
