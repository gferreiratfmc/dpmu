/*
 * canopen_sdo_upload_indices.c
 *
 *  Created on: 15 apr. 2023
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#ifndef APP_SRC_CANOPEN_INDICES_C_
#define APP_SRC_CANOPEN_INDICES_C_

#include "co_datatype.h"
#include "co_odaccess.h"
#include "convert.h"
#include "gen_indices.h"
#include "log.h"
#include "main.h"
#include "serial.h"
#include "shared_variables.h"
#include "temperature_sensor.h"
#include "timer.h"
#include "../../../dpmu_cpu2/app/inc/switches.h"

/* one CAN/CANopen message */
#pragma DATA_ALIGN(log_sent_data, 4)
unsigned char log_sent_data[8];

static inline uint8_t indices_I_STORE_PARAMETERS(UNSIGNED8 subIndex)
{
    uint8_t retVal = CO_FALSE;
    uint32_t value;

    // TODO: Maybe read values from designated area of external flash?
    switch (subIndex)
    {
    case S_SAVE_ALL_PARAMETERS:
        retVal = coOdGetObj_u32(I_STORE_PARAMETERS, S_SAVE_ALL_PARAMETERS, &value);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_SAVE_ALL_PARAMETERS: 0x%x\r\n", value);
        break;
    case S_SAVE_COMMUNICATION_PARAMETERS:
        retVal = coOdGetObj_u32(I_STORE_PARAMETERS, S_SAVE_COMMUNICATION_PARAMETERS, &value);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_SAVE_COMMUNICATION_PARAMETERS: 0x%x\r\n", value);
        break;
    case S_SAVE_APPLICATION_PARAMETERS:
        retVal = coOdGetObj_u32(I_STORE_PARAMETERS, S_SAVE_APPLICATION_PARAMETERS, &value);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_SAVE_APPLICATION_PARAMETERS: 0x%x\r\n", value);
        break;
    case S_SAVE_MANUFACTURER_DEFINED_PARAMETERS:
        retVal = coOdGetObj_u32(I_STORE_PARAMETERS, S_SAVE_MANUFACTURER_DEFINED_PARAMETERS, &value);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_SAVE_MANUFACTURER_DEFINED_PARAMETERS: 0x%x\r\n", value);
        break;
    default:
        Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD SUBINDEX: 0x%x\r\n", subIndex);
    }

    return retVal;
}

static inline uint8_t indices_I_RESTORE_DEFAULT_PARAMETERS(UNSIGNED8 subIndex)
{
    uint8_t retVal = CO_FALSE;
    uint32_t value;

    switch (subIndex)
    {
    case S_RESTORE_ALL_DEFAULT_PARAMETERS:
        retVal = coOdGetObj_u32(I_RESTORE_DEFAULT_PARAMETERS, S_RESTORE_ALL_DEFAULT_PARAMETERS, &value);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_RESTORE_ALL_DEFAULT_PARAMETERS: 0x%x\r\n", value);
        break;
    case S_RESTORE_COMMUNICATION_DEFAULT_PARAMETERS:
        retVal = coOdGetObj_u32(I_RESTORE_DEFAULT_PARAMETERS, S_RESTORE_COMMUNICATION_DEFAULT_PARAMETERS, &value);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_RESTORE_COMMUNICATION_DEFAULT_PARAMETERS: 0x%x\r\n", value);
        break;
    case S_RESTORE_APPLICATION_DEFAULT_PARAMETERS:
        retVal = coOdGetObj_u32(I_RESTORE_DEFAULT_PARAMETERS, S_RESTORE_APPLICATION_DEFAULT_PARAMETERS, &value);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_RESTORE_APPLICATION_DEFAULT_PARAMETERS: 0x%x\r\n", value);
        break;
    case S_RESTORE_MANUFACTURER_DEFINED_DEFAULT_PARAMETERS:
        retVal = coOdGetObj_u32(I_RESTORE_DEFAULT_PARAMETERS, S_RESTORE_MANUFACTURER_DEFINED_DEFAULT_PARAMETERS, &value);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_RESTORE_MANUFACTURER_DEFINED_DEFAULT_PARAMETERS: 0x%x\r\n", value);
        break;
    default:
        Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD SUBINDEX: 0x%x\r\n", subIndex);
    }

    return retVal;
}

