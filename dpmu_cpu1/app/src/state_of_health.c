/*
 * stateOfHealth.c
 *
 *  Created on: 23 de mai de 2024
 *      Author: gferreira
 */


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "application_vars.h"
#include "serial.h"
#include "common.h"
#include "ext_flash.h"
#include "shared_variables.h"
#include "state_of_health.h"
#include "timer.h"

extern struct Serial cli_serial;

#define SIZE_OF_BUFFER 32
uint16_t buff[SIZE_OF_BUFFER];


void RequestSaveNewCapacitanceToExtFlash( float initialCapacitance, float currentCapacitance ) {
    static app_vars_t newAppVars;
    newAppVars.initialCapacitance = initialCapacitance;
    newAppVars.currentCapacitance = currentCapacitance;
    AppVarsSaveRequest( &newAppVars, CapacitanceAppVar );
}
//void saveCapacitanceToFlash(float initialCapacitance, float currentCapacitance ) {
//    ext_flash_desc_t *app_vars_ext_flash_desc;
//    memcpy( buff, (void *)&initialCapacitance, sizeof(float) );
//    memcpy( buff+sizeof(float), (void *)&currentCapacitance, sizeof(float) );
//
//    app_vars_ext_flash_desc = (ext_flash_desc_t *)ext_flash_sector_from_address(APP_VARS_EXT_FLASH_ADDRESS_START);
//    ext_flash_erase_sector(app_vars_ext_flash_desc->sector);
//    ext_flash_write_buf(APP_VARS_EXT_FLASH_ADDRESS_START, buff, 2*sizeof(float) );
//
//}

void RetrieveInitalCapacitanceFromFlash( float *initialCapacitance, float *currentCapacitance ) {
    app_vars_t *appVars;

    if( AppVarsReadRequestReady() ) {
        appVars = GetCurrentAppVars();
        *initialCapacitance = appVars->initialCapacitance;
        *currentCapacitance = appVars->currentCapacitance;
    } else {
        *initialCapacitance = 0.0;
        *currentCapacitance = 0.0;
    }
}




//void RetrieveInitalCapacitanceFromFlash( float *initialCapacitance, float *currentCapacitance ) {
//    uint16_t invalidInfoCount = 0;
//    memset( buff, 0, SIZE_OF_BUFFER );
//    ext_flash_read_buf(APP_VARS_EXT_FLASH_ADDRESS_START, buff, 2*sizeof(float) );
//
//    for( int i=0;i<2*sizeof(float);i++ ) {
//        if( buff[i] == 0xFFFF ){
//            invalidInfoCount++;
//        }
//    }
//    if( invalidInfoCount == 2*sizeof(float) ) {
//        *initialCapacitance = 0.0;
//        *currentCapacitance = 0.0;
//    } else {
//        memcpy( initialCapacitance, buff, sizeof(float) );
//        memcpy( currentCapacitance, buff+sizeof(float), sizeof(float) );
//    }
//
//
//}
