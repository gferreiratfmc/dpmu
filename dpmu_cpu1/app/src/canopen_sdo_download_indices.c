/*
 * canopen_sdo_download_indices.c
 *
 *  Created on: 15 apr. 2023
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#ifndef APP_SRC_CANOPEN_INDICES_C_
#define APP_SRC_CANOPEN_INDICES_C_

#include <assert.h>
#include <dpmu_type.h>
#include <stdbool.h>
#include <stdint.h>

#include "cli_cpu1.h"
#include "co_datatype.h"
#include "co_odaccess.h"
#include "common.h"
#include "convert.h"
#include "ext_flash.h"
#include "gen_indices.h"
#include "log.h"
#include "main.h"
#include "node_id.h"
#include "serial.h"
#include "shared_variables.h"
#include "savedobjs.h"
#include "canopen_params_flash.h"
#include "temperature_sensor.h"
#include "timer.h"

//static save_od_t savedData[sizeof(saveObj) / sizeof(save_od_t)];
static uint16_t savedCnt;

/* one CAN/CANopen message */
#pragma DATA_ALIGN(log_received_data, 4)
unsigned char log_received_data[8];

/**
 * \brief   Store OD parameters in external flash sector 0.
 *
 * See Doc No SPC70057501, appendix A, DPMU Object Dictionary.
 *
 * \param   sIndex    sub index for object 0x1010 (Store Parameters)
 */
static RET_T indices_I_STORE_PARAMETERS(UNSIGNED8 subIndex)
{
    RET_T retVal = RET_OK;
    uint16_t startIdx, lastIdx;
    uint32_t nrOfObj;

    Serial_debug(DEBUG_INFO, &cli_serial, "Store Params indication 0x%x -- ", subIndex);

    // TODO: Store values in designated area of external flash.
    switch (subIndex)
    {
    case S_SAVE_ALL_PARAMETERS:
        Serial_debug(DEBUG_INFO, &cli_serial, "S_SAVE_ALL_PARAMETERS\r\n");
        startIdx = 0x1000;
        lastIdx = 0xffff;
        break;

    case S_SAVE_COMMUNICATION_PARAMETERS:
        Serial_debug(DEBUG_INFO, &cli_serial, "S_SAVE_COMMUNICATION_PARAMETERS\r\n");
        startIdx = 0x1000;
        lastIdx = 0x1fff;
        break;

    case S_SAVE_APPLICATION_PARAMETERS:
        Serial_debug(DEBUG_INFO, &cli_serial, "S_SAVE_APPLICATION_PARAMETERS\r\n");
        startIdx = 0x6000;
        lastIdx = 0x9FFF;
        break;

    case S_SAVE_MANUFACTURER_DEFINED_PARAMETERS:
        Serial_debug(DEBUG_INFO, &cli_serial, "S_SAVE_MANUFACTURER_DEFINED_PARAMETERS\r\n");
        startIdx = 0x2000;
        lastIdx = 0x5fff;
        break;

    default:
        Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD SUBINDEX!\r\n");
        // Setting these to the same value will skip the for loop below.
        startIdx = 0x1000;
        lastIdx = 0x1000;
        retVal = RET_SUBIDX_NOT_FOUND;
        break;
    }

    // Get number of saved objects.
//    nrOfObj = sizeof(saveObj) / sizeof(save_od_t);
    nrOfObj = get_num_saved_objs();

    savedCnt = 0;

    // First we read all objects saved in external flash, excluding the ones to be updated.
    od_range_t excluding = { .begin = startIdx, .end = lastIdx };
    int read_result = read_params_from_ext_flash(savedData, &excluding);
    if (read_result >= 0) {
        savedCnt = read_result;
    }

    // For all saveable objects.
    for (uint16_t i = 0; i < nrOfObj; ++i) {
        uint16_t index = saveObj[i].index;
        uint8_t subIndex = saveObj[i].subIndex;

        if (index >= startIdx && index <= lastIdx) {
            const CO_OBJECT_DESC_T *pDesc;

            // Get object description.
            if (coOdGetObjDescPtr(index, subIndex, &pDesc) != RET_OK) {
                continue;
            }

            // Get size of object from object description.
            uint32_t size = coOdGetObjSize(pDesc);

            // Get pointer to object.
            void *pObj = coOdGetObjAddr(index, subIndex);
            if (pObj == NULL) {
                Serial_debug(DEBUG_ERROR, &cli_serial, "%x not found\r\n", index);
                return RET_IDX_NOT_FOUND;
            }

            Serial_debug(DEBUG_ERROR, &cli_serial, "Store %x:%d, size %ld\r\n", index, subIndex, size);

            // Get data type of this index.
            switch (size) {
            case 1:
            case 2:
            case 4:
                savedData[savedCnt].index = index;
                savedData[savedCnt].subIndex = subIndex;
                savedData[savedCnt].size = size;
                coNumMemcpy(&savedData[savedCnt].value, pObj, size, CO_ATTR_NUM);
                savedCnt++;
                break;

            default:
                Serial_debug(DEBUG_ERROR, &cli_serial, "SDO invalid value (size)\r\n");
                return RET_SDO_INVALID_VALUE;
            }
        }
    }

    write_params_to_ext_flash(savedData, savedCnt);

    return retVal;
}

