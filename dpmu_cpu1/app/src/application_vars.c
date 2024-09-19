/*
 * application_vars.c
 *
 *  Created on: 15 de jul de 2024
 *      Author: gferreira
 */

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "application_vars.h"
#include "ext_flash.h"
#include "serial.h"

extern struct Serial cli_serial;


enum HAVSMStates { HAVWaitCommand = 0, HAVReadInit, HAVReadCurrentAppVars,
                   HAVSaveInit, HAVSaveNewAppVarsToFlash, HAVResetSM};
static States_t HAVSM = { 0 };

uint32_t appVarsNextFreeAddress = APP_VARS_EXT_FLASH_ADDRESS_START;
app_vars_t currentAppVars;
app_vars_t newAppVars;
bool currentAppVarsValid = false;
bool startReadAppVarsFlag = false;
bool startSaveAppVarsToFlashFlag = false;
bool newAppVarsAvailableFlag = false;
bool CANLogEntireFlashResetInitiated = false;
bool CANLogEntireFlashResetReady = false;

app_vars_type_t appVarTypeToSave;
app_vars_cmd_t appVarsCommand = AppVarsWait;


bool RetriveAppVarsFromExtFlash();
bool SaveNewAppVarsToExtFlash();
void PrepareNewAppVarToSave();

void AppVarsReadRequest() {
    appVarsCommand = AppVarsRead;
}

bool AppVarsReadRequestReady() {
    return currentAppVarsValid;
}

app_vars_t* GetCurrentAppVars() {
    return &currentAppVars;
}


void AppVarsSaveRequest(app_vars_t *newAppVarsToSave, app_vars_type_t appVarType) {
    appVarsCommand = AppVarsWrite;
    memcpy( &newAppVars, newAppVarsToSave, sizeof(app_vars_t) );
    newAppVarsAvailableFlag = true;
    currentAppVarsValid = false;
    appVarTypeToSave = appVarType;
}

bool AppVarsSaveRequestReady() {
    if( newAppVarsAvailableFlag == true ) {
        return false;
    } else {
        return true;
    }
}

void AppVarsInformEntireFlashResetInitiated(){
    CANLogEntireFlashResetInitiated = true;
}

void AppVarsInformEntireFlashResetReady(){
    CANLogEntireFlashResetReady = true;
}




void HandleAppVarsOnExternalFlashSM() {

    switch( HAVSM.State_Current ) {

        case HAVWaitCommand:
            switch ( appVarsCommand ) {
                case AppVarsRead:
                    HAVSM.State_Next = HAVReadInit;
                    break;
                case AppVarsWrite:
                    HAVSM.State_Next = HAVSaveInit;
                    break;
                case AppVarsWait:
                default:
                    HAVSM.State_Next = HAVWaitCommand;

            }
            appVarsCommand = AppVarsWait;

            break;

        case HAVReadInit:
            if( currentAppVarsValid == false ) {
                startReadAppVarsFlag = true;
                HAVSM.State_Next = HAVReadCurrentAppVars;
            } else {
                startReadAppVarsFlag = false;
                HAVSM.State_Next = HAVWaitCommand;
            }
            break;

        case HAVReadCurrentAppVars:
            if( RetriveAppVarsFromExtFlash() == true ) {
                //Serial_debug(DEBUG_INFO, &cli_serial, "HAVSM --> appVarsNextFreeAddress:[0x%08p]\r\n", appVarsNextFreeAddress);
                HAVSM.State_Next = HAVWaitCommand;
            }
            break;

        case HAVSaveInit:
            if( newAppVarsAvailableFlag == true ) {
                HAVSM.State_Next = HAVSaveNewAppVarsToFlash;
                startSaveAppVarsToFlashFlag = true;
            } else {
                HAVSM.State_Next = HAVWaitCommand;
            }
            break;

        case HAVSaveNewAppVarsToFlash:
            if( SaveNewAppVarsToExtFlash() == true ) {
                newAppVarsAvailableFlag = false;
                HAVSM.State_Next = HAVReadInit;
            }
            break;

        case HAVResetSM:
            if( CANLogEntireFlashResetReady == true ){
                appVarsNextFreeAddress = APP_VARS_EXT_FLASH_ADDRESS_START;
                currentAppVarsValid = false;
                startReadAppVarsFlag = false;
                startSaveAppVarsToFlashFlag = false;
                newAppVarsAvailableFlag = false;
                appVarsCommand = AppVarsWait;
                CANLogEntireFlashResetInitiated = false;
                CANLogEntireFlashResetReady = false;
                HAVSM.State_Next = HAVWaitCommand;
            }
            break;

        default:
            HAVSM.State_Next = HAVWaitCommand;
    }
    //if( HAVSM.State_Current != HAVSM.State_Next ) {
    //    Serial_debug(DEBUG_INFO, &cli_serial, "HAVSM.State_Current[%d] -> HAVSM.State_Next[%d]\r\n", HAVSM.State_Current, HAVSM.State_Next );
    //}
    if( CANLogEntireFlashResetInitiated == true){
        HAVSM.State_Next = HAVResetSM;
        CANLogEntireFlashResetReady = false;
    }
    HAVSM.State_Current = HAVSM.State_Next;

}

