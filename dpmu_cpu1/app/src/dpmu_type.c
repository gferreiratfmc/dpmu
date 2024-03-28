/*
 * dpmu_type.c
 *
 *  Created on: 25 sep. 2023
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#include <stdbool.h>
#include <stdint.h>

#include "co_canopen.h"
#include "dpmu_type.h"
#include "gen_indices.h"


bool dpmu_type_having_battery(void)
{
    bool ret;
    UNSIGNED8 dpmu_type;

    coOdGetObj_u8(I_DPMU_POWER_SOURCE_TYPE, 0, &dpmu_type);

    /* check if DPMU is of a allowed type to connect through the shared bus */
    switch (dpmu_type)
    {
//    case DPMU_DEFAULT_CAP:    /* alone, nothing to connect with */
//    case DPMU_DEFAULT_CAP_W_REDUNTANT:
//    case DPMU_DEFAULT_CAP_W_SUPPLEMENTARY:
//    case DPMU_DEFAULT_CAP_W_REDUNDANT_SUPPLEMENTARY:
    case DPMU_DEFAULT_BAT:    /* alone, nothing to connect with */
    case DPMU_DEFAULT_BAT_W_REDUNTANT:
    case DPMU_DEFAULT_BAT_W_SUPPLEMENTARY:
    case DPMU_DEFAULT_BAT_W_REDUNDANT_SUPPLEMENTARY:
//    case DPMU_REDUNDANT_CAP:
//    case DPMU_REDUNDANT_CAP_W_SUPPLEMENTARY:
    case DPMU_REDUNDANT_BAT:
    case DPMU_REDUNDANT_BAT_W_SUPPLEMENTARY:
//    case DPMU_SUPPLEMENTARY_CAP:
    case DPMU_SUPPLEMENTARY_BAT:
        ret = true;
        break;
    default:
        ret = false;
        break;
    }

    return ret;
}

bool dpmu_type_having_caps(void)
{
    bool ret;
    UNSIGNED8 dpmu_type;

    coOdGetObj_u8(I_DPMU_POWER_SOURCE_TYPE, 0, &dpmu_type);

    /* check if DPMU is of a allowed type to connect through the shared bus */
    switch (dpmu_type)
    {
    case DPMU_DEFAULT_CAP:    /* alone, nothing to connect with */
    case DPMU_DEFAULT_CAP_W_REDUNTANT:
    case DPMU_DEFAULT_CAP_W_SUPPLEMENTARY:
    case DPMU_DEFAULT_CAP_W_REDUNDANT_SUPPLEMENTARY:
//    case DPMU_DEFAULT_BAT:    /* alone, nothing to connect with */
//    case DPMU_DEFAULT_BAT_W_REDUNTANT:
//    case DPMU_DEFAULT_BAT_W_SUPPLEMENTARY:
//    case DPMU_DEFAULT_BAT_W_REDUNDANT_SUPPLEMENTARY:
    case DPMU_REDUNDANT_CAP:
    case DPMU_REDUNDANT_CAP_W_SUPPLEMENTARY:
//    case DPMU_REDUNDANT_BAT:
//    case DPMU_REDUNDANT_BAT_W_SUPPLEMENTARY:
    case DPMU_SUPPLEMENTARY_CAP:
//    case DPMU_SUPPLEMENTARY_BAT:
        ret = true;
        break;
    default:
        ret = false;
        break;
    }

    return ret;
}

bool dpmu_type_allowed_to_use_shared_bus(void)
{
    bool ret;
    UNSIGNED8 dpmu_type;

    coOdGetObj_u8(I_DPMU_POWER_SOURCE_TYPE, 0, &dpmu_type);

    /* check if DPMU is of a allowed type to connect through the shared bus */
    switch (dpmu_type)
    {
//    case DPMU_DEFAULT_CAP:    /* alone, nothing to connect with */
    case DPMU_DEFAULT_CAP_W_REDUNTANT:
    case DPMU_DEFAULT_CAP_W_SUPPLEMENTARY:
    case DPMU_DEFAULT_CAP_W_REDUNDANT_SUPPLEMENTARY:
//    case DPMU_DEFAULT_BAT:    /* alone, nothing to connect with */
    case DPMU_DEFAULT_BAT_W_REDUNTANT:
    case DPMU_DEFAULT_BAT_W_SUPPLEMENTARY:
    case DPMU_DEFAULT_BAT_W_REDUNDANT_SUPPLEMENTARY:
    case DPMU_REDUNDANT_CAP:
    case DPMU_REDUNDANT_CAP_W_SUPPLEMENTARY:
    case DPMU_REDUNDANT_BAT:
    case DPMU_REDUNDANT_BAT_W_SUPPLEMENTARY:
    case DPMU_SUPPLEMENTARY_CAP:
    case DPMU_SUPPLEMENTARY_BAT:
        ret = true;
        break;
    default:
        ret = false;
        break;
    }

    return ret;
}

bool dpmu_type_allowed_to_use_load_bus(void)
{
    uint8_t dpmu_type;
    bool ret;

    /* read DPMU type fron CANopen OD */
    coOdGetObj_u8(I_DPMU_POWER_SOURCE_TYPE, 0, &dpmu_type);

    /* check if we are allowed to use the LOAD bus */
    switch (dpmu_type)
    {
    case DPMU_DEFAULT_CAP: /* default DPMU with capacitors */
    case DPMU_DEFAULT_CAP_W_REDUNTANT: /* redundant DPMU with capacitors, with redundant DPMU */
    case DPMU_DEFAULT_CAP_W_SUPPLEMENTARY: /* redundant DPMU with capacitors, with supplementary DPMU */
    case DPMU_DEFAULT_CAP_W_REDUNDANT_SUPPLEMENTARY: /* redundant DPMU with capacitors, with redundant DPMU and with supplementary DPMU */
    case DPMU_DEFAULT_BAT: /* default DPMU with batteries  */
    case DPMU_DEFAULT_BAT_W_REDUNTANT: /* redundant DPMU with batteries , with supplementary DPMU */
    case DPMU_DEFAULT_BAT_W_SUPPLEMENTARY: /* redundant DPMU with batteries , with redundant DPMU */
    case DPMU_DEFAULT_BAT_W_REDUNDANT_SUPPLEMENTARY: /* redundant DPMU with capacitors, with redundant DPMU and with supplementary DPMU */
//    case DPMU_REDUNDANT_CAP:                 /* redundant DPMU with capacitors */
//    case DPMU_REDUNDANT_CAP_W_SUPPLEMENTARY: /* redundant DPMU with capacitors, with supplementary DPMU */
//    case DPMU_REDUNDANT_BAT:                 /* redundant DPMU with batteries  */
//    case DPMU_REDUNDANT_BAT_W_SUPPLEMENTARY: /* redundant DPMU with batteries , with supplementary DPMU */
//    case DPMU_SUPPLEMENTARY_CAP: /* supplementary DPMU with capacitors in parallel with default or redundant DPMU */
//    case DPMU_SUPPLEMENTARY_BAT: /* supplementary DPMU with batteries  in parallel with default or redundant DPMU */

        ret = true;
        break;

    default:
        ret = false;
        break;
    }

    return ret;
}