/**
 * Restore OD parameters from external flash sector 0.
 *
 * See Doc No SPC70057501, appendix A, DPMU Object Dictionary.

 * Current state of OD parameters will be overwritten by previously stored state.
 */
static RET_T indices_I_RESTORE_DEFAULT_PARAMETERS(UNSIGNED8 sIndex)
{
    RET_T retVal = RET_OK;
    uint16_t startIdx, lastIdx;
    uint32_t value;

    switch (sIndex)
    {
    case S_RESTORE_ALL_DEFAULT_PARAMETERS:
        startIdx = 0x1000;
        lastIdx = 0xffff;
        retVal = coOdGetObj_u32(I_RESTORE_DEFAULT_PARAMETERS, S_RESTORE_ALL_DEFAULT_PARAMETERS, &value);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_RESTORE_ALL_DEFAULT_PARAMETERS: 0x%x\r\n", value);
        break;

    case S_RESTORE_COMMUNICATION_DEFAULT_PARAMETERS:
        startIdx = 0x1000;
        lastIdx = 0x1fff;
        retVal = coOdGetObj_u32(I_RESTORE_DEFAULT_PARAMETERS, S_RESTORE_COMMUNICATION_DEFAULT_PARAMETERS, &value);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_RESTORE_COMMUNICATION_DEFAULT_PARAMETERS: 0x%x\r\n", value);
        break;

    case S_RESTORE_APPLICATION_DEFAULT_PARAMETERS:
        startIdx = 0x6000;
        lastIdx = 0x9FFF;
        retVal = coOdGetObj_u32(I_RESTORE_DEFAULT_PARAMETERS, S_RESTORE_APPLICATION_DEFAULT_PARAMETERS, &value);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_RESTORE_APPLICATION_DEFAULT_PARAMETERS: 0x%x\r\n", value);
        break;

    case S_RESTORE_MANUFACTURER_DEFINED_DEFAULT_PARAMETERS:
        startIdx = 0x2000;
        lastIdx = 0x5fff;
        retVal = coOdGetObj_u32(I_RESTORE_DEFAULT_PARAMETERS, S_RESTORE_MANUFACTURER_DEFINED_DEFAULT_PARAMETERS, &value);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_RESTORE_MANUFACTURER_DEFINED_DEFAULT_PARAMETERS: 0x%x\r\n", value);
        break;

    default:
        Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD SUBINDEX: 0x%x\r\n", sIndex);
        // Setting these to the same value will skip the for loop below.
        startIdx = 0x1000;
        lastIdx = 0x1000;
        retVal = RET_SUBIDX_NOT_FOUND;
    }

    savedCnt = read_params_from_ext_flash(savedData, NULL);

    for (int i = 0; i < savedCnt; i++) {
        uint16_t index = savedData[i].index;
        uint8_t subIndex = savedData[i].subIndex;
        uint32_t size = savedData[i].size;
        uint32_t value = savedData[i].value;

        if (index >= startIdx && index <= lastIdx) {
            const CO_OBJECT_DESC_T *pDesc;


            /* get object description */
            retVal = coOdGetObjDescPtr(index, subIndex, &pDesc);
            if (retVal != RET_OK)  {
                return retVal;
            }

            /* get pointer to object */
            void *pObj = coOdGetObjAddr(index, subIndex);
            if (pObj == NULL)  {
                return RET_IDX_NOT_FOUND;
            }

            Serial_debug(DEBUG_ERROR, &cli_serial, "Load %x:%d, size %ld\r\n", index, subIndex, size);

            /* get data type of this index */
            switch (size)  {
                case 1:
                case 2:
                case 4:
                    coNumMemcpy(pObj, &value, size, CO_ATTR_NUM);
                    break;

                default:
                    return RET_SDO_INVALID_VALUE;
            }
        }
    }

    return retVal;
}

