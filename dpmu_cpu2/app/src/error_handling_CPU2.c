/*
 * error_handling.c
 *
 *  Created on: 18 de jun de 2024
 *      Author: gferreira
 */

#include <stdbool.h>

#include "common.h"
#include "cli_cpu2.h"
#include "error_handling.h"
#include "error_handling_CPU2.h"
#include "sensors.h"
#include "shared_variables.h"

bool dpmuErrorOcurredFlag = false;


bool DpmuErrorOcurred() {
    return dpmuErrorOcurredFlag;
}

void ResetDpmuErrorOcurred() {
    dpmuErrorOcurredFlag = false;
}


void HandleLoadOverCurrent(float max_allowed_load_current, uint16_t efuse_top_half_flag)
{
    /* check for DC bus load current */
    if (sensorVector[ISen1fIdx].realValue > (float) max_allowed_load_current|| efuse_top_half_flag == true) {
        PRINT("sensorVector[ISen1fIdx]:[%5.2f] max_allowed_load_current:[%5.2f] ",sensorVector[ISen1fIdx].realValue, max_allowed_load_current);
        sharedVars_cpu2toCpu1.error_code |= (1UL << ERROR_LOAD_OVER_CURRENT);
        dpmuErrorOcurredFlag = true;
    } else {
        sharedVars_cpu2toCpu1.error_code &= ~(1UL << ERROR_LOAD_OVER_CURRENT);
    }
}