static inline uint8_t indices_I_TPDO_MAPPING_TEMPERATURES(UNSIGNED8 subIndex)
{
    uint8_t retVal = CO_FALSE;


    /* all values were stored in the OD directly after each measurements */
    switch (subIndex)
    {
    case S_TEMPERATURE_MEASURED_AT_DPMU_HOTTEST_POINT_PDO:
        coOdPutObj_i8( I_TPDO_MAPPING_TEMPERATURES, S_TEMPERATURE_MEASURED_AT_DPMU_HOTTEST_POINT_PDO, temperatureHotPoint);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_TEMPERATURE_MEASURED_AT_DPMU_HOTTEST_POINT_PDO: 0x%x\r\n", temperatureHotPoint);
        break;
    case S_TEMPERATURE_BASE_PDO:
        coOdPutObj_i8( I_TPDO_MAPPING_TEMPERATURES, S_TEMPERATURE_BASE_PDO, temperatureSensorVector[TEMPERATURE_SENSOR_BASE]);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_TEMPERATURE_BASE_PDO: 0x%x\r\n", temperatureSensorVector[TEMPERATURE_SENSOR_BASE]);
        break;
    case S_TEMPERATURE_MAIN_PDO:
        coOdPutObj_i8( I_TPDO_MAPPING_TEMPERATURES, S_TEMPERATURE_MAIN_PDO, temperatureSensorVector[TEMPERATURE_SENSOR_MAIN] );
        Serial_debug(DEBUG_INFO, &cli_serial, "S_TEMPERATURE_MAIN_PDO: 0x%x\r\n", temperatureSensorVector[TEMPERATURE_SENSOR_MAIN]);
        break;
    case S_TEMPERATURE_MEZZANINE_PDO:
        coOdPutObj_i8( I_TPDO_MAPPING_TEMPERATURES, S_TEMPERATURE_MEZZANINE_PDO, temperatureSensorVector[TEMPERATURE_SENSOR_MEZZANINE] );
        Serial_debug(DEBUG_INFO, &cli_serial, "S_TEMPERATURE_MEZZANINE_PDO: 0x%x\r\n", temperatureSensorVector[TEMPERATURE_SENSOR_MEZZANINE]);
        break;
    case S_TEMPERATURE_PWR_BANK_PDO:
        coOdPutObj_i8( I_TPDO_MAPPING_TEMPERATURES, S_TEMPERATURE_PWR_BANK_PDO, temperatureSensorVector[TEMPERATURE_SENSOR_PWR_BANK] );
        Serial_debug(DEBUG_INFO, &cli_serial, "S_TEMPERATURE_PWR_BANK_PDO: 0x%x\r\n", temperatureSensorVector[TEMPERATURE_SENSOR_PWR_BANK]);
        break;
    default:
        Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD SUBINDEX: 0x%x\r\n", subIndex);
    }

    return retVal;
}

static inline uint8_t indices_I_DATE_AND_TIME(void)
{
    uint8_t retVal = CO_FALSE;
    uint32_t value = 0;
    timer_time_t t;

    timer_get_time(&t);
    value = t.can_time;
    retVal = coOdPutObj_u32(I_DATE_AND_TIME, 0, value);
    Serial_debug(DEBUG_INFO, &cli_serial, "I_DATE_AND_TIME: 0x%08lx\r\n", value);

    return retVal;
}

static RET_T indices_I_SET_NODEID(void)
{
    RET_T retVal = RET_OK;
    uint8_t value;

    retVal = coOdGetObj_u8(I_SET_NODEID, 0, &value);
    Serial_debug(DEBUG_INFO, &cli_serial, "I_SET_NODEID: 0x%x\r\n", value);

    return retVal;
}

static inline uint8_t indices_I_DC_BUS_VOLTAGE(UNSIGNED8 subIndex)
{
    uint8_t retVal = CO_FALSE;
    uint8_t value;

    switch (subIndex)
    {
    case S_MIN_ALLOWED_DC_BUS_VOLTAGE:
        retVal = coOdGetObj_u8(I_DC_BUS_VOLTAGE, S_MIN_ALLOWED_DC_BUS_VOLTAGE, &value);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_MIN_ALLOWED_DC_BUS_VOLTAGE: 0x%x\r\n", value);
        break;
    case S_MAX_ALLOWED_DC_BUS_VOLTAGE:
        retVal = coOdGetObj_u8(I_DC_BUS_VOLTAGE, S_MAX_ALLOWED_DC_BUS_VOLTAGE, &value);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_MAX_ALLOWED_DC_BUS_VOLTAGE: 0x%x\r\n", value);
        break;
    case S_TARGET_VOLTAGE_AT_DC_BUS:
        retVal = coOdGetObj_u8(I_DC_BUS_VOLTAGE, S_TARGET_VOLTAGE_AT_DC_BUS, &value);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_TARGET_VOLTAGE_AT_DC_BUS: 0x%x\r\n", value);
        break;
    case S_VDC_BUS_SHORT_CIRCUIT_LIMIT:
        retVal = coOdGetObj_u8(I_DC_BUS_VOLTAGE, S_VDC_BUS_SHORT_CIRCUIT_LIMIT, &value);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_VDC_BUS_SHORT_CIRCUIT_LIMIT: 0x%x\r\n", value);
        break;
    case S_VDROOP:
//        retVal = coOdPutObj_u8(I_DC_BUS_VOLTAGE, S_VDROOP, sharedVars_cpu1toCpu2.vdroop);
        retVal = coOdPutObj_u8(I_DC_BUS_VOLTAGE, S_VDROOP, sharedVars_cpu2toCpu1.vdroop);
        //TODO does the 'value' need to be converted ?
        Serial_debug(DEBUG_INFO, &cli_serial, "S_VDROOP: 0x%x\r\n", value);
        break;
    default:
        Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD SUBINDEX: 0x%x\r\n", subIndex);
    }

    return retVal;
}