static inline RET_T indices_I_DATE_AND_TIME(UNSIGNED8 subIndex)
{
    RET_T retVal = RET_OK;
    uint32_t value;

    retVal = coOdGetObj_u32(I_DATE_AND_TIME, 0, &value);
    Serial_debug(DEBUG_INFO, &cli_serial, "I_DATE_AND_TIME: 0x%08lx\r\n", value);

    timer_set_can_time(value);

    return retVal;
}

static RET_T indices_I_SET_NODEID(void)
{
    RET_T retVal = RET_OK;
    uint8_t value;

    retVal = coOdGetObj_u8(I_SET_NODEID, 0, &value);
    node_update_nodeID(value);
    Serial_debug(DEBUG_INFO, &cli_serial, "I_SET_NODEID: 0x%x\r\n", value);

    return retVal;
}

static RET_T indices_I_DC_BUS_VOLTAGE(UNSIGNED8 subIndex)
{
    RET_T retVal = RET_OK;
    uint8_t value;
    float   value_converted;

    switch (subIndex)
    {
    case S_MIN_ALLOWED_DC_BUS_VOLTAGE:
        retVal = coOdGetObj_u8(I_DC_BUS_VOLTAGE, S_MIN_ALLOWED_DC_BUS_VOLTAGE, &value);
        value_converted = convert_dc_bus_voltage_from_OD(value);
        sharedVars_cpu1toCpu2.min_allowed_dc_bus_voltage = value_converted;
        Serial_debug(DEBUG_INFO, &cli_serial, "S_MIN_ALLOWED_DC_BUS_VOLTAGE: 0x%x\r\n", value);
        break;
    case S_MAX_ALLOWED_DC_BUS_VOLTAGE:
        retVal = coOdGetObj_u8(I_DC_BUS_VOLTAGE, S_MAX_ALLOWED_DC_BUS_VOLTAGE, &value);
        value_converted = convert_dc_bus_voltage_from_OD(value);
        sharedVars_cpu1toCpu2.max_allowed_dc_bus_voltage = value_converted;
        Serial_debug(DEBUG_INFO, &cli_serial, "S_MAX_ALLOWED_DC_BUS_VOLTAGE: 0x%x\r\n", value);
        break;
    case S_TARGET_VOLTAGE_AT_DC_BUS:
        retVal = coOdGetObj_u8(I_DC_BUS_VOLTAGE, S_TARGET_VOLTAGE_AT_DC_BUS, &value);
        value_converted = convert_dc_bus_voltage_from_OD(value);
        sharedVars_cpu1toCpu2.target_voltage_at_dc_bus = value_converted;
        Serial_debug(DEBUG_INFO, &cli_serial, "S_TARGET_VOLTAGE_AT_DC_BUS: 0x%x\r\n", value);
        break;
    case S_VDC_BUS_SHORT_CIRCUIT_LIMIT:
        retVal = coOdGetObj_u8(I_DC_BUS_VOLTAGE, S_VDC_BUS_SHORT_CIRCUIT_LIMIT, &value);
        value_converted = convert_dc_bus_voltage_from_OD(value);
        sharedVars_cpu1toCpu2.vdc_bus_short_circuit_limit = value_converted;
        Serial_debug(DEBUG_INFO, &cli_serial, "S_VDC_BUS_SHORT_CIRCUIT_LIMIT: 0x%x\r\n", value);
        break;
    case S_VDROOP:
        retVal = coOdGetObj_u8(I_DC_BUS_VOLTAGE, S_VDROOP, &value);
        //TODO how the 'value' be converted ?
        value_converted = convert_dc_bus_voltage_from_OD(value);
        sharedVars_cpu1toCpu2.vdroop = value_converted;
        Serial_debug(DEBUG_INFO, &cli_serial, "S_VDROOP: 0x%x\r\n", value);
        break;
    default:
        Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD SUBINDEX: 0x%x\r\n", subIndex);
    }

    return retVal;
}

static RET_T indices_I_ESS_CURRENT(void)
{
    RET_T retVal = RET_OK;
    uint8_t value;
    float   value_converted;

    retVal = coOdGetObj_u8(I_ESS_CURRENT, 0, &value);
    value_converted = convert_ess_current_from_OD(value);
    sharedVars_cpu1toCpu2.ess_current = value_converted;
    Serial_debug(DEBUG_INFO, &cli_serial, "I_ESS_CURRENT: 0x%x\r\n", value);

    return retVal;
}