bool SaveNewAppVarsToExtFlash( ) {

    static bool retVal = false;
    enum RAVSMStates { SNAWaitStart = 0, SNAInit, SNAEraseSector, SNAWaitSectorEraseDone,
        SNAPrepareForWrite, SNAWrite, SNAEnd};
    static States_t SNASM = { 0 };
    static ext_flash_desc_t appVarsExtFlashDescToErase;
    uint16_t status;

    retVal = false;
    switch( SNASM.State_Current ) {

        case SNAWaitStart:
            if( startSaveAppVarsToFlashFlag == true ) {
                startSaveAppVarsToFlashFlag = false;
                SNASM.State_Next = SNAInit;
            }
            break;

        case SNAInit:
            if( appVarsNextFreeAddress == APP_VARS_EXT_FLASH_ADDRESS_START) {
                SNASM.State_Next = SNAEraseSector;
            } else {
                SNASM.State_Next = SNAPrepareForWrite;
            }
            break;

        case SNAEraseSector:
            memcpy( &appVarsExtFlashDescToErase,
                    ext_flash_sector_from_address( APP_VARS_EXT_FLASH_ADDRESS_START ),
                    sizeof(ext_flash_desc_t) );
            Serial_debug( DEBUG_INFO, &cli_serial, "Before appVarsExtFlashDescToErase addr:[0x%08p] sector:[%u]\r\n",
                          appVarsExtFlashDescToErase.addr, appVarsExtFlashDescToErase.sector );
            appVarsExtFlashDescToErase.addr =  APP_VARS_EXT_FLASH_ADDRESS_START;
            Serial_debug( DEBUG_INFO, &cli_serial, "After appVarsExtFlashDescToErase addr:[0x%08p] sector:[%u]\r\n",
                          appVarsExtFlashDescToErase.addr, appVarsExtFlashDescToErase.sector );
            ext_flash_erase_sector_by_descriptor( &appVarsExtFlashDescToErase );
            SNASM.State_Next = SNAWaitSectorEraseDone;
            break;

        case SNAWaitSectorEraseDone:
            if( ext_flash_ready() ) {
                SNASM.State_Next = SNAPrepareForWrite;
            }
            break;

        case SNAPrepareForWrite:
            PrepareNewAppVarToSave();
            Serial_debug(DEBUG_INFO, &cli_serial, "Saving to ext flash addr[0x%08p]\r\nnewAppVars.magicNumber:[0x%08p]\r\n",
                         appVarsNextFreeAddress, newAppVars.MagicNumber);
            Serial_debug(DEBUG_INFO, &cli_serial, "newAppVars.initialCapacitance[%6.2f], newAppVars.currentCapacitance:[%6.2f]\r\n",
                         newAppVars.initialCapacitance, newAppVars.currentCapacitance);
            Serial_debug(DEBUG_INFO, &cli_serial, "newAppVars.SerialNumber:[0..%d]:[", (SERIAL_NUMBER_SIZE_IN_CHARS-1) );
            for(int i=0;i<SERIAL_NUMBER_SIZE_IN_CHARS;i++) {
                Serial_debug(DEBUG_INFO, &cli_serial, "%d ", newAppVars.serialNumber[i]);
            }
            Serial_debug(DEBUG_INFO, &cli_serial, "]\r\n");
            ext_flash_init_non_blocking_write( appVarsNextFreeAddress, (uint16_t *)&newAppVars, sizeof(app_vars_t) );
            SNASM.State_Next = SNAWrite;
            break;

        case SNAWrite:
            status = ext_flash_non_blocking_write_buf();
            Serial_debug(DEBUG_INFO, &cli_serial, "ext_flash_non_blocking_write_buf status=[%d]\r\n", status);
            if( status ==  EXT_FLASH_BUF_WRITE_DONE || status == EXT_FLASH_BUF_WRITE_TIME_OUT) {
                SNASM.State_Next = SNAEnd;
            }
            break;
        case SNAEnd:
            retVal = true;
            SNASM.State_Next = SNAWaitStart;
            break;
        default:
            SNASM.State_Next = SNAWaitStart;

    }
    //if( SNASM.State_Current != SNASM.State_Next ) {
    //    Serial_debug(DEBUG_INFO, &cli_serial, "SNASM.State_Current[%d] -> SNASM.State_Next[%d]\r\n", SNASM.State_Current, SNASM.State_Next );
    //}
    SNASM.State_Current = SNASM.State_Next;
    return retVal;
}

