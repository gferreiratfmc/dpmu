/*
 * shared_variables.c
 *
 *  Created on: 22 juni 2023
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#include "shared_variables.h"

#pragma RETAIN(sharedVars_cpu1toCpu2)
#pragma DATA_SECTION(sharedVars_cpu1toCpu2, "sharedVars_cpu1toCpu2")
sharedVars_cpu1toCpu2_t sharedVars_cpu1toCpu2 = {0};

#pragma DATA_SECTION(energy_bank_condition_from_flash, "MSGRAM_CPU1_TO_CPU2")
energy_bank_condition_t energy_bank_condition_from_flash;

#pragma RETAIN(sharedVars_cpu2toCpu1)
#pragma DATA_SECTION(sharedVars_cpu2toCpu1, "sharedVars_cpu2toCpu1")
sharedVars_cpu2toCpu1_t sharedVars_cpu2toCpu1 = {0};


#pragma DATA_SECTION(ipc_soh_msg, "MSGRAM_CPU2_TO_CPU1")
char ipc_soh_msg[128];
