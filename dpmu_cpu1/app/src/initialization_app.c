/*
 * initialization_app.c
 *
 *  Created on: 28 de mai de 2024
 *      Author: gferreira
 */

#include <stdbool.h>
#include "initialization_app.h"
#include "shared_variables.h"

bool DPMUAppInfoInitialized[IDX_DPMU_VAR_COUNT] = { false };

void checkDPMUAppInfoInitializeVars(uint16_t DPMUappVarIdx ) {
    uint16_t idx = 0;
    if( DPMUappVarIdx < IDX_DPMU_VAR_COUNT ) {
        DPMUAppInfoInitialized[DPMUappVarIdx] = true;
    }

    for( idx = 0; idx< IDX_DPMU_VAR_COUNT; idx++) {
        if( DPMUAppInfoInitialized[idx] == false ) {
            break;
        }
    }
    if( idx == IDX_DPMU_VAR_COUNT ) {
        sharedVars_cpu1toCpu2.DPMUAppInfoInitializedFlag = true;
    } else {
        sharedVars_cpu1toCpu2.DPMUAppInfoInitializedFlag = false;
    }
}