bool RetriveAppVarsFromExtFlash() {
    enum RAVSMStates { RAVWaitStart = 0, RAVInit, RAVVerifyDataValidFromFlash, RAVNextAddress,
                            RAVVerifyNextMemoryEmptyInFlash, RAVEnd };
    static States_t RAVSM = { 0 };
    static bool validDataFound = false;
    static bool retValue = false;
    static uint32_t addr = APP_VARS_EXT_FLASH_ADDRESS_START;
    static app_vars_t emptyAppVars;

    switch ( RAVSM.State_Current) {
        case RAVWaitStart:
            retValue = false;
            if( startReadAppVarsFlag == true ) {
                startReadAppVarsFlag = false;
                RAVSM.State_Next = RAVInit;
            }
            break;

        case RAVInit:
            addr = APP_VARS_EXT_FLASH_ADDRESS_START;
            RAVSM.State_Next = RAVVerifyDataValidFromFlash;
            ext_flash_init_non_blocking_read(addr, (uint16_t *)&currentAppVars, sizeof(app_vars_t) );
            break;

        case RAVVerifyDataValidFromFlash:
            if( ext_flash_non_blocking_read_buf() == true ) {
                //Serial_debug(DEBUG_INFO, &cli_serial, "Read from flash addr:[0x%08p]\r\ncurrentAppVars MagicNumber[0x%08p]\r\n", addr, currentAppVars.MagicNumber );
                //Serial_debug(DEBUG_INFO, &cli_serial, "currentAppVars.initialCapacitance[%6.2f], currentAppVars.currentCapacitance:[%6.2f]\r\n",
                //             currentAppVars.initialCapacitance, currentAppVars.currentCapacitance);

                //Serial_debug(DEBUG_INFO, &cli_serial, "currentAppVars.SerialNumber:[0..%d]:[", (SERIAL_NUMBER_SIZE_IN_CHARS-1) );
                //for(int i=0;i<SERIAL_NUMBER_SIZE_IN_CHARS;i++) {
                //    Serial_debug(DEBUG_INFO, &cli_serial, "%d ", currentAppVars.serialNumber[i]);
                //}
                //Serial_debug(DEBUG_INFO, &cli_serial, "]\r\n");

                if( currentAppVars.MagicNumber == MAGIC_NUMBER) {
                    validDataFound = true;
                }
                RAVSM.State_Next = RAVNextAddress;
            }
            break;

        case RAVNextAddress:
            addr = addr + sizeof(app_vars_t);
            if( ( addr >= APP_VARS_EXT_FLASH_ADDRESS_END ) ||
                ( (addr + sizeof(app_vars_t) ) >= APP_VARS_EXT_FLASH_ADDRESS_END ) ) {
                appVarsNextFreeAddress = APP_VARS_EXT_FLASH_ADDRESS_START;
                RAVSM.State_Next = RAVEnd;
            } else {
                ext_flash_init_non_blocking_read(addr, (uint16_t *)&emptyAppVars, sizeof(app_vars_t) );
                if( validDataFound == true ) {
                    RAVSM.State_Next = RAVVerifyNextMemoryEmptyInFlash;
                } else {
                    RAVSM.State_Next = RAVVerifyDataValidFromFlash;
                }
            }
            break;

        case RAVVerifyNextMemoryEmptyInFlash:
            if( ext_flash_non_blocking_read_buf() == true ) {
                //Serial_debug(DEBUG_INFO, &cli_serial, "RAVVerifyNextMemoryEmptyInFlash addr:[0x%08p]\r\ncurrentAppVars MagicNumber[0x%08p]\r\n", addr, currentAppVars.MagicNumber );
                if( emptyAppVars.MagicNumber == 0xFFFFFFFF) {
                        appVarsNextFreeAddress =  addr;
                        RAVSM.State_Next = RAVEnd;
                } else {
                    validDataFound = false;
                    ext_flash_init_non_blocking_read(addr, (uint16_t *)&currentAppVars, sizeof(app_vars_t) );
                    RAVSM.State_Next = RAVVerifyDataValidFromFlash;
                }
            }
            break;

        case RAVEnd:
            if( validDataFound == true) {
                currentAppVarsValid = true;
            }
            retValue = true;
            RAVSM.State_Next = RAVWaitStart;
            break;
    }
    //if( RAVSM.State_Current != RAVSM.State_Next ) {
    //    Serial_debug(DEBUG_INFO, &cli_serial, "RAVSM.State_Current[%d] -> RAVSM.State_Next[%d]\r\n", RAVSM.State_Current, RAVSM.State_Next );
    //}
    RAVSM.State_Current = RAVSM.State_Next;

    return retValue;
}