static RET_T indices_I_ENERGY_CELL_SUMMARY(UNSIGNED8 subIndex)
{
    RET_T retVal = RET_OK;
    uint8_t value;
    float   value_converted;

    switch (subIndex)
    {
    case S_MIN_VOLTAGE_ENERGY_CELL:
        retVal = coOdGetObj_u8(I_ENERGY_CELL_SUMMARY, S_MIN_VOLTAGE_ENERGY_CELL, &value);
        value_converted = convert_voltage_energy_cell_from_OD(value);
        if( value_converted <= MAX_VOLTAGE_ENERGY_CELL ) {
            sharedVars_cpu1toCpu2.min_allowed_voltage_energy_cell = value_converted;
            Serial_debug(DEBUG_INFO, &cli_serial, "S_MIN_VOLTAGE_ENERGY_CELL: 0x%x\r\n", value);
        } else {
            Serial_debug(DEBUG_INFO, &cli_serial, "INVALID S_MIN_VOLTAGE_ENERGY_CELL: %x > MAX_VOLTAGE_ENERGY_CELL\r\n", value);
            retVal = RET_SDO_INVALID_VALUE;
        }
        break;
    case S_MAX_VOLTAGE_ENERGY_CELL:
        retVal = coOdGetObj_u8(I_ENERGY_CELL_SUMMARY, S_MAX_VOLTAGE_ENERGY_CELL, &value);
        value_converted = convert_voltage_energy_cell_from_OD(value);
        if( value_converted <= MAX_VOLTAGE_ENERGY_CELL ) {
            sharedVars_cpu1toCpu2.max_allowed_voltage_energy_cell = value_converted;
            Serial_debug(DEBUG_INFO, &cli_serial, "S_MAX_VOLTAGE_ENERGY_CELL: 0x%x\r\n", value);
        } else {
            Serial_debug(DEBUG_INFO, &cli_serial, "INVALID S_MAX_VOLTAGE_ENERGY_CELL: %x > MAX_VOLTAGE_ENERGY_CELL\r\n", value);
            retVal = RET_SDO_INVALID_VALUE;
        }

        break;
    default:
        Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD SUBINDEX: 0x%x\r\n", subIndex);
        retVal = RET_SUBIDX_NOT_FOUND;
        break;
    }

    return retVal;
}

static RET_T indices_I_TEMPERATURE(UNSIGNED8 subIndex)
{
    RET_T retVal = RET_OK;
    int8_t value;

    switch (subIndex)
    {
    case S_DPMU_TEMPERATURE_MAX_LIMIT:
        retVal = coOdGetObj_i8(I_TEMPERATURE, S_DPMU_TEMPERATURE_MAX_LIMIT, &value);
        temperature_absolute_max_limit = value;
        Serial_debug(DEBUG_INFO, &cli_serial, "S_MAX_ALLOWED_DPMU_TEMPERATURE: 0x%x\r\n", value);
        break;
    case S_DPMU_TEMPERATURE_HIGH_LIMIT:
        retVal = coOdGetObj_i8(I_TEMPERATURE, S_DPMU_TEMPERATURE_HIGH_LIMIT, &value);
        temperature_high_limit = value;
        Serial_debug(DEBUG_INFO, &cli_serial, "S_DPMU_TEMPERATURE_HIGH_LIMIT: 0x%x\r\n", value);
        break;
    default:
        Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD SUBINDEX: 0x%x\r\n", subIndex);
        retVal = RET_SUBIDX_NOT_FOUND;
        break;
    }

    return retVal;
}

static RET_T indices_I_MAXIMUM_ALLOWED_LOAD_POWER(void)
{
    RET_T retVal = RET_OK;
    uint16_t value;
    float    value_converted;

    retVal = coOdGetObj_u16(I_MAXIMUM_ALLOWED_LOAD_POWER, 0, &value);
    value_converted = convert_power_from_OD(value);
    sharedVars_cpu1toCpu2.max_allowed_load_power = value_converted;
    Serial_debug(DEBUG_INFO, &cli_serial, "I_MAXIMUM_ALLOWED_LOAD_POWER: 0x%x\r\n", value);

    return retVal;
}

