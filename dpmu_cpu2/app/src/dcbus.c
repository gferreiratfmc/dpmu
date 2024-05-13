/*
 * dcbus.c
 *
 *  Created on: 25 okt. 2023
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#include <error_handling.h>
#include "sensors.h"
#include "shared_variables.h"

extern volatile uint16_t efuse_top_half_flag;

bool test_update_of_error_codes = false;

void dcbus_update_settings(void)
{
    /* set bus Voltage reference */
    DCDC_VI.target_Voltage_At_DCBus = sharedVars_cpu1toCpu2.target_voltage_at_dc_bus;
    //DCDC_VI.target_Voltage_At_DCBus = 100.0;

    /******* Load **************/
    /* set max allowed load power */
    /*TODO ? calculated from sharedVars_cpu1toCpu2.Max_Allowed_Load_Power ? */
    if(DCDC_VI.target_Voltage_At_DCBus)    /* do execute if denominator is zero */
        CLLC_VI.I_Ref_Raw = sharedVars_cpu1toCpu2.max_allowed_load_power / DCDC_VI.target_Voltage_At_DCBus;

    /******* Input *************/
    /* set available input power budget */
    if(DCDC_VI.target_Voltage_At_DCBus)    /* do execute if denominator is zero */
        DCDC_VI.iIn_limit = sharedVars_cpu1toCpu2.available_power_budget_dc_input / DCDC_VI.target_Voltage_At_DCBus;
}

void dcbus_check(void)
{
    if(test_update_of_error_codes)
    {
//        test_update_of_error_codes = false;
    /* check for DC bus over Voltage */
    if( sensorVector[VBusIdx].realValue > (float)sharedVars_cpu1toCpu2.max_allowed_dc_bus_voltage)
        sharedVars_cpu2toCpu1.error_code |=  (1UL << ERROR_BUS_OVER_VOLTAGE);
    else
        sharedVars_cpu2toCpu1.error_code &= ~(1UL << ERROR_BUS_OVER_VOLTAGE);

    /* check for DC bus under Voltage */
    if( sensorVector[VBusIdx].realValue < (float)sharedVars_cpu1toCpu2.min_allowed_dc_bus_voltage)
        sharedVars_cpu2toCpu1.error_code |=  (1UL << ERROR_BUS_UNDER_VOLTAGE);
    else
        sharedVars_cpu2toCpu1.error_code &= ~(1UL << ERROR_BUS_UNDER_VOLTAGE);

    /* check for DC bus shortage */
    if( sensorVector[VBusIdx].realValue < (float)sharedVars_cpu1toCpu2.vdc_bus_short_circuit_limit)
        sharedVars_cpu2toCpu1.error_code |=  (1UL << ERROR_BUS_SHORT_CIRCUIT);
    else
        sharedVars_cpu2toCpu1.error_code &= ~(1UL << ERROR_BUS_SHORT_CIRCUIT);

    /* check for DC bus load power */
    if( ( sensorVector[VBusIdx].realValue * sensorVector[ISen1fIdx].realValue ) > (float)sharedVars_cpu1toCpu2.max_allowed_load_power)
        sharedVars_cpu2toCpu1.error_code |=  (1UL << ERROR_CONSUMED_POWER_TO_HIGH);
    else
        sharedVars_cpu2toCpu1.error_code &= ~(1UL << ERROR_CONSUMED_POWER_TO_HIGH);

    /* check for DC bus load current */
    //if( sensorVector[ISen1fIdx].realValue > (float)sharedVars_cpu1toCpu2.max_allowed_load_current || efuse_top_half_flag == true )
    if( sensorVector[ISen1fIdx].realValue > ((float)sharedVars_cpu1toCpu2.max_allowed_load_power / (float)sharedVars_cpu1toCpu2.target_voltage_at_dc_bus)  || efuse_top_half_flag == true )
        sharedVars_cpu2toCpu1.error_code |=  (1UL << ERROR_LOAD_OVER_CURRENT);
    else
        sharedVars_cpu2toCpu1.error_code &= ~(1UL << ERROR_LOAD_OVER_CURRENT);

    /* check for DC bus input power */
    if((sensorVector[VBusIdx].realValue * sensorVector[IF_1fIdx].realValue) > (float)sharedVars_cpu1toCpu2.available_power_budget_dc_input)
        sharedVars_cpu2toCpu1.error_code |=  (1UL << ERROR_INPUT_POWER_TO_HIGH);
    else
        sharedVars_cpu2toCpu1.error_code &= ~(1UL << ERROR_INPUT_POWER_TO_HIGH);
    }
    sharedVars_cpu2toCpu1.voltage_at_dc_bus = DCDC_VI.avgVBus;
    sharedVars_cpu2toCpu1.power_from_dc_input = DCDC_VI.avgVBus * sensorVector[IF_1fIdx].realValue;
    sharedVars_cpu2toCpu1.current_charging_current = sensorVector[ISen2fIdx].realValue;
    sharedVars_cpu2toCpu1.current_load_current = sensorVector[ISen1fIdx].realValue;



}
