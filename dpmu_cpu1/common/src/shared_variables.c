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

#pragma RETAIN(sharedVars_cpu2toCpu1)
#pragma DATA_SECTION(sharedVars_cpu2toCpu1, "sharedVars_cpu2toCpu1")
sharedVars_cpu2toCpu1_t sharedVars_cpu2toCpu1 = {0};