static RET_T indices_I_POWER_BUDGET_DC_INPUT(UNSIGNED8 subIndex)
{
    RET_T retVal = RET_OK;
    uint16_t value;
    float    value_converted;

    switch (subIndex)
    {
    case S_AVAILABLE_POWER_BUDGET_DC_INPUT:
        retVal = coOdGetObj_u16(I_POWER_BUDGET_DC_INPUT, S_AVAILABLE_POWER_BUDGET_DC_INPUT, &value);
        value_converted = convert_power_from_OD(value);
        sharedVars_cpu1toCpu2.available_power_budget_dc_input = value_converted;
        Serial_debug(DEBUG_INFO, &cli_serial, "S_AVAILABLE_POWER_BUDGET_DC_INPUT: 0x%x\r\n", value);
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
        retVal = RET_SUBIDX_NOT_FOUND;
        break;
    }

    return retVal;
}

static RET_T indices_I_CHARGE_FACTOR(void)
{
    RET_T retVal = RET_OK;
    uint8_t value;

    retVal = coOdGetObj_u8(I_CHARGE_FACTOR, 0, &value);
    Serial_debug(DEBUG_INFO, &cli_serial, "I_CHARGE_FACTOR: 0x%x\r\n", value);

    return retVal;
}

static inline RET_T indices_I_DPMU_STATE(UNSIGNED8 subIndex)
{
    RET_T retVal = RET_OK;
    uint8_t state;

    switch (subIndex)
    {
    case S_DPMU_OPERATION_REQUEST_STATE:
        retVal = coOdGetObj_u8(I_DPMU_STATE, S_DPMU_OPERATION_REQUEST_STATE, &state);
        Serial_debug(DEBUG_INFO, &cli_serial, "S_DPMU_OPERATION_REQUEST_STATE: 0x%x\r\n", state);
        sharedVars_cpu1toCpu2.iop_operation_request_state = state;
        IPC_setFlagLtoR(IPC_CPU1_L_CPU2_R, IPC_IOP_REQUEST_CHANGE_OF_STATE);
        break;
    default:
        Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD SUBINDEX: 0x%x\r\n", subIndex);
        retVal = RET_SUBIDX_NOT_FOUND;
        break;
    }

    return retVal;
}