bool RetriveSerialNumberFromFlash( uint32_t serNumberCMD, uint32_t *serialNumber32bits ){

    app_vars_t *appVars;
    static bool retVal;
    static uint16_t state = 0;
    static uint16_t timeoutCount = 0;
    uint8_t cmd, idx = 0;


    retVal = false;
    switch( state ) {
        case 0:
            AppVarsReadRequest();
            timeoutCount = 0;
            state = 1;
            break;
        case 1:
            if( AppVarsReadRequestReady() ) {
                appVars = GetCurrentAppVars();
                *serialNumber32bits = 0x00000000;
                cmd=(serNumberCMD & 0x000000FF);
                if( cmd <= SERIAL_NUMBER_SIZE_IN_CHARS/3 ) {
                    idx = cmd * 3;
                    for( int i=0; i<3; i++) {
                        idx=idx+i;
                        if( idx < SERIAL_NUMBER_SIZE_IN_CHARS ){
                            *serialNumber32bits = *serialNumber32bits | ( (uint32_t)( appVars->serialNumber[idx] ) & ( 0x000000FF << (i*8) ) );
                        }
                    }
                }
                *serialNumber32bits = *serialNumber32bits | ( (uint32_t)cmd  << 24 );
                state = 0;
                retVal = true;
            } else {
                timeoutCount++;
                retVal = false;
                if( timeoutCount == 20) {
                    *serialNumber32bits = 0x00000000;
                    state = 0;
                    retVal = true;
                }
            }
            break;
    }
    return retVal;
}


void SaveSerialNumberToFlash( uint32_t serNumberCMD ) {

    static app_vars_t newAppVarsSerialNumber;
    uint8_t cmd, idx;

    cmd = (unsigned char)((serNumberCMD & 0xFF000000) >> 24);

    Serial_debug(DEBUG_INFO, &cli_serial, "Received serialNumberCmd[%08p] decoded CMD:[%02d]\r\n", serNumberCMD, cmd);

    if( cmd < (SERIAL_NUMBER_SIZE_IN_CHARS/3) ) {
        idx = 3*cmd;
//        newAppVarsSerialNumber.serialNumber[idx] = (unsigned char)(serNumberCMD & 0x000000FF) ;
//        newAppVarsSerialNumber.serialNumber[idx+1] = (unsigned char)((serNumberCMD & 0x0000FF00) >> 8);
//        newAppVarsSerialNumber.serialNumber[idx+2] = (unsigned char)((serNumberCMD & 0x00FF0000) >> 16);
        for(int c = 0; c<3; c++){
            newAppVarsSerialNumber.serialNumber[idx+c] = (unsigned char)( ( serNumberCMD & (0x000000FF << (8*c) ) ) >> ( 8*c ) );
            Serial_debug(DEBUG_INFO, &cli_serial, "serialNumber[%02d]=[%c]\r\n", idx+c, newAppVarsSerialNumber.serialNumber[idx+c]);
        }
    } else if( cmd == 255){
        AppVarsSaveRequest(&newAppVarsSerialNumber, SerialNumberAppVar);
        Serial_debug(DEBUG_INFO, &cli_serial, "saveRequest received\r\n");
    } else {
        Serial_debug(DEBUG_INFO, &cli_serial, "Invalid serialNumberCmd\r\n");
    }
}


