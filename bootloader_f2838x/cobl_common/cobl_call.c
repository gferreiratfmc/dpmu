/*
* cobl_call.c
*
* Copyright (c) 2012-2019 emotas embedded communication GmbH
*-------------------------------------------------------------------
* SVN  $Id: cobl_call.c 31962 2020-05-06 08:43:26Z ro $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \file
* \brief cobl_call.c - check application and decide to stay in the bootloader
*
* Currently this file contains the communication over a RAM section with
* the application. The Customer can implement additional functionality 
* to stay in the bootloader with a corrupt application, but with correct 
* checksum. E.g. usage of additional GPIO Pins.

*
*
* Bootloader dont find 'BOOT' command
*  - check application CRC
*  - call application
*
* Application send 'BOOT' command and start Bootloader
*  - Bootloader find 'BOOT' command
*  - Bootloader wait for CANopen commands
*
* With Backdoor (not generally supported!)
*  Bootloader dont find 'BOOT' command
*   - activate backdoor
*   in case the backdoor is not used
*   - send 'START' command and restart bootloader
*
*   Bootloader find 'START' command
*   - check application
*   - call application
*
*/



/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <string.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <user_config.h>
#include <cobl_type.h>

#include <cobl_call.h>
#include <cobl_application.h>

#include <cobl_crc.h>


/* constant definitions
---------------------------------------------------------------------------*/

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/
/**
* \cobl_command is shared memory between bootloader and application
*
* cobl_command[0..3] call bootloader or application
*/
#if defined ( __CC_ARM )
U8 cobl_command[16] __attribute__((at(0x20000100)));		/* Keil */
#elif defined ( __GNUC__ )
//U8 cobl_command[16] __attribute__((section(".noinit")));	/* GCC */
U8 cobl_command[16] __attribute__((section("command")));	/* GCC */
#elif defined (__ICCARM__)
#pragma location = "command_ram"
__no_init U8 cobl_command[16];								/* IAR */
#elif defined (WIN32)
U8 cobl_command[16];										/* VS */
#elif defined(__CCRX__)
#pragma address cobl_command=0x00000004
U8 cobl_command[16];								/*  */
#elif defined (__E2STUDIO__)
#pragma address cobl_command=0xfb300
U8 cobl_command[16];								/*  */
#else
U8 cobl_command[16] __attribute__((section("command")));    /* GCC */
#endif


/* local defined variables
---------------------------------------------------------------------------*/

/********************************************************************/
/**
 * \brief check to stay in the bootloader
 * 
* Within this function it is possible to add manufacturer
 * backup solutions in case of wrong application, that cannot
 * call back the bootloader.
 *
 * \retval COBL_COMMAND_START
 * application should called
 * \retval COBL_COMMAND_BL
 * stay in bootloader
 * \retval COBL_COMMAND_BACKDOOR
 * activate backdoor
 */
Flag_t stayInBootloader(void)
{
#ifdef COBL_CHECK_PRODUCTID
CoblRet_t ret;
#endif

	if (memcmp(&cobl_command[0], COMMAND_BOOT, COMMAND_SIZE) == 0)  {
		/* stay in bootloader - 'BOOT' command from the application */
		return(COBL_COMMAND_BL);
	}
#ifdef COBL_ECC_FLASH
	if (cobl_command[ECC_ERROR_IDX] == ECC_ERROR_VAL)  {
		return(COBL_COMMAND_BL);
	}
#endif
#ifdef COBL_CHECK_PRODUCTID
	ret = applCheckImage();
	if (ret != COBL_RET_OK)  {
		/* wrong image */
		return(COBL_COMMAND_BL);
	}
#endif

#ifdef CFG_CAN_BACKDOOR
	if (memcmp(&cobl_command[0], COMMAND_START, COMMAND_SIZE) != 0)  {
		/* stay in bootloader - activate backdoor */
		return(COBL_COMMAND_BACKDOOR);
	}
#endif

	/* call application, if possible */
	return(COBL_COMMAND_START);
}

#ifdef CFG_LOCK_USED
void lockException(void)
{
	/* set bootloader exception marker */
	cobl_command[4] = EXCEPTION_BL_1;
	cobl_command[5] = EXCEPTION_BL_2;
}

void unlockException(void)
{
	/* reset bootloader exception marker */
	cobl_command[4] = 0x00;
	cobl_command[5] = 0xFF;
}
#endif

/********************************************************************/
/**
 * \brief reset bootloader stay command
 * 
 */
void setNoneCommand(void)
{
	/* reset marker */
	memset( &cobl_command[0], 0, sizeof(cobl_command));
	memcpy( &cobl_command[0], COMMAND_NONE, COMMAND_SIZE);

#ifdef CFG_LOCK_USED
	/* init this area */
	lockException();
#endif

#ifdef CFG_CAN_BACKDOOR
	/* init backdoor parameter */
	setBackdoorCommand(COBL_BACKDOOR_DEACTIVATE);
#endif

}

/********************************************************************/
/**
 * \brief reset unknown cobl_command[] sequences
 * Initialize the memory like after a power on reset.
 * Note, that some settings also set, if the memory
 * was initialized (COMMAND_NONE).
 *
 */

void resetUnknownCommand(void)
{
	if (
			(memcmp(&cobl_command[0], COMMAND_BOOT, COMMAND_SIZE) != 0)
		&&  (memcmp(&cobl_command[0], COMMAND_START, COMMAND_SIZE) != 0)
		&&	(memcmp(&cobl_command[0], COMMAND_NONE, COMMAND_SIZE) != 0)
		)
	{
		setNoneCommand();
	}

}

/********************************************************************/
/**
 * \brief set command to call application
 *
 */
void setStartCommand(void)
{
	/* reset marker */
	memcpy( &cobl_command[0], COMMAND_START, COMMAND_SIZE);

}

/********************************************************************/
/**
 * \brief set command to stay in bootloader
 *
 */
void setBootCommand(void)
{
	/* reset marker */
	memcpy( &cobl_command[0], COMMAND_BOOT, COMMAND_SIZE);

}

#ifdef CFG_CAN_BACKDOOR
/********************************************************************/
/**
 * \brief set backdoor state command
 *
 */
void setBackdoorCommand(
		CoblBackdoor_t state
	)
{
	cobl_command[COMMAND_IDX_BACKDOOR_STATE] = (U8)state;
}

/********************************************************************/
/**
 * \brief get current backdoor command
 *
 */
CoblBackdoor_t getBackdoorCommand(void)
{
	return((CoblBackdoor_t)cobl_command[COMMAND_IDX_BACKDOOR_STATE]);
}
#endif