static inline uint8_t indices_I_ESS_CURRENT(void)
{
    uint8_t retVal = CO_FALSE;
    int16_t value;

    value = 0;
    value = convert_ess_current_to_OD(sharedVars_cpu2toCpu1.current_charging_current);
    retVal = coOdPutObj_i8(I_ESS_CURRENT, 0, (int8_t)value );

    Serial_debug(DEBUG_INFO, &cli_serial, "I_ESS_CURRENT: 0x%x\r\n", value);

    return retVal;
}

static inline uint8_t indices_I_ENERGY_CELL_SUMMARY(UNSIGNED8 subIndex)
{
    uint8_t retVal = CO_FALSE;
    uint8_t value;

    switch (subIndex)
    {
    case S_MIN_VOLTAGE_ENERGY_CELL:
        value = convert_voltage_energy_cell_to_OD( sharedVars_cpu1toCpu2.min_allowed_voltage_energy_cell );
        retVal = coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_MIN_VOLTAGE_ENERGY_CELL, value );
        Serial_debug(DEBUG_INFO, &cli_serial, "S_MIN_VOLTAGE_ENERGY_CELL: 0x%x\r\n", value);
        break;
    case S_MAX_VOLTAGE_ENERGY_CELL:
        value = convert_voltage_energy_cell_to_OD( sharedVars_cpu1toCpu2.max_allowed_voltage_energy_cell );
        retVal = coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_MAX_VOLTAGE_ENERGY_CELL, value );

        Serial_debug(DEBUG_INFO, &cli_serial, "S_MAX_VOLTAGE_ENERGY_CELL: 0x%x\r\n", value);
        break;
    default:
        if((subIndex >= S_STATE_OF_CHARGE_OF_ENERGY_CELL_01) && (subIndex <= S_STATE_OF_CHARGE_OF_ENERGY_CELL_30))
        {
            value = convert_voltage_energy_cell_to_OD( sharedVars_cpu2toCpu1.soc_energy_cell[subIndex-3]);
            retVal = coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, subIndex, value );
//            Serial_debug( DEBUG_INFO, &cli_serial, "S_STATE_OF_CHARGE_OF_ENERGY_CELL_%d: 0x%x\r\n",
//                    subIndex - S_MAX_VOLTAGE_ENERGY_CELL, value);
        } else if((subIndex >= S_STATE_OF_HEALTH_OF_ENERGY_CELL_01) && (subIndex <= S_STATE_OF_HEALTH_OF_ENERGY_CELL_30))
        {
            retVal = coOdGetObj_u8(I_ENERGY_CELL_SUMMARY, subIndex, &value);
//            Serial_debug( DEBUG_INFO, &cli_serial, "S_STATE_OF_HEALTH_OF_ENERGY_CELL_%d: 0x%x\r\n",
//                    subIndex - S_MAX_VOLTAGE_ENERGY_CELL, value);
        } else
        {
            Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD SUBINDEX: 0x%x\r\n", subIndex);
        }
        break;
    }

    return retVal;
}

