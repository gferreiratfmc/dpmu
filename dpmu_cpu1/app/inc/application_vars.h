/*
 * application_vars.h
 *
 *  Created on: 16 de jul de 2024
 *      Author: gferreira
 */

#include <stdbool.h>
#include "GlobalV.h"

#ifndef APP_INC_APPLICATION_VARS_H_
#define APP_INC_APPLICATION_VARS_H_

typedef enum {
    MANUFACTURER_PARAMETER_CRC_CPU = 0,
}  manufacturer_parameter_idx_t;

typedef enum {
    AppVarsRead,
    AppVarsWrite,
    AppVarsWait
} app_vars_cmd_t;

typedef enum {
    CapacitanceAppVar,
    SerialNumberAppVar,
    AllAppVars
} app_vars_type_t;

void ResetHandleAppVarsOnExternalFlashSM();
void AppVarsReadRequest();
bool AppVarsReadRequestReady();
void AppVarsSaveRequest(app_vars_t *newAppVarsToSave, app_vars_type_t appVarType);
bool AppVarsSaveRequestReady();
void HandleAppVarsOnExternalFlashSM();
app_vars_t* GetCurrentAppVars();
bool RetriveSerialNumberFromFlash( uint32_t *serialNumber32bits );
void SaveSerialNumberToFlash( uint32_t serNumberCMD );
void AppVarsInformEntireFlashResetInitiated();
void AppVarsInformEntireFlashResetReady();
void RestoreManucfturerDefinedParemters( uint32_t parameterIndex, uint32_t *outputValue );

#endif /* APP_INC_APPLICATION_VARS_H_ */
