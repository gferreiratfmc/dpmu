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

// Called from SDO_Download_Indices.
// Sets a flag in the DPMUAppIndoInitialized array for each initialized variable.
// After all variables are set informs CPU2 through a shared flag from CPU1 to CPU2 - DPMUAppInfoInitializedFlag
// The parameter is the index in the array for the variable being set
void CheckDPMUAppInfoInitializeVars(DPMU_Initialization_Vars_t DPMUappVarIdx ) {
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


void ResetDPMUAppInfoInitializeVars() {
    sharedVars_cpu1toCpu2.DPMUAppInfoInitializedFlag = false;
    for( int idx=0; idx<IDX_DPMU_VAR_COUNT; idx++ ) {
        DPMUAppInfoInitialized[idx] = false;
    }
}


//Checks if any specific variable is initialized
bool VerifyAppInfoVarInitialized( DPMU_Initialization_Vars_t DPMUappVarIdx ) {
    return DPMUAppInfoInitialized[DPMUappVarIdx];
}

void InitializeCPU1ToCPU2SharedVariables( void ) {
    sharedVars_cpu1toCpu2.iop_operation_request_state = Idle;
    sharedVars_cpu1toCpu2.supercap_short_circuit_current = DPMU_SUPERCAP_SHORT_CIRCUIT_CURRENT;
    sharedVars_cpu1toCpu2.input_short_circuit_current = DPMU_SHORT_CIRCUIT_CURRENT;
    sharedVars_cpu1toCpu2.output_short_circuit_current = DPMU_SHORT_CIRCUIT_CURRENT;
}