static inline uint8_t indices_I_TEMPERATURE(UNSIGNED8 subIndex)
{
    uint8_t retVal = CO_FALSE;

    switch (subIndex)
    {
    case S_DPMU_TEMPERATURE_MAX_LIMIT:
        coOdPutObj_u8(I_TEMPERATURE, S_DPMU_TEMPERATURE_MAX_LIMIT, temperature_absolute_max_limit );
        Serial_debug(DEBUG_INFO, &cli_serial, "S_MAX_ALLOWED_DPMU_TEMPERATURE: 0x%x\r\n", temperature_absolute_max_limit);
        break;
    case S_TEMPERATURE_MEASURED_AT_DPMU_HOTTEST_POINT:
        coOdPutObj_i8( I_TEMPERATURE, S_TEMPERATURE_MEASURED_AT_DPMU_HOTTEST_POINT, temperatureHotPoint);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_TEMPERATURE_MEASURED_AT_DPMU_HOTTEST_POINT: 0x%x\r\n", temperatureHotPoint);
        break;
    case S_DPMU_TEMPERATURE_HIGH_LIMIT:
        coOdPutObj_u8(I_TEMPERATURE, S_DPMU_TEMPERATURE_HIGH_LIMIT, temperature_high_limit );
        Serial_debug(DEBUG_INFO, &cli_serial, "S_DPMU_TEMPERATURE_HIGH_LIMIT: 0x%x\r\n", temperature_high_limit);
        break;
    case S_TEMPERATURE_BASE:
        coOdPutObj_i8( I_TEMPERATURE, S_TEMPERATURE_BASE, temperatureSensorVector[TEMPERATURE_SENSOR_BASE]);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_TEMPERATURE_BASE: 0x%x\r\n", temperatureSensorVector[TEMPERATURE_SENSOR_BASE]);
        break;
    case S_TEMPERATURE_MAIN:
        coOdPutObj_i8( I_TEMPERATURE, S_TEMPERATURE_MAIN, temperatureSensorVector[TEMPERATURE_SENSOR_MAIN] );
        Serial_debug(DEBUG_INFO, &cli_serial, "S_TEMPERATURE_MAIN: 0x%x\r\n", temperatureSensorVector[TEMPERATURE_SENSOR_MAIN]);
        break;
    case S_TEMPERATURE_MEZZANINE:
        coOdPutObj_i8( I_TEMPERATURE, S_TEMPERATURE_MEZZANINE, temperatureSensorVector[TEMPERATURE_SENSOR_MEZZANINE] );
        Serial_debug(DEBUG_INFO, &cli_serial, "S_TEMPERATURE_MEZZANINE: 0x%x\r\n", temperatureSensorVector[TEMPERATURE_SENSOR_MEZZANINE]);
        break;
    case S_TEMPERATURE_PWR_BANK:
        coOdPutObj_i8( I_TEMPERATURE, S_TEMPERATURE_PWR_BANK, temperatureSensorVector[TEMPERATURE_SENSOR_PWR_BANK] );
        Serial_debug(DEBUG_INFO, &cli_serial, "S_TEMPERATURE_PWR_BANK: 0x%x\r\n", temperatureSensorVector[TEMPERATURE_SENSOR_PWR_BANK]);
        break;

    default:
        Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD SUBINDEX: 0x%x\r\n", subIndex);
    }

    return retVal;
}

static inline uint8_t indices_I_MAXIMUM_ALLOWED_LOAD_POWER(void)
{
    uint8_t retVal = CO_FALSE;
    uint16_t value16;

    value16 = convert_power_to_OD( sharedVars_cpu1toCpu2.max_allowed_load_power );
    retVal = coOdPutObj_u16(I_MAXIMUM_ALLOWED_LOAD_POWER, 0, value16);

    Serial_debug(DEBUG_INFO, &cli_serial, "I_MAXIMUM_ALLOWED_LOAD_POWER: 0x%x\r\n", value16);

    return retVal;
}

static inline uint8_t indices_I_POWER_BUDGET_DC_INPUT(UNSIGNED8 subIndex)
{
    uint8_t retVal = CO_FALSE;
    uint16_t value16;

    switch (subIndex)
    {
    case S_AVAILABLE_POWER_BUDGET_DC_INPUT:
        value16 = convert_power_to_OD( sharedVars_cpu1toCpu2.available_power_budget_dc_input );
        retVal = coOdPutObj_u16(I_POWER_BUDGET_DC_INPUT, S_AVAILABLE_POWER_BUDGET_DC_INPUT, value16);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_AVAILABLE_POWER_BUDGET_DC_INPUT: 0x%x\r\n", value16);
        break;
//    case S_MAX_CURRENT_POWER_LINE_A:
//        retVal = coOdGetObj_u16(I_POWER_BUDGET_DC_INPUT, S_MAX_CURRENT_POWER_LINE_A, &value);
//        Serial_debug(DEBUG_INFO, &cli_serial, "S_MAX_CURRENT_POWER_LINE_A: 0x%x\r\n", value);
//        break;
//    case S_MAX_CURRENT_POWER_LINE_B:
//        retVal = coOdGetObj_u16(I_POWER_BUDGET_DC_INPUT, S_MAX_CURRENT_POWER_LINE_B, &value);
//        Serial_debug(DEBUG_INFO, &cli_serial, "S_MAX_CURRENT_POWER_LINE_B: 0x%x\r\n", value);
//        break;
    default:
        Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD SUBINDEX: 0x%x\r\n", subIndex);
    }

    return retVal;
}

