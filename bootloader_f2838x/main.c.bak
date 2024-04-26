/* main
 *
 * Copyright (c) 2012-2020 emotas embedded communication GmbH
 *-------------------------------------------------------------------
 * SVN  $Date: 2020-08-28 14:28:54 +0200 (Fr, 28. Aug 2020) $
 * SVN  $Rev: 33311 $
 * SVN  $Author: ged $
 *-------------------------------------------------------------------
 *
 *
 */

/********************************************************************/
/**
 * \file
 * \brief main entry point
 */

/* hardware specific includes and settings
 --------------------------------------------------------------------------*/

/* standard includes
 --------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* header of project specific types
 ---------------------------------------------------------------------------*/
#include <user_config.h>

#include <cobl_type.h>
#include <cobl_can.h>
#include <cobl_canopen.h>
#include <cobl_flash.h>
#include <cobl_crc.h>
#include <cobl_application.h>
//#include <cobl_debug.h>
#include <cobl_call.h>
#include <cobl_hardware.h>
#include <cobl_timer.h>
#ifdef CFG_CAN_BACKDOOR
#include <cobl_canbackdoor.h>
#endif
/* constant definitions
 ---------------------------------------------------------------------------*/

/* local defined data types
 ---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
 ---------------------------------------------------------------------------*/

/* list of global defined functions
 ---------------------------------------------------------------------------*/
/* extern void printTable(void); */

/* list of local defined functions
 ---------------------------------------------------------------------------*/
static void jump2Appl(void);
#ifdef CFG_CAN_BACKDOOR
static void bootloaderRestart(void);
#endif /* CFG_CAN_BACKDOOR */

/* external variables
 ---------------------------------------------------------------------------*/

/* global variables
 ---------------------------------------------------------------------------*/

/* local defined variables
 ---------------------------------------------------------------------------*/


/***************************************************************************/
/**
 * \brief main entry
 *
 * \params
 *       nothing
 * \results
 *       exit value
 */


//#define NO_BOOT

/* set Z1_OTP to start from FLASH */
#pragma RETAIN(z1_Otp_bootPin_Config)
#pragma DATA_SECTION(z1_Otp_bootPin_Config, "z1_otpBootPinConfig");
const uint32_t z1_Otp_bootPin_Config = 0x5Affffff;    /* KEY = 0x5A, no boot pins      */
#pragma RETAIN(z1_Otp_bootDef_Low)
#pragma DATA_SECTION(z1_Otp_bootDef_Low, "z1_otpBootDef_Low");
const uint32_t z1_Otp_bootDef_Low    = 0x00000003;    /* BOOT_DEF0 = Flash Boot (0x03), BOOT_DEF1 = 0 */
//#pragma RETAIN(z1_Otp_bootDef_High)
//#pragma DATA_SECTION(z1_Otp_bootDef_High, "z1_otpBootDef_High");
//const uint32_t z1_Otp_bootDef_High   = 0x00000000;    /* BOOT_DEF2 = 0, BOOT_DEF3 = 0 */

int main(void)
{
CoblRet_t ret;
CanState_t canRet;
Flag_t callFlag;
#if COBL_DOMAIN_COUNT > 1
U8 i;
#endif

	/* disable all interrupts, if not disabled after reset */
    DINT;

#ifdef COBL_ECC_FLASH
		/* ECC specific handling */
		cobl_command[ECC_ERROR_IDX] = 0;
#endif

	SysCtl_disableWatchdog();

	/* clock initialization */
	initBaseHardware();


#ifdef NO_BOOT
	/* this part can used during the development of 
	 * bootloader adapted applications without CRC calculation
	 */
	jump2Appl();
#endif

#ifdef COBL_DEBUG
	cobl_printDatatypes();
#endif

#ifdef CFG_CAN_BACKDOOR
    resetUnknownCommand();
#endif


	callFlag = stayInBootloader();
#ifdef NO_APPL
	callFlag = COBL_COMMAND_BL;
#endif
	/* check marker - to stay in bootloader */
	if (callFlag == COBL_COMMAND_START) {
		
		crcInitCalculation();
		/* check CRC */
		ret = applCheckChecksum(0u);
		if (ret == COBL_RET_OK) {
			PRINTF0("correct application in flash\n");
#if COBL_DOMAIN_COUNT > 1
			for(i = 1; i < COBL_DOMAIN_COUNT ; i++) {
				if (ret == COBL_RET_OK) {
					ret = applCheckChecksum(i);
				}
			}
#endif
		}

		if (ret == COBL_RET_OK) {
			/* jump to application - never return */
			jump2Appl();
//			while(1); // never reach this point

		} else {
			PRINTF0("wrong application\n");
		}
	}

#ifdef CFG_CAN_BACKDOOR
	if (callFlag == COBL_COMMAND_BACKDOOR) {
		/* check for backdoor message */
		 backdoorCheck();
		 /* no return - software reset is calling */
	}
#endif

	/* reset stay in bootloader marker */
	setStartCommand();
	
    /* initialize bootloader hardware */
	initHardware();

	/* initialize - at the first - hardware */
	ret = flashInit(0u);
	if (ret != COBL_RET_OK) {
		while(1)
			;
	}

	canRet = coblCanInit();
	if (canRet != CAN_OK) {

		exit(-1);
		while(1) { }
	}
	coblCanEnable();

	/* initialize - now protocol */
	ret = canopenInit();
	if (ret != COBL_RET_OK) {
		while(1) { }
	}

#ifdef COBL_ECC_FLASH
	/* send emcy after ECC double bit error */
	if (cobl_command[ECC_ERROR_IDX] == ECC_ERROR_VAL)
	{
		SEND_EMCY(0x5000u, 0u, 0u, 0u);
		flashEraseRequired();
	} else
#endif /*  COBL_ECC_FLASH */

	{
#ifdef COBL_CHECK_PRODUCTID
#ifdef COBL_EMCY_PRODUCER
		ret = applCheckImage();
		if (ret == COBL_RET_IMAGE_WRONG) {
			/* wrong image */
			canopenSendEmcy(0x6200u, 0u, 0u, 0u);
		}
#endif /* COBL_EMCY_PRODUCER*/
#endif /* COBL_CHECK_PRODUCTID */
	}

	/* endless loop */
	do {
		canopenCyclic();
		flashCyclic();
		toggleWD();
	} while(1);

	PRINTF0("end of program\n");
}

/***************************************************************************/
/**
* \brief jump2Appl - jump to application
*
*
* no return
*
*/

static void jump2Appl(void)
{
    void (*pAppl)(void);

    /* call application - overwrite last command */
    setStartCommand();
    #ifdef CFG_LOCK_USED
        unlockException();
    #endif
    #ifdef CFG_CAN_BACKDOOR
        setBackdoorCommand(COBL_BACKDOOR_DEACTIVATE);
    #endif

    pAppl = (void (*)(void))applFlashStart(0); /* BEGIN section, call code_start  */

        pAppl();

}

#ifdef CFG_CAN_BACKDOOR
static void bootloaderRestart(void)
{
   void (*pAppl)(void);
   DINT;

   /* we jump directly to start of boot loader again */
   pAppl = (void (*)(void))0x00080000;
   pAppl();
}
#endif /* CFG_CAN_BACKDOOR */

/***************************************************************************/
/**
* \brief Reset - reset system
*
*/

void softwareReset(void)
{
#ifdef CFG_CAN_BACKDOOR
    bootloaderRestart();
#else
#endif /* CFG_CAN_BACKDOOR */
    SysCtl_enableWatchdog();

	while(1);
}