static RET_T indices_I_ENERGY_BANK_SUMMARY(UNSIGNED8 subIndex)
{
    RET_T retVal = RET_OK;
    uint8_t value;
    uint16_t value16;
    float   value_converted;

    switch (subIndex)
    {
    case S_MAX_VOLTAGE_APPLIED_TO_ENERGY_BANK:
        retVal = coOdGetObj_u8(I_ENERGY_BANK_SUMMARY, S_MAX_VOLTAGE_APPLIED_TO_ENERGY_BANK, &value);
        value_converted = convert_voltage_energy_bank_from_OD(value);
        if( value_converted > 0.0 &&  value_converted <= MAX_VOLTAGE_ENERGY_BANK ) {
            sharedVars_cpu1toCpu2.max_voltage_applied_to_energy_bank = value_converted;
            Serial_debug(DEBUG_INFO, &cli_serial, "S_MAX_VOLTAGE_APPLIED_TO_STORAGE_BANK: 0x%x\r\n", value);
        } else {
            retVal = RET_SDO_INVALID_VALUE;
            Serial_debug(DEBUG_INFO, &cli_serial, "INVALID S_MAX_VOLTAGE_APPLIED_TO_STORAGE_BANK: 0x%x\r\n", value);
        }
        break;
    case S_MIN_VOLTAGE_APPLIED_TO_ENERGY_BANK:
        retVal = coOdGetObj_u8(I_ENERGY_BANK_SUMMARY, S_MIN_VOLTAGE_APPLIED_TO_ENERGY_BANK, &value);
        value_converted = convert_min_voltage_applied_to_energy_bank_from_OD(value);
        if( value_converted > 0.0 &&  value_converted <= MAX_VOLTAGE_ENERGY_BANK) {
            sharedVars_cpu1toCpu2.min_voltage_applied_to_energy_bank = value_converted;
            Serial_debug(DEBUG_INFO, &cli_serial, "S_MIN_ALLOWED_STATE_OF_CHARGE_OF_ENERGY_BANK: 0x%x\r\n", value);
        } else {
            retVal = RET_SDO_INVALID_VALUE;
            Serial_debug(DEBUG_INFO, &cli_serial, "INVALID S_MIN_ALLOWED_STATE_OF_CHARGE_OF_ENERGY_BANK: 0x%x\r\n", value);
        }

        break;
    case S_SAFETY_THRESHOLD_STATE_OF_CHARGE:
        retVal = coOdGetObj_u16(I_ENERGY_BANK_SUMMARY, S_SAFETY_THRESHOLD_STATE_OF_CHARGE, &value16);
        value_converted = convert_energy_soc_energy_bank_from_OD(value16);
        if( value_converted > 0.0 &&  value_converted <= MAX_VOLTAGE_ENERGY_BANK) {
            sharedVars_cpu1toCpu2.safety_threshold_state_of_charge = value_converted;
            Serial_debug(DEBUG_INFO, &cli_serial, "S_SAFETY_THRESHOLD_STATE_OF_CHARGE: 0x%x\r\n", value);
        } else {
            retVal = RET_SDO_INVALID_VALUE;
            Serial_debug(DEBUG_INFO, &cli_serial, "INVALID S_SAFETY_THRESHOLD_STATE_OF_CHARGE: 0x%x\r\n", value);
        }
        break;
//    case S_STATE_OF_CHARGE_OF_ENERGY_BANK:
//        retVal = coOdGetObj_u8(I_ENERGY_BANK_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_BANK, &value);
//        Serial_debug(DEBUG_INFO, &cli_serial, "S_STATE_OF_CHARGE_OF_ENERGY_BANK: 0x%x\r\n", value);
//        break;
//    case S_STATE_OF_HEALTH_OF_ENERGY_BANK:
//        retVal = coOdGetObj_u8(I_ENERGY_BANK_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_BANK, &value);
//        Serial_debug(DEBUG_INFO, &cli_serial, "S_STATE_OF_HEALTH_OF_ENERGY_BANK: 0x%x\r\n", value);
//        break;
//    case S_REMAINING_ENERGY_TO_MIN_SOC_AT_ENERGY_BANK:
//        retVal = coOdGetObj_u8(I_ENERGY_BANK_SUMMARY, S_REMAINING_ENERGY_TO_MIN_SOC_AT_ENERGY_BANK, &value);
//        Serial_debug(DEBUG_INFO, &cli_serial, "S_REMAINING_ENERGY_TO_MIN_SOC_AT_ENERGY_BANK: 0x%x\r\n", value);
//        break;
//    case S_STACK_TEMPERATURE:
//        retVal = coOdGetObj_u8(I_ENERGY_BANK_SUMMARY, S_STACK_TEMPERATURE, &value);
//        Serial_debug(DEBUG_INFO, &cli_serial, "S_STACK_TEMPERATURE: 0x%x\r\n", value);
//        break;
    case S_CONSTANT_VOLTAGE_THRESHOLD:
        retVal = coOdGetObj_u8(I_ENERGY_BANK_SUMMARY, S_CONSTANT_VOLTAGE_THRESHOLD, &value);
        value_converted = convert_voltage_energy_bank_from_OD(value);
        if( value_converted > 0.0 &&  value_converted <= MAX_VOLTAGE_ENERGY_BANK) {
            sharedVars_cpu1toCpu2.constant_voltage_threshold = value_converted;
            Serial_debug(DEBUG_INFO, &cli_serial, "S_CONSTANT_VOLTAGE_THRESHOLD: 0x%x\r\n", value);
        } else {
            retVal = RET_SDO_INVALID_VALUE;
            Serial_debug(DEBUG_INFO, &cli_serial, "INVALID S_CONSTANT_VOLTAGE_THRESHOLD: 0x%x\r\n", value);
        }
        break;
    case S_PRECONDITIONAL_THRESHOLD:
        retVal = coOdGetObj_u8(I_ENERGY_BANK_SUMMARY, S_PRECONDITIONAL_THRESHOLD, &value);
        value_converted = convert_voltage_energy_bank_from_OD(value);
        if( value_converted > 0.0 &&  value_converted <= MAX_VOLTAGE_ENERGY_BANK) {
            sharedVars_cpu1toCpu2.preconditional_threshold = value_converted;
            Serial_debug(DEBUG_INFO, &cli_serial, "S_PRECONDITIONAL_THRESHOLD: 0x%x\r\n", value);
        } else {
            retVal = RET_SDO_INVALID_VALUE;
            Serial_debug(DEBUG_INFO, &cli_serial, "INVALID S_PRECONDITIONAL_THRESHOLD: 0x%x\r\n", value);
        }
        break;
    default:
        Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD SUBINDEX: 0x%x\r\n", subIndex);
        retVal = RET_SUBIDX_NOT_FOUND;
        break;
    }

    return retVal;
}