static inline uint8_t indices_I_CHARGE_FACTOR(void)
{
    uint8_t retVal = CO_FALSE;
    uint8_t value;

    retVal = coOdGetObj_u8(I_CHARGE_FACTOR, 0, &value);
    Serial_debug(DEBUG_INFO, &cli_serial, "I_CHARGE_FACTOR: 0x%x\r\n", value);

    return retVal;
}

static inline uint8_t indices_I_READ_POWER(UNSIGNED8 subIndex)
{
    uint8_t retVal = CO_FALSE;
    uint8_t value8;
    uint16_t value16;
    int8_t intValue8;
    float realValue;

    switch (subIndex)
    {
    case S_READ_VOLTAGE_AT_DC_BUS:
        realValue = sharedVars_cpu2toCpu1.voltage_at_dc_bus;
        value8 = convert_dc_bus_voltage_to_OD( realValue );
        retVal = coOdPutObj_u8(I_READ_POWER, S_READ_VOLTAGE_AT_DC_BUS,  value8 );
        Serial_debug(DEBUG_INFO, &cli_serial, "S_READ_VOLTAGE_AT_DC_BUS: %d\r\n", value8 );
        break;

    case S_POWER_FROM_DC_INPUT:

        realValue = sharedVars_cpu2toCpu1.power_from_dc_input;
        value16 = convert_power_to_OD( realValue );
        retVal = coOdPutObj_u16(I_READ_POWER, S_POWER_FROM_DC_INPUT,  value16 );
        Serial_debug(DEBUG_INFO, &cli_serial, "S_POWER_FROM_DC_INPUT: %d\r\n", value16);
        break;

    case S_READ_LOAD_CURRENT:
        realValue = sharedVars_cpu2toCpu1.current_load_current;
        intValue8 = convert_dc_load_current_to_OD( realValue );
        retVal = coOdPutObj_i8( I_READ_POWER, S_READ_LOAD_CURRENT, intValue8 );
        Serial_debug(DEBUG_INFO, &cli_serial,
                     "S_READ_LOAD_CURRENT: %d\r\n", intValue8);
        break;

    case S_POWER_CONSUMED_BY_LOAD:
        realValue = sharedVars_cpu2toCpu1.current_load_current * sharedVars_cpu2toCpu1.voltage_at_dc_bus;
        value16 = convert_power_to_OD( realValue );
        retVal = coOdPutObj_u16(I_READ_POWER, S_POWER_CONSUMED_BY_LOAD, value16);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_POWER_CONSUMED_BY_LOAD: %d\r\n", value16);
        break;

    default:
        Serial_debug(DEBUG_ERROR, &cli_serial,
                     "UNKNOWN CAN OD SUBINDEX: 0x%x\r\n", subIndex);
    }

    return retVal;
}

static inline uint8_t indices_I_DPMU_STATE(UNSIGNED8 subIndex)
{
    uint8_t retVal = CO_FALSE;
    uint8_t value;

    switch (subIndex)
    {
    case S_DPMU_OPERATION_REQUEST_STATE:
        retVal = coOdGetObj_u8(I_DPMU_STATE, S_DPMU_OPERATION_REQUEST_STATE, &value);
        //Serial_debug(DEBUG_INFO, &cli_serial, "S_DPMU_OPERATION_REQUEST_STATE: 0x%x\r\n", value);
        break;
    case S_DPMU_OPERATION_CURRENT_STATE:
        retVal = coOdPutObj_u8(I_DPMU_STATE, S_DPMU_OPERATION_CURRENT_STATE, sharedVars_cpu2toCpu1.current_state);
        //Serial_debug(DEBUG_INFO, &cli_serial, "S_DPMU_OPERATION_CURRENT_STATE: 0x%x\r\n", sharedVars_cpu2toCpu1.current_state);
        break;
    default:
        Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD SUBINDEX: 0x%x\r\n", subIndex);
    }

    return retVal;
}

