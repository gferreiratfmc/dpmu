/*
* cobl_canbackdoor.c
*
* Copyright (c) 2015-2019 emotas embedded communication GmbH
*-------------------------------------------------------------------
* SVN  $Id: cobl_canbackdoor.c 29606 2019-10-23 10:25:58Z hil $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \file
* \brief cobl_canbackdoor.c - Bootloader backdoor using CAN
*
* The Bootloader activate the CAN during startup and wait a short time
* for a backdoor message. Without this message the application is calling.
*
* 
*
*/



/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <string.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <user_config.h>

#ifdef CFG_CAN_BACKDOOR
#include <cobl_type.h>
#include <cobl_hardware.h>
#include <cobl_can.h>
#include <cobl_timer.h>
#include <cobl_call.h>
#include <cobl_canopen.h>

#include <cobl_canbackdoor.h>

/* constant definitions
---------------------------------------------------------------------------*/
//#define BACKDOOR_SDO_WRITE_STOP 1
//#define BACKDOOR_SDO_WRITE_AB 1
#define BACKDOOR_SPECIAL_MESSAGE 1


/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/
void softwareReset(void);

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/


/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/



/********************************************************************/
/**
 * \brief backdoorCheck - wait a short time and check all received messages
 * 
 * \returns
 * never
 * 
 */
void backdoorCheck(void)
{
	/* initialize bootloader hardware */
	initHardware();

	setBackdoorCommand(COBL_BACKDOOR_ACTIVATE);

	(void)coblCanInit();
	coblCanListenOnlyMode();
	coblCanConfigureFilter(0);
	coblCanEnable();

	//no EMCY Debug possible -> no nodeId !
	//SEND_EMCY(0xFFFF, 0, 0, 0);
	timerInit();

	/* in case of reset */
	setBackdoorCommand(COBL_BACKDOOR_USED);
	while (timerTimeExpired(CFG_BACKDOOR_TIME) == 0)  {
	CanMsg_t canMsg;

		if (coblCanReceive(&canMsg) != CAN_EMPTY)  {

#ifdef BACKDOOR_SDO_WRITE_STOP
			// example SDO Stop message 0x1F51:1 = 0
			if (
					(canMsg.cobId.id == (0x600 + GET_NODEID()))
				&&	 (canMsg.dlc == 8)
				&& (canMsg.msg.u32Data[0] == 0x011F512Ful)
				&& (canMsg.msg.u32Data[1] == 0x00000000ul)
			)
			{
				setBootCommand();
				softwareReset();
			}
#endif
#ifdef BACKDOOR_SDO_WRITE_AB
			// example SDO Stop message with unused value 0x1F51:1 = 0xAB
			if (
					(canMsg.cobId.id == (0x600 + GET_NODEID()))
				&&	 (canMsg.dlc == 8)
				&& (COBL_REVERSE_U32(canMsg.msg.u32Data[0]) == 0x011F512Ful)
				&& (COBL_REVERSE_U32(canMsg.msg.u32Data[1]) == 0x000000ABul)
			)
			{
				setBootCommand();
				softwareReset();
			}
#endif
#ifdef BACKDOOR_SPECIAL_MESSAGE
			// special message
			if (canMsg.cobId.id == 42u)
			{
				setBootCommand();
				softwareReset();
			}
#endif
		}
	}

	setBackdoorCommand(COBL_BACKDOOR_DEACTIVATE);
	setStartCommand();
	softwareReset();

}

#endif /* CFG_CAN_BACKDOOR */