static RET_T indices_I_SWITCH_STATE(UNSIGNED8 subIndex)
{
    RET_T retVal = RET_OK;
    uint8_t state;


    switch (subIndex)
    {
    case S_SW_QINRUSH_STATE:
        retVal = coOdGetObj_u8(I_SWITCH_STATE, S_SW_QINRUSH_STATE, &state); /*get unsigned integer 8 bits*/
        if (retVal == RET_OK)
        {
            /* send instruction to CPU2 */
//            cli_switches(IPC_SWITCHES_QIRS, state);
            sharedVars_cpu1toCpu2.QinrushSwitchRequestState = state;
            IPC_setFlagLtoR(IPC_CPU1_L_CPU2_R, IPC_SWITCHES_QIRS);
        }
        break;
    case S_SW_QLB_STATE:
        retVal = coOdGetObj_u8(I_SWITCH_STATE, S_SW_QLB_STATE, &state);
        if (retVal == RET_OK)
        {
            /* send instruction to CPU2 */
//            cli_switches(IPC_SWITCHES_QLB, state);
            sharedVars_cpu1toCpu2.QlbSwitchRequestState = state;
            IPC_setFlagLtoR(IPC_CPU1_L_CPU2_R, IPC_SWITCHES_QLB);
}
        break;
    case S_SW_QSB_STATE:
        retVal = coOdGetObj_u8(I_SWITCH_STATE, S_SW_QSB_STATE, &state);
        if (retVal == RET_OK)
        {
            /* send instruction to CPU2 */
            //cli_switches(IPC_SWITCHES_QSB, state);
            sharedVars_cpu1toCpu2.QsbSwitchRequestState = state;
            IPC_setFlagLtoR(IPC_CPU1_L_CPU2_R, IPC_SWITCHES_QSB);
        }
        break;
    case S_SW_QINB_STATE:
        retVal = coOdGetObj_u8(I_SWITCH_STATE, S_SW_QINB_STATE, &state);
        if (retVal == RET_OK)
        {
            /* send instruction to CPU2 */
//            cli_switches(IPC_SWITCHES_QINB, state);
            sharedVars_cpu1toCpu2.QinbSwitchRequestState = state;
            IPC_setFlagLtoR(IPC_CPU1_L_CPU2_R, IPC_SWITCHES_QINB);

        }
        break;
    default:
        Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD SUBINDEX: 0x%x\r\n", subIndex);
        retVal = RET_SUBIDX_NOT_FOUND;
        break;
    }

    Serial_debug(DEBUG_INFO, &cli_serial, "CANOPEN DOWNLOAD SWITCH  S 0x%0x  STATE ", subIndex);
    Serial_debug(DEBUG_INFO, &cli_serial, "STATE %s for switch %d\r\n", (state ? "ON": "OFF"), subIndex);

    return retVal;
}

static RET_T indices_I_DPMU_POWER_SOURCE_TYPE(void)
{
    RET_T retVal = RET_OK;
    uint8_t value;

    retVal = coOdGetObj_u8(I_DPMU_POWER_SOURCE_TYPE, 0, &value);
    Serial_debug(DEBUG_INFO, &cli_serial, "I_DPMU_POWER_SOURCE_TYPE: 0x%x\r\n", value);

    /* check and update if we are using battery */
    if(dpmu_type_having_battery())
        sharedVars_cpu1toCpu2.having_battery = true;
    else
        sharedVars_cpu1toCpu2.having_battery = false;

    sharedVars_cpu1toCpu2.dpmu_default_flag = dpmu_type_default();

    if( sharedVars_cpu1toCpu2.dpmu_default_flag == true) {
        Serial_debug( DEBUG_INFO, &cli_serial, "DPMU TYPE SET TO DEFAULT\r\n" );
    } else {
        Serial_debug( DEBUG_INFO, &cli_serial, "DPMU TYPE SET TO REDUNDANT\r\n" );
    }

    return retVal;
}

static RET_T indices_I_ISSUE_REBOOT(void)
{
    RET_T retVal = RET_OK;
    uint8_t value;

    retVal = coOdGetObj_u8(I_SEND_REBOOT_REQUEST, 0, &value);
    Serial_debug(DEBUG_INFO, &cli_serial, "I_ISSUE_REBOOT: 0x%x\r\n", value);
    if(0x5A == value)
    {
        Serial_debug(DEBUG_INFO, &cli_serial, "REBOOTING\r\n", value);
        SysCtl_resetDevice();
    } else
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "NO REBOOTING\r\n", value);
    }

    return retVal;
}