static inline uint8_t indices_I_ENERGY_BANK_SUMMARY(UNSIGNED8 subIndex)
{
    uint8_t retVal = CO_FALSE;
    uint8_t value;
    uint16_t value16;

    switch (subIndex)
    {
    case S_MAX_VOLTAGE_APPLIED_TO_ENERGY_BANK:
        value = convert_voltage_energy_bank_to_OD( sharedVars_cpu1toCpu2.max_voltage_applied_to_energy_bank );
        retVal = coOdPutObj_u8(I_ENERGY_BANK_SUMMARY, S_MAX_VOLTAGE_APPLIED_TO_ENERGY_BANK, value );
        Serial_debug(DEBUG_INFO, &cli_serial, "S_MAX_VOLTAGE_APPLIED_TO_STORAGE_BANK: 0x%x\r\n", value);
        break;
    case S_MIN_VOLTAGE_APPLIED_TO_ENERGY_BANK:
        value = convert_min_voltage_applied_to_energy_bank_to_OD( sharedVars_cpu1toCpu2.min_voltage_applied_to_energy_bank );
        retVal = coOdPutObj_u8(I_ENERGY_BANK_SUMMARY, S_MIN_VOLTAGE_APPLIED_TO_ENERGY_BANK, value );
        Serial_debug(DEBUG_INFO, &cli_serial, "S_MIN_ALLOWED_STATE_OF_CHARGE_OF_ENERGY_BANK: 0x%x\r\n", value);
        break;
    case S_SAFETY_THRESHOLD_STATE_OF_CHARGE:
        value16 = convert_energy_soc_energy_bank_to_OD( sharedVars_cpu1toCpu2.safety_threshold_state_of_charge );
        retVal = coOdPutObj_u16(I_ENERGY_BANK_SUMMARY, S_SAFETY_THRESHOLD_STATE_OF_CHARGE, value16 );
        Serial_debug(DEBUG_INFO, &cli_serial, "S_SAFETY_THRESHOLD_STATE_OF_CHARGE: xx%x\r\n", value16);
        break;
    case S_STATE_OF_CHARGE_OF_ENERGY_BANK:
        value = convert_energy_soc_energy_bank_to_OD( sharedVars_cpu2toCpu1.soc_energy_bank );
        retVal = coOdPutObj_u8(I_ENERGY_BANK_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_BANK, value );
        Serial_debug(DEBUG_INFO, &cli_serial, "S_STATE_OF_CHARGE_OF_ENERGY_BANK: 0x%x\r\n", value);
        break;
    case S_STATE_OF_HEALTH_OF_ENERGY_BANK:
        value = convert_soh_energy_bank_to_OD( sharedVars_cpu2toCpu1.soh_energy_bank );
        retVal = coOdPutObj_u8(I_ENERGY_BANK_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_BANK, value);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_STATE_OF_HEALTH_OF_ENERGY_BANK: 0x%x\r\n", value);
        break;
    case S_REMAINING_ENERGY_TO_MIN_SOC_AT_ENERGY_BANK:
        value = convert_soh_energy_bank_to_OD( sharedVars_cpu2toCpu1.remaining_energy_to_min_soc_energy_bank );
        retVal = coOdPutObj_u16(I_ENERGY_BANK_SUMMARY, S_REMAINING_ENERGY_TO_MIN_SOC_AT_ENERGY_BANK, value);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_REMAINING_ENERGY_TO_MIN_SOC_AT_ENERGY_BANK: 0x%x\r\n", value);
        break;
    case S_PRECONDITIONAL_THRESHOLD:
        value = convert_voltage_energy_bank_to_OD( sharedVars_cpu1toCpu2.preconditional_threshold );
        retVal = coOdPutObj_u8(I_ENERGY_BANK_SUMMARY, S_PRECONDITIONAL_THRESHOLD, value );
        Serial_debug(DEBUG_INFO, &cli_serial, "S_PRECONDITIONAL_THRESHOLD: xx%x\r\n", value);
        break;
    case S_STACK_TEMPERATURE:
        retVal = coOdPutObj_i8(I_ENERGY_BANK_SUMMARY, S_STACK_TEMPERATURE, temperatureSensorVector[TEMPERATURE_SENSOR_PWR_BANK]);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_STACK_TEMPERATURE: xx%x\r\n", temperatureSensorVector[TEMPERATURE_SENSOR_PWR_BANK]);
        break;
    default:
        Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD SUBINDEX: 0x%x\r\n", subIndex);
    }

    return retVal;
}

