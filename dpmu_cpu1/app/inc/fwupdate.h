/*
* fwupdate.h - firmware update
*
* Copyright (c) 2012 emotas embedded communication GmbH
*
*-------------------------------------------------------------------
* $Id: fwupdate.h 14796 2016-08-12 12:07:26Z ro $

*-------------------------------------------------------------------
*
* Changelog:
*
*
*/

/**
* \file
* \brief req. Information for the initiation of the firmware update
*/

#ifndef FWUPDATE_H
#define FWUPDATE_H 1


#include "co_datatype.h"

/* datatypes */

/* definitions */

#define COMMAND_SIZE 4
#define COMMAND_BOOT "BOOT"
#define COMMAND_NONE "NNNN"

#define FLASH_CPU2_CONFIG_BLOCK_ADDRESS         0x0A0000
#define FLASH_CPU2_CHECKSUM_ADDRESS             0x0A0002
#define FLASH_CPU2_CONFIG_BLOCK_SIZE_IN_WORDS   0x08
#define FLASH_CPU2_APPLICATION_ADDRESS          (FLASH_CPU2_CONFIG_BLOCK_ADDRESS + FLASH_CPU2_CONFIG_BLOCK_SIZE_IN_WORDS)
#define CPU2_APPLICATION_SIZE                   0x10000

#define IMAGE_BUFFER_SIZE  16


typedef enum{
    fw_not_available = 0,
    fw_available,
    fw_checksum_error,
    fw_ok
}FWImageStatus;



/* external data */
extern UNSIGNED8 cobl_command[16];


/* function prototypes */
extern void jump2Bootloader(void);
extern FWImageStatus fwupdate_updateExtRamWithCPU2Binary();
void startCPU2FirmwareUpdate();
#endif /* ANZEIGE_H */