static RET_T indices_I_DEBUG_LOG(BOOL_T execute, UNSIGNED8 sdoNr, UNSIGNED16  index, UNSIGNED8 subIndex)
{
    RET_T retVal = RET_OK;
    uint8_t value;

    Serial_debug(DEBUG_INFO, &cli_serial, "DEBUG_LOG  S 0x%0x  ", subIndex);

    switch (subIndex)
    {
    case S_DEBUG_LOG_STATE:
        retVal = coOdGetObj_u8(I_DEBUG_LOG, S_DEBUG_LOG_STATE, &value); /*get unsigned integer 8 bits*/
        if (retVal == RET_OK) {
            log_debug_log_set_state(value);
            Serial_debug(DEBUG_INFO, &cli_serial, "S_DEBUG_LOG_STATE set state to %s\r\n", (value ? "ON": "OFF"));
        }
        break;
//    case S_DEBUG_LOG_READ:
//        retVal = log_debug_log_read(execute, sdoNr, index, subIndex);
//        break;
    case S_DEBUG_LOG_RESET:
        retVal = coOdGetObj_u8(I_DEBUG_LOG, S_DEBUG_LOG_RESET, &value);
        if (retVal == RET_OK) {
            log_debug_log_reset(value);
            Serial_debug(DEBUG_INFO, &cli_serial, "S_DEBUG_LOG_RESET\r\n");
        }
        break;
    default:
        Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD SUBINDEX: 0x%02x\r\n", subIndex);
        retVal = RET_SUBIDX_NOT_FOUND;
        break;
    }

    return retVal;
}

static RET_T indices_I_CAN_LOG(BOOL_T execute, UNSIGNED8 sdoNr, UNSIGNED16  index, UNSIGNED8 subIndex)
{
    RET_T retVal = RET_OK;

    Serial_debug(DEBUG_INFO, &cli_serial, "CAN_LOG  S 0x%0x  ", subIndex);

    switch (subIndex)
    {
//    case S_CAN_LOG_READ:
//        retVal = log_can_log_read(execute, sdoNr, index, subIndex);
//        break;
    case S_CAN_LOG_RESET:
        log_can_log_reset();
        Serial_debug(DEBUG_INFO, &cli_serial, "S_CAN_LOG_RESET\r\n");
        break;
    default:
        Serial_debug(DEBUG_ERROR, &cli_serial, "UNKNOWN CAN OD SUBINDEX: 0x%02x\r\n", subIndex);
        retVal = RET_SUBIDX_NOT_FOUND;
        break;
    }

    return retVal;
}

RET_T co_usr_sdo_dl_indices(
        BOOL_T      execute,
        UNSIGNED8   sdoNr,
        UNSIGNED16  index,
        UNSIGNED8   subIndex
    )
{
    RET_T retVal = RET_OK;

    if ((execute == CO_TRUE))
    {
        Serial_debug(DEBUG_INFO, &cli_serial, "SDO -> ");

        switch (index)
        {
        case I_STORE_PARAMETERS:
            retVal = indices_I_STORE_PARAMETERS(subIndex);
            break;
        case I_RESTORE_DEFAULT_PARAMETERS:
            retVal = indices_I_RESTORE_DEFAULT_PARAMETERS(subIndex);
            break;
        case I_DATE_AND_TIME:
            retVal = indices_I_DATE_AND_TIME(subIndex);
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
//        case I_READ_POWER:
//            retVal = indices_I_READ_POWER(subIndex);
//            break;
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
        case I_SEND_REBOOT_REQUEST:
            retVal = indices_I_ISSUE_REBOOT();
            break;
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
        log_received_data[0] = 'W';
        log_received_data[1] = index & 0xff;
        log_received_data[2] = (index>>8) & 0xff;
        log_received_data[3] = subIndex;
        log_received_data[4] = 0;
        log_received_data[5] = 0;
        log_received_data[6] = 0;
        log_received_data[7] = 0;

        /* Retrieve payload, data[4..7] */
        CO_CONST CO_OBJECT_DESC_T *pObjDesc;
        coOdGetObjDescPtr(index, subIndex, &pObjDesc);
        getObjData(pObjDesc, &log_received_data[4], index, subIndex);
        //log_store_can_log(8, log_received_data);
    }

    return retVal;
}


#endif /* APP_SRC_CANOPEN_INDICES_C_ */
