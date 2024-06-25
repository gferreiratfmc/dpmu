/*
 * dcbus.c
 *
 *  Created on: 25 okt. 2023
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */
#include "cli_cpu2.h"
#include "common.h"
#include "error_handling.h"
#include "error_handling_CPU2.h"
#include "GlobalV.h"
#include "sensors.h"
#include "shared_variables.h"
#include "timer.h"


extern volatile uint16_t efuse_top_half_flag;

bool test_update_of_error_codes = false;
float max_allowed_load_current = MAX_DPMU_OUTPUT_CURRENT;

void dcbus_update_settings(void)
{
    /* set bus Voltage reference */
    DCDC_VI.target_Voltage_At_DCBus = sharedVars_cpu1toCpu2.target_voltage_at_dc_bus;

    /******* Load **************/
    /* set max allowed load power */
    if(DCDC_VI.target_Voltage_At_DCBus > 0.0) {    /* do execute if denominator is zero */
        max_allowed_load_current = sharedVars_cpu1toCpu2.max_allowed_load_power / DCDC_VI.target_Voltage_At_DCBus;
//        if( (timer_get_ticks() % 2000)  < 1 ) {
//            PRINT("max_allowed_load_current[%5.2f],sharedVars_cpu1toCpu2.max_allowed_load_power[%5.2f],DCDC_VI.target_Voltage_At_DCBus[%5.2f]\r\n",
//                  max_allowed_load_current,sharedVars_cpu1toCpu2.max_allowed_load_power,DCDC_VI.target_Voltage_At_DCBus);
//        }
    } else {
        max_allowed_load_current = MAX_DPMU_OUTPUT_CURRENT;
    }

    /******* Input *************/
    /* set available input power budget */
    if(DCDC_VI.target_Voltage_At_DCBus > 0.0)    /* do execute if denominator is zero */
    {
        DCDC_VI.iIn_limit = sharedVars_cpu1toCpu2.available_power_budget_dc_input / DCDC_VI.target_Voltage_At_DCBus;
    } else {
        DCDC_VI.iIn_limit = 0.0;
    }
}


void dcbus_check(void)
{
    if(test_update_of_error_codes) {
    //        test_update_of_error_codes = false;
        /* check for DC bus over Voltage */
    //    if( sensorVector[VBusIdx].realValue > (float)sharedVars_cpu1toCpu2.max_allowed_dc_bus_voltage)
    //        sharedVars_cpu2toCpu1.error_code |=  (1UL << ERROR_BUS_OVER_VOLTAGE);
    //    else
    //        sharedVars_cpu2toCpu1.error_code &= ~(1UL << ERROR_BUS_OVER_VOLTAGE);
        HandleDCBusOverVoltage();


        /* check for DC bus under Voltage */
        HandleDCBusUnderVoltage();

        /* check for DC bus shortage */
        HandleDCBusShortCircuit();

        /* check for DC bus load power */
        if( ( sensorVector[VBusIdx].realValue * sensorVector[ISen1fIdx].realValue ) > (float)sharedVars_cpu1toCpu2.max_allowed_load_power)
            sharedVars_cpu2toCpu1.error_code |=  (1UL << ERROR_CONSUMED_POWER_TO_HIGH);
        else
            sharedVars_cpu2toCpu1.error_code &= ~(1UL << ERROR_CONSUMED_POWER_TO_HIGH);

        /* check for DC bus load current */
        HandleLoadOverCurrent(max_allowed_load_current, efuse_top_half_flag);

        /* Check for temperature max limit */
        HandleOverTemperature();

        /* check for DC bus input power */
        if((sensorVector[VBusIdx].realValue * sensorVector[IF_1fIdx].realValue) > (float)sharedVars_cpu1toCpu2.available_power_budget_dc_input)
            sharedVars_cpu2toCpu1.error_code |=  (1UL << ERROR_INPUT_POWER_TO_HIGH);
        else
            sharedVars_cpu2toCpu1.error_code &= ~(1UL << ERROR_INPUT_POWER_TO_HIGH);
    }
    sharedVars_cpu2toCpu1.voltage_at_dc_bus = DCDC_VI.avgVBus;
    sharedVars_cpu2toCpu1.voltage_at_storage_bank = DCDC_VI.avgVStore;
    sharedVars_cpu2toCpu1.power_from_dc_input = DCDC_VI.avgVBus * sensorVector[IF_1fIdx].realValue;
    sharedVars_cpu2toCpu1.current_charging_current = sensorVector[ISen2fIdx].realValue;
    sharedVars_cpu2toCpu1.current_load_current = sensorVector[ISen1fIdx].realValue;
    sharedVars_cpu2toCpu1.current_input_current = sensorVector[IF_1fIdx].realValue;



}