//void SaveSerialNumberToFlash( uint32_t serNumberCMD ) {
//
//    static app_vars_t newAppVarsSerialNumber;
//    unsigned char cmd;
//
//    cmd = (unsigned char)((serNumberCMD & 0xFF000000) >> 24);
//
//    Serial_debug(DEBUG_INFO, &cli_serial, "Received serialNumberCmd[%08p] decoded CMD:[%02d]\r\n", serNumberCMD, cmd);
//
//    switch ( cmd ) {
//
//        case 00:
//            newAppVarsSerialNumber.serialNumber[0] = (unsigned char)(serNumberCMD & 0x000000FF) ;
//            newAppVarsSerialNumber.serialNumber[1] = (unsigned char)((serNumberCMD & 0x0000FF00) >> 8);
//            newAppVarsSerialNumber.serialNumber[2] = (unsigned char)((serNumberCMD & 0x00FF0000) >> 16);
//            Serial_debug(DEBUG_INFO, &cli_serial, "serialNumber[0,1,2]=[%02X, %02X, %02X]\r\n",
//                          newAppVarsSerialNumber.serialNumber[0],
//                          newAppVarsSerialNumber.serialNumber[1],
//                          newAppVarsSerialNumber.serialNumber[2]);
//            break;
//        case 01:
//            newAppVarsSerialNumber.serialNumber[3] = (unsigned char)(serNumberCMD & 0x000000FF) ;
//            newAppVarsSerialNumber.serialNumber[4] = (unsigned char)((serNumberCMD & 0x0000FF00) >> 8);
//            newAppVarsSerialNumber.serialNumber[5] = (unsigned char)((serNumberCMD & 0x00FF0000) >> 16);
//            Serial_debug(DEBUG_INFO, &cli_serial, "serialNumber[3,4,5]=[%02X, %02X, %02X]\r\n",
//                          newAppVarsSerialNumber.serialNumber[3],
//                          newAppVarsSerialNumber.serialNumber[4],
//                          newAppVarsSerialNumber.serialNumber[5]);
//            break;
//        case 02:
//            newAppVarsSerialNumber.serialNumber[6] = (unsigned char)(serNumberCMD & 0x000000FF) ;
//            newAppVarsSerialNumber.serialNumber[7] = (unsigned char)((serNumberCMD & 0x0000FF00) >> 8);
//            Serial_debug(DEBUG_INFO, &cli_serial, "serialNumber[6,7]=[%02X, %02X]\r\n",
//                          newAppVarsSerialNumber.serialNumber[6],
//                          newAppVarsSerialNumber.serialNumber[7]);
//            break;
//        case 255:
//            AppVarsSaveRequest(&newAppVarsSerialNumber, SerialNumberAppVar);
//            Serial_debug(DEBUG_INFO, &cli_serial, "saveRequest received\r\n");
//            break;
//        default:
//            Serial_debug(DEBUG_INFO, &cli_serial, "Invalid serialNumberCmd\r\n");
//    }
//}

void PrepareNewAppVarToSave() {
    app_vars_t auxAppVarsToSave;
    memcpy( &auxAppVarsToSave, &currentAppVars, sizeof( app_vars_t ) );
    switch( appVarTypeToSave ) {
        case CapacitanceAppVar:
            auxAppVarsToSave.currentCapacitance = newAppVars.currentCapacitance;
            auxAppVarsToSave.initialCapacitance = newAppVars.initialCapacitance;
            break;
        case SerialNumberAppVar:
            memcpy( &auxAppVarsToSave.serialNumber, &newAppVars.serialNumber, SERIAL_NUMBER_SIZE_IN_CHARS );
            break;
        case AllAppVars:
            memcpy( &auxAppVarsToSave, &newAppVars, sizeof(app_vars_t) );
            break;
        default:
            break;
    }
    auxAppVarsToSave.MagicNumber = MAGIC_NUMBER;
    memcpy(&newAppVars, &auxAppVarsToSave, sizeof( app_vars_t) );
}
