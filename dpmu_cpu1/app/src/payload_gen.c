/*
 * payload_gen.c
 *
 *  Created on: 2 jan. 2024
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#include <stdint.h>

#include "convert.h"
#include "shared_variables.h"

void payload_gen_load_current(uint8_t status, uint8_t ampere[4])
{
    int8_t current_load_current = convert_ess_current_to_OD( sharedVars_cpu2toCpu1.current_load_current );
    ampere[0] = (current_load_current >> 0) & 0xff;
    ampere[1] = 0;
    ampere[2] = 0;
    ampere[3] = 0;
}

void payload_gen_bus_voltage(uint8_t status, uint8_t voltage[4])
{
    uint8_t volt = convert_dc_bus_voltage_to_OD(sharedVars_cpu2toCpu1.voltage_at_dc_bus);

    voltage[0] = (volt >> 0) & 0xff;
    voltage[1] = 0;
    voltage[2] = 0;
    voltage[3] = 0;
}