#define GLOAD_4_3_EPWMA_GPIO 161
#define GLOAD_4_3_EPWMB_GPIO 162
static inline uint8_t indices_I_SWITCH_STATE(UNSIGNED8 subIndex)
{
    uint8_t retVal = CO_FALSE;
    uint8_t state;

    //Serial_debug(DEBUG_INFO, &cli_serial, "SWITCH  S 0x%0x  STATE ", subIndex);

    switch (subIndex)
    {
    case S_SW_QINRUSH_STATE:
        //TODO - same similar for inrush current limiter
        state = GPIO_readPin(168);//Qinrush);
        retVal = coOdPutObj_u8(I_SWITCH_STATE, S_SW_QINRUSH_STATE, state); /*get unsigned integer 8 bits*/
//        if (retVal == RET_OK)
//            switches_Qinrush_digital(state);
        break;
    case S_SW_QLB_STATE:
        state = GPIO_readPin(Qlb);
        retVal = coOdPutObj_u8(I_SWITCH_STATE, S_SW_QLB_STATE, state);
//        if (retVal == RET_OK)
//            switches_Qlb(state);
        break;
    case S_SW_QSB_STATE:
        state = GPIO_readPin(Qsb);
        retVal = coOdPutObj_u8(I_SWITCH_STATE, S_SW_QSB_STATE, state);
//        if (retVal == RET_OK)
//            switches_Qsb(state);
        break;
    case S_SW_QINB_STATE:
        state = GPIO_readPin(Qinb);
        retVal = coOdPutObj_u8(I_SWITCH_STATE, S_SW_QINB_STATE, state);
//        if (retVal == RET_OK)
//            switches_Qinb(state);
        break;
    default:
        Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD SUBINDEX: 0x%x\r\n", subIndex);
    }

    //Serial_debug(DEBUG_INFO, &cli_serial, "STATE %s\r\n", (state ? "ON": "OFF"));

    return retVal;
}

static inline uint8_t indices_I_DPMU_POWER_SOURCE_TYPE(void)
{
    uint8_t retVal = CO_FALSE;
    uint8_t value;

    retVal = coOdGetObj_u8(I_DPMU_POWER_SOURCE_TYPE, 0, &value);
    Serial_debug(DEBUG_INFO, &cli_serial, "I_DPMU_POWER_SOURCE_TYPE: 0x%x\r\n", value);

    return retVal;
}

//static inline uint8_t indices_I_ISSUE_REBOOT(void)
//{
//    uint8_t retVal = CO_FALSE;
//    uint8_t value;
//
//    retVal = coOdGetObj_u8(I_ISSUE_REBOOT, 0, &value);
//    Serial_debug(DEBUG_INFO, &cli_serial, "I_ISSUE_REBOOT: 0x%x\r\n", value);
//    if(value)
//    {
//        Serial_debug(DEBUG_INFO, &cli_serial, "REBOOTING\r\n", value);
//        SysCtl_resetDevice();
//    } else
//    {
//        Serial_debug(DEBUG_INFO, &cli_serial, "NO REBOOTING\r\n", value);
//    }
//
//    return retVal;
//}

static inline uint8_t indices_I_DEBUG_LOG(BOOL_T execute, UNSIGNED8 sdoNr, UNSIGNED16  index, UNSIGNED8 subIndex)
{
    uint8_t retVal = CO_FALSE;
    uint8_t value = 0xff;

    Serial_debug(DEBUG_INFO, &cli_serial, "DEBUG_LOG  S 0x%0x  ", subIndex);

    switch (subIndex)
    {
    case S_DEBUG_LOG_STATE:
        retVal = coOdGetObj_u8(I_DEBUG_LOG, S_DEBUG_LOG_STATE, &value); /*get unsigned integer 8 bits*/
        if (retVal == RET_OK)
            log_debug_log_set_state(value);
        break;
    case S_DEBUG_LOG_READ:
        retVal = log_debug_log_read(execute, sdoNr, index, subIndex);
        break;
//    case S_DEBUG_LOG_RESET:
//        retVal = coOdGetObj_u8(I_DEBUG_LOG, S_DEBUG_LOG_RESET, &value);
//        if (retVal == RET_OK)
//            log_debug_log_reset(value);
//        break;
    default:
        Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD SUBINDEX: 0x%02x\r\n", subIndex);
    }

    Serial_debug(DEBUG_INFO, &cli_serial, "VALUE %x\r\n", value);

    return retVal;
}

static inline uint8_t indices_I_CAN_LOG(BOOL_T execute, UNSIGNED8 sdoNr, UNSIGNED16  index, UNSIGNED8 subIndex)
{
    uint8_t retVal = CO_FALSE;
    uint8_t value  = 0xff;

    Serial_debug(DEBUG_INFO, &cli_serial, "CAN_LOG  S 0x%0x  ", subIndex);

    switch (subIndex)
    {
    case S_CAN_LOG_RESET:

        log_can_log_reset();
        retVal = CO_TRUE;
        break;
    case S_CAN_LOG_READ:
        retVal = log_can_log_read(execute, sdoNr, index, subIndex);
        break;
    default:
        Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD SUBINDEX: 0x%02x\r\n", subIndex);
    }

    Serial_debug(DEBUG_INFO, &cli_serial, "VALUE %x\r\n", value);

    return retVal;
}

