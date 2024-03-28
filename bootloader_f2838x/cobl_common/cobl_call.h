/*
* cobl_call.h
*
* Copyright (c) 2012-2019 emotas embedded communication GmbH
*-------------------------------------------------------------------
* SVN  $Id: cobl_call.h 30042 2019-12-03 16:32:16Z ro $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \file
* \brief information for application call
*
*/



#ifndef COBL_CALL_H
#define COBL_CALL_H 1

/** cobl_command[]
 * byte 0..3 Bootloader/Application State
 * byte 4..5 lock exception table to bootloader
 * byte 6    backdoor state
 * ..
 * byte 14   reset reason RSTSTAT
 * byte 15   data storage exception or general ECC trap
 */

extern U8 cobl_command[16]; /**< command memory between appl and bootloader */
#define COMMAND_SIZE 4
#define COMMAND_BOOT "BOOT"
#define COMMAND_NONE "NNNN"
#define COMMAND_START "RUN1"

/* cobl_command[4..5] in case of BL exception */
#define EXCEPTION_BL_1 0x55
#define EXCEPTION_BL_2 0xAA

#define COMMAND_IDX_BACKDOOR_STATE 6

#define COMMAND_IDX_NODEID 7
#define COMMAND_IDX_BITRATE 8	/* 8..9 */

/* reset reason */
#define RESET_REG_IDX 14

/* DSI_ is obsolete, use ECC_ERROR */
#define DSI_MARKER_IDX 15
#define DSI_MARKER_VAL 0xAA
#define ECC_ERROR_IDX 15
#define ECC_ERROR_VAL 0xAA




/* Prototypes
*---------------------------------------------------------------------------*/
Flag_t stayInBootloader(void);

void setNoneCommand(void);
void resetUnknownCommand(void);
void setStartCommand(void);
void setBootCommand(void);

void lockException(void);
void unlockException(void);

void setBackdoorCommand(CoblBackdoor_t state);
CoblBackdoor_t getBackdoorCommand(void);


#endif /* COBL_CALL_H */