RET_T co_usr_sdo_ul_indices(
        BOOL_T      execute,
        UNSIGNED8   sdoNr,
        UNSIGNED16  index,
        UNSIGNED8   subIndex
    )
{
    static uint32_t sdoUploadCounter = 0;
    uint8_t retVal = RET_OK;

    if ((execute == CO_TRUE))
    {
        sdoUploadCounter++;
        Serial_debug(DEBUG_INFO, &cli_serial, "SDO <- [%lu]\r", sdoUploadCounter);

        switch (index)
        {
        case I_TPDO_MAPPING_TEMPERATURES:
            retVal = indices_I_TPDO_MAPPING_TEMPERATURES(subIndex);
            break;
        case I_STORE_PARAMETERS:
            retVal = indices_I_STORE_PARAMETERS(subIndex);
            break;
        case I_RESTORE_DEFAULT_PARAMETERS:
            retVal = indices_I_RESTORE_DEFAULT_PARAMETERS(subIndex);
            break;
        case I_DATE_AND_TIME:
            retVal = indices_I_DATE_AND_TIME();
            break;
        case I_SET_NODEID:
            retVal = indices_I_SET_NODEID();
            break;
        case I_DC_BUS_VOLTAGE:
            retVal = indices_I_DC_BUS_VOLTAGE(subIndex);
            break;
        case I_ESS_CURRENT:
            retVal = indices_I_ESS_CURRENT();
            break;
        case I_ENERGY_CELL_SUMMARY:
            retVal = indices_I_ENERGY_CELL_SUMMARY(subIndex);
            break;
        case I_TEMPERATURE:
            retVal = indices_I_TEMPERATURE(subIndex);
            break;
        case I_MAXIMUM_ALLOWED_LOAD_POWER:
            retVal = indices_I_MAXIMUM_ALLOWED_LOAD_POWER();
            break;
        case I_POWER_BUDGET_DC_INPUT:
            retVal = indices_I_POWER_BUDGET_DC_INPUT(subIndex);
            break;
        case I_CHARGE_FACTOR:
            retVal = indices_I_CHARGE_FACTOR();
            break;
        case I_READ_POWER:
            retVal = indices_I_READ_POWER(subIndex);
            break;
        case I_DPMU_STATE:
            retVal = indices_I_DPMU_STATE(subIndex);
            break;
        case I_ENERGY_BANK_SUMMARY:
            retVal = indices_I_ENERGY_BANK_SUMMARY(subIndex);
            break;
        case I_SWITCH_STATE:
            retVal = indices_I_SWITCH_STATE(subIndex);
            break;
        case I_DPMU_POWER_SOURCE_TYPE:
            retVal = indices_I_DPMU_POWER_SOURCE_TYPE();
            break;
//        case I_ISSUE_REBOOT:
//            retVal = indices_I_ISSUE_REBOOT();
//            break;
        case I_DEBUG_LOG:
            retVal = indices_I_DEBUG_LOG(execute, sdoNr, index, subIndex);
            break;
        case I_CAN_LOG:
            retVal = indices_I_CAN_LOG(execute, sdoNr, index, subIndex);
            break;
        default:
            Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD INDEX: 0x%04x 0x%02x\r\n", index, subIndex);
        }

        /* store transfered data in log */
        log_sent_data[0] = 'R';
        log_sent_data[1] = index & 0xff;
        log_sent_data[2] = (index>>8) & 0xff;
        log_sent_data[3] = subIndex;
        log_sent_data[4] = 0;
        log_sent_data[5] = 0;
        log_sent_data[6] = 0;
        log_sent_data[7] = 0;

        /* Retrieve payload, data[4..7] */
        CO_CONST CO_OBJECT_DESC_T *pObjDesc;
        coOdGetObjDescPtr(index, subIndex, &pObjDesc);
        getObjData(pObjDesc, &log_sent_data[4], index, subIndex);
        //log_store_can_log(8, log_sent_data);
    } else
    {
//        Serial_debug(DEBUG_ERROR, &cli_serial, "SDO <- ");
//
//        switch (index)
//        {
//        case I_DEBUG_LOG:
//            retVal = indices_I_DEBUG_LOG(execute, sdoNr, index, subIndex);
//            break;
//        case I_CAN_LOG:
//            retVal = indices_I_CAN_LOG(execute, sdoNr, index, subIndex);
//            break;
//        default:
//            Serial_debug(DEBUG_ERROR, &cli_serial, "EXEC = FALSE : UNKNOWN CAN OD INDEX: 0x%x 0x%x\r\n", index, subIndex);
//        }
    }

    return (RET_T)retVal;
}


#endif /* APP_SRC_CANOPEN_INDICES_C_ */
