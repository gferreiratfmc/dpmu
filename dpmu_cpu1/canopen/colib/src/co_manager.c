/*
* co_manager.c - contains manager routines according CiA 302-2
*
* Copyright (c) 2014-2022 emotas embedded communication GmbH
*-------------------------------------------------------------------------------
* $Id: co_manager.c 41917 2022-09-01 06:50:13Z boe $
*
*
*-------------------------------------------------------------------------------
*
*
*/


/********************************************************************/
/**
* \brief Manager handling according to CiA 302-2
*
* \file co_manager.c
* contains CANopen Manager handling according to CiA 302-2
*
*/

/* header of standard C - libraries
------------------------------------------------------------------------------*/
#include <stddef.h>

/* header of project specific types
------------------------------------------------------------------------------*/
#include <gen_define.h>

#ifdef CO_BOOTUP_MANAGER

#include <co_datatype.h>
#include <co_drv.h>
#include <co_timer.h>
#include <co_odaccess.h>
#include <co_odindex.h>
#include <co_nmt.h>
#include <co_sdo.h>
#include <co_store.h>
#include <co_manager.h>
#include "ico_cobhandler.h"
#include "ico_queue.h"
#include "ico_nmt.h"
#include "ico_event.h"

/* constant definitions
------------------------------------------------------------------------------*/
#ifdef CO_EVENT_DYNAMIC_MANAGER
# ifdef CO_EVENT_PROFILE_MANAGER
#  define CO_EVENT_MANAGER_CNT	(CO_EVENT_DYNAMIC_MANAGER + CO_EVENT_PROFILE_MANAGER)
# else /* CO_EVENT_PROFILE_MANAGER */
#  define CO_EVENT_MANAGER_CNT	(CO_EVENT_DYNAMIC_MANAGER)
# endif /* CO_EVENT_PROFILE_MANAGER */
#else /* CO_EVENT_DYNAMIC_MANAGER */
# ifdef CO_EVENT_PROFILE_MANAGER
#  define CO_EVENT_MANAGER_CNT	(CO_EVENT_PROFILE_MANAGER)
# endif /* CO_EVENT_PROFILE_MANAGER */
#endif /* CO_EVENT_DYNAMIC_MANAGER */

#ifndef CO_MGRBOOT_SDO_TO
# define CO_MGRBOOT_SDO_TO 1000ul
#endif /* CO_MGRBOOT_SDO_TO */

#ifndef CO_MGRBOOT_RESET_WAIT
# define CO_MGRBOOT_RESET_WAIT	1000ul
#endif /* CO_MGRBOOT_RESET_WAIT */

#ifndef CO_MAX_SLAVE_ASSIGNMENT_CNT
# define CO_MAX_SLAVE_ASSIGNMENT_CNT	CO_SLAVE_ASSIGNMENT_CNT
#endif /* CO_MAX_SLAVE_ASSIGNMENT_CNT */

/* local defined data types
------------------------------------------------------------------------------*/
/* manager states */
typedef enum {
	MGR_STATE_STARTED,
	MGR_STATE_RDY_OPERATIONAL,
	MGR_STATE_FAILURE
} MGR_STATE_T;

/* slave states */
typedef enum {
	SLAVE_STATE_UNKNOWN,
	SLAVE_STATE_FAILURE,
	SLAVE_STATE_BOOT,
	SLAVE_STATE_SDO_1000,
	SLAVE_STATE_SDO_IDENT_1,
	SLAVE_STATE_SDO_IDENT_2,
	SLAVE_STATE_SDO_IDENT_3,
	SLAVE_STATE_SDO_IDENT_4,
	SLAVE_STATE_SDO_1020_1,
	SLAVE_STATE_SDO_1020_2,
	SLAVE_STATE_CHECK_RESTORE,
	SLAVE_STATE_CHECK_SW,
	SLAVE_STATE_UPDATE_CFG,
	SLAVE_STATE_WAIT_ERRCTRL,
	SLAVE_STATE_BOOTED
} SLAVE_STATE_T;


/* manager error events */
typedef enum {
	ERROR_STATE_A,
	ERROR_STATE_B,
	ERROR_STATE_C,
	ERROR_STATE_D,
	ERROR_STATE_J,
	ERROR_STATE_G,
	ERROR_STATE_K,
	ERROR_STATE_M,
	ERROR_STATE_N,
	ERROR_STATE_O
} MANAGER_EVENT_T;

/* slave data structure */
typedef struct {
	UNSIGNED32		u32;
	SLAVE_STATE_T	state;
	CO_TIMER_T		timer;
	CO_EVENT_T		event;
	UNSIGNED8		nodeId;
	UNSIGNED8		sdoNr;
	CO_NMT_STATE_T	nmtCmd;
} SLAVE_DATA_T;

/* list of external used functions, if not in headers
------------------------------------------------------------------------------*/
# ifdef CO_EVENT_STATIC_MANAGER
extern CO_CONST CO_EVENT_MANAGER_BOOTUP_T coEventManagerInd;
# endif /* CO_EVENT_STATIC_MANAGER */

/* list of global defined functions
------------------------------------------------------------------------------*/

/* list of local defined functions
------------------------------------------------------------------------------*/
static RET_T managerResetComm(void);
static void managerResetCommNode(void *pData);
static void bootSlaves(void *ptr);
static void bootSlave(SLAVE_DATA_T *pSlave);
static void managerErrorIndication(SLAVE_DATA_T *pSlave,
		MANAGER_EVENT_T	event);
static void sdoReadAnswer(UNSIGNED8 sdoNr,
		UNSIGNED16 index, UNSIGNED8	subIndex, UNSIGNED32 size,
		UNSIGNED32 result);
static void checkRestoreDefaultValues(SLAVE_DATA_T *pSlave);
static void	checkSoftwareVersion(SLAVE_DATA_T *pSlave);
static void	updateConfiguration(SLAVE_DATA_T *pSlave);
static void startErrorControl(SLAVE_DATA_T *pSlave);
static void bootTimeFinished(void *ptr);
static void readObj1000(void *ptr);
static void errorCtrlReady(SLAVE_DATA_T *pSlave);
static void managerIndication(UNSIGNED8 node,
		CO_MANAGER_EVENT_T event);
static void guardingIndication(UNSIGNED8 nodeId,
		CO_ERRCTRL_T event,	CO_NMT_STATE_T nmtState);
static void checkMandatorySlavesBooted(void);
static void nmtCmd(void *pData);
static void sendNmtCmd(UNSIGNED8 slave, CO_NMT_STATE_T cmd);
static void errorHandler(SLAVE_DATA_T *pSlave);
static UNSIGNED32 getSlaveAssign(CO_CONST SLAVE_DATA_T *pSlave);
static void readIdentity(SLAVE_DATA_T *pSlave);
static void	checkConfigData(SLAVE_DATA_T *pSlave);
static SLAVE_DATA_T	*getSlave(UNSIGNED8 idx);
static SLAVE_DATA_T	*getSlaveByNodeId(UNSIGNED8 nodeId);

/* global variables
------------------------------------------------------------------------------*/

/* local defined variables
------------------------------------------------------------------------------*/
static MGR_STATE_T	managerState;
static UNSIGNED8	slaveCnt;		/* number of slaves */
static UNSIGNED8	slaveCntReset;	/* act. number for reset slaves */
static CO_EVENT_T	managerEvent;
static SLAVE_DATA_T	slaveData[CO_MAX_SLAVE_ASSIGNMENT_CNT];
static CO_NMT_STATE_T	masterNmtCmd;
static CO_TIMER_T	bootupTimer;
# ifdef CO_EVENT_MANAGER_CNT
static CO_EVENT_MANAGER_BOOTUP_T	managerTable[CO_EVENT_MANAGER_CNT];
static UNSIGNED8		managerTableCnt = 0u;
# endif /* CO_EVENT_MANAGER_CNT */
static CO_TIMER_T	postResetTimer;
static BOOL_T		initState = CO_FALSE;




/***************************************************************************/
/**
* \brief coManagerStart
*
* This function starts the CANopen manager process.
* All necessary parameter for mandatory and optional slaves 
* has to be available at the object dictionary.
*
* Please note:
* SDO channels are initialized and used by manager process.
* To reuse a SDO channel please use function
* coManagerGetUsedSdoChannel()
*
* \return RET_T
*
*/
RET_T coManagerStart(
		void	/* no parameter */
	)
{
UNSIGNED32	nmtStartup;
RET_T		retVal;
UNSIGNED32	bootTime = 0u;

	/* init only once */
	if (initState == CO_FALSE)  {
		/* register indication functions */
		retVal = coEventRegister_SDO_CLIENT_READ(sdoReadAnswer); /*lint !e960 */
		/* Derogation MisraC2004 R.16.9 function identifier used without '&'
		 * or parenthesized parameter */
		if (retVal != RET_OK)  {
			return(retVal);
		}
		retVal = coEventRegister_ERRCTRL(guardingIndication); /*lint !e960 */
		/* Derogation MisraC2004 R.16.9 function identifier used without '&'
		 * or parenthesized parameter */
		if (retVal != RET_OK)  {
			return(retVal);
		}
		initState = CO_TRUE;
	}

	/* are we configured as master ? */
	nmtStartup = icoNmtMasterGetStartupObj();
	if ((nmtStartup & CO_NMT_STARTUP_BIT_MASTER) == 0u)  {
		return(RET_INVALID_PARAMETER);
	}

#ifdef CO_FLYING_MASTER_SUPPORTED
	/* flying master request ?*/
	if ((nmtStartup & CO_NMT_STARTUP_BIT_FLYMA) != 0u)  {
		/* wait, until we are the master */
		return(RET_SERVICE_BUSY);
	}
#endif /* CO_FLYING_MASTER_SUPPORTED */

	/* if bootup manager in progress, return error */
	if (coTimerIsActive(&bootupTimer) == CO_TRUE)  {
		return(RET_SERVICE_BUSY);
	}

	/* start bootup timer */
	(void)coOdGetObj_u32(0x1f89u, 0u, &bootTime);
	(void)coTimerStart(&bootupTimer, bootTime * 1000ul,
			bootTimeFinished, NULL, CO_TIMER_ATTR_ROUNDUP); /*lint !e960 */
	/* Derogation MisraC2004 R.16.9 function identifier used without '&'
	 * or parenthesized parameter */

	managerState = MGR_STATE_STARTED;
	retVal = managerResetComm();

	return(retVal);
}


/***************************************************************************/
/**
* \internal
*
* \brief managerResetComm
*
* Send reset comm to all slaves
* If one of keep alive bit is set, send reset comm individuell for each device
*
* \return RET_T
*
*/
static RET_T managerResetComm(
		void	/* no parameter */
	)
{
UNSIGNED32	slaveAssign;
UNSIGNED8	noResComm = 0u;
UNSIGNED8	i;
RET_T		retVal;
SLAVE_DATA_T *pSlave;
UNSIGNED8	maxSubs;

	/* get number of slaves */
	retVal = coOdGetObj_u8(0x1f81u, 0u, &maxSubs);
	if (retVal != RET_OK)  {
		return(retVal);
	}

	/* setup slave data structures */
	/* check, if keep alive bit of one of the slaves is set */
	slaveCnt = 0u;

	/* for all subindexes */
	for (i = 0u; i < maxSubs; i++)  {
		/* ignore own node id */
		if ((i + 1) != coNmtGetNodeId())  {

			/* get slave assignment */
			slaveAssign = icoNmtMasterGetSlaveAssignment(i);

			/* keep alive bit set ? */
			if ((slaveAssign & CO_NMT_SLAVE_ASSIGN_BIT_RESETCOMM) != 0u)  {
				/* yes, device doesn't allow reset comm every time */
				noResComm++;
			}

			/* slave still in network list ? */
			if ((slaveAssign & CO_NMT_SLAVE_ASSIGN_BIT_SLAVE) != 0u)  {

				/* yes, init data struct for this slave */
				pSlave = getSlave(slaveCnt);
				if (pSlave == NULL)  {
					/* no more slave structs available - abort */
					return(RET_EVENT_NO_RESSOURCE);
				}

				pSlave->state = SLAVE_STATE_UNKNOWN;
				pSlave->nodeId = (UNSIGNED8)i + 1u;
				pSlave->sdoNr = slaveCnt + 1u;

				slaveCnt++;
			} else {
				managerErrorIndication(pSlave, ERROR_STATE_A);
			}
		}
	}

	/* keep alive bit set for any node ? */
	if (noResComm == 0u)  {
		/* no, send reset for all nodes */
		retVal = coNmtStateReq(0u, CO_NMT_STATE_RESET_COMM, CO_FALSE);
		if (retVal != RET_OK)  {
			return(retVal);
		}

		/* boot slaves */
		coTimerStart(&postResetTimer, CO_MGRBOOT_RESET_WAIT * 1000u,
			bootSlaves, NULL, CO_TIMER_ATTR_ROUNDUP);

	} else {

		/* send reset comm for each slave individually */
		slaveCntReset = maxSubs;
		managerResetCommNode(NULL);
	}

	return(retVal);
}


/***************************************************************************/
/**
* \internal
*
* \brief managerResetCommNode - reset comm for one node
*
* Send reset comm to only one node
*
* \return void
*
*/
static void managerResetCommNode(
		void			*pData
	)
{
RET_T	retVal = RET_OK;
UNSIGNED32	slaveAssign = 0u;
CO_NMT_STATE_T		rState;
(void)pData;

	/* check, if keep alive bit of one of the slaves is set */
	/* ignore own node id */
	if ((slaveCntReset) == coNmtGetNodeId())  {
		slaveCntReset--;
	}

	/* for all possible slaves at network */
	if (slaveCntReset != 0u)  {
		/* slave with entry at 1f81 ? */
		slaveAssign = icoNmtMasterGetSlaveAssignment(slaveCntReset - 1u);

		/* reset comm depends on NMT state */
		rState = coNmtGetRemoteNodeState(slaveCntReset);
		if (((slaveAssign & CO_NMT_SLAVE_ASSIGN_BIT_RESETCOMM) != 0u)
		 && (rState == CO_NMT_STATE_OPERATIONAL))  {
			/* OPERATIONAL - do nothing */
			slaveCntReset--;
		} else {
			/* send only if NMT inhibit is not active */
			if (coNmtInhibitActive() == CO_FALSE)  {
				retVal = coNmtStateReq(slaveCntReset, CO_NMT_STATE_RESET_COMM,
					CO_FALSE);
				if (retVal == RET_OK)  {
					slaveCntReset--;
				}
			}
		}

		/* call function again for next slave */
		(void)icoEventStart(&managerEvent,
			managerResetCommNode, NULL); /*lint !e960 */
		/* Derogation MisraC2004 R.16.9 function identifier used without '&'
		 * or parenthesized parameter */

	} else {
		/* last node was commanded, boot slaves */
		coTimerStart(&postResetTimer, CO_MGRBOOT_RESET_WAIT * 1000u,
			bootSlaves, NULL, CO_TIMER_ATTR_ROUNDUP);
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief bootSlaves - boot all mandatory and optional slaves 
*
* Send reset comm to one node
*
* \return void
*
*/
static void bootSlaves(
		void *ptr
	)
{
UNSIGNED8	i;
SLAVE_DATA_T *pSlave;
(void)ptr;

	/* for slaves in slave list */
	for (i = 0u; i < slaveCnt; i++)  {

		/* if manager process already finished ? */
		if (managerState == MGR_STATE_FAILURE)  {
			return;
		}

		/* slave available in network ? */
		pSlave = getSlave(i);
		bootSlave(pSlave);		
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief bootSlave - boot one slaves 
*
* \return void
*
*/
static void bootSlave(
		SLAVE_DATA_T	*pSlave
	)
{
RET_T		retVal, retVal2;
UNSIGNED32	slaveAssign;
UNSIGNED16	index;
UNSIGNED32	id;

	pSlave->state = SLAVE_STATE_BOOT;

	/* boot this slave ? */
	slaveAssign = getSlaveAssign(pSlave);
	if ((slaveAssign & CO_NMT_SLAVE_ASSIGN_BIT_BOOT) != 0u)  {
		/* setup cobid for SDO access */
		index = 0x1280u + pSlave->sdoNr - 1u; 
		id = 0x600ul + pSlave->nodeId;
		retVal = coOdSetCobid(index, 1u, id);
		id = 0x580ul + pSlave->nodeId;
		retVal2 = coOdSetCobid(index, 2u, id);

		/* inform application */
		managerIndication(pSlave->nodeId, CO_MANAGER_EVENT_BOOT);

		if ((retVal == RET_OK) && (retVal2 == RET_OK))  {
			/* ensure timer is stopped */
			(void)coTimerStop(&pSlave->timer);

			/* request object 1000 */
			retVal = coSdoRead(pSlave->sdoNr, 0x1000u, 0u,
					(UNSIGNED8 *)&pSlave->u32, 4u, 1u, CO_MGRBOOT_SDO_TO); /*lint !e928 */
			/* Derogation MisraC2004 R.11.4 cast from pointer to pointer */

			if (retVal == RET_OK)  {
				/* wait for sdo answer from slave */
				pSlave->state = SLAVE_STATE_SDO_1000;
			} else {
				/* inform application about error */
				managerErrorIndication(pSlave, ERROR_STATE_B);
			}
		} else {
			/* inform application about error */
			managerErrorIndication(pSlave, ERROR_STATE_B);
		}
	} else {
		/* goto to E */
		startErrorControl(pSlave);
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief readIdentity - read identity values from slave
*
* Read identity values from slave and compare it with values from 1f84..1f88
*
* \returns void
*
*/
static void readIdentity(
		SLAVE_DATA_T	*pSlave
	)
{
RET_T	retVal;
UNSIGNED32	slaveAssign;
UNSIGNED32	deviceTypeIdent = 0u;
UNSIGNED32	deviceVendor = 0u;
UNSIGNED32	deviceProduct = 0u;
UNSIGNED32	deviceRevision = 0u;
UNSIGNED32	deviceSerial = 0u;

	/* wait for object 0x1000 ? */
	if (pSlave->state == SLAVE_STATE_SDO_1000)  {
		/* device type check required and correct ? */
		(void)coOdGetObj_u32(0x1f84u, pSlave->nodeId, &deviceTypeIdent);
		/* compare received value with slave values (if != 0) */
		if ((deviceTypeIdent != 0ul)
		 && (deviceTypeIdent != pSlave->u32)) {
			/* doesn't fit, abort */
			pSlave->state = SLAVE_STATE_FAILURE;
			managerErrorIndication(pSlave, ERROR_STATE_C);
			return;
		}

		pSlave->state = SLAVE_STATE_SDO_IDENT_1;

		/* check vendor id required ? */
		(void)coOdGetObj_u32(0x1f85u, pSlave->nodeId, &deviceVendor);
		if (deviceVendor != 0u)  {
			/* start sdo transfer */
			retVal = coSdoRead(pSlave->sdoNr, 0x1018u, 1u,
				(UNSIGNED8 *)&pSlave->u32, 4u, 1u, CO_MGRBOOT_SDO_TO); /*lint !e928 */
			/* Derogation MisraC2004 R.11.4 cast from pointer to pointer */

			if (retVal != RET_OK)  {
				pSlave->state = SLAVE_STATE_FAILURE;
				managerErrorIndication(pSlave, ERROR_STATE_D);
			}
			return;
		}
	}

	/* state ident1 received ? */
	if (pSlave->state == SLAVE_STATE_SDO_IDENT_1)  {
		/* get vendor id from OD */
		(void)coOdGetObj_u32(0x1f85u, pSlave->nodeId, &deviceVendor);
		/* and compare it with received sdo value */
		if ((deviceVendor != 0u)
		 && (deviceVendor != pSlave->u32)) {
			/* doesn't fit - error */
			pSlave->state = SLAVE_STATE_FAILURE;
			managerErrorIndication(pSlave, ERROR_STATE_D);
			return;
		}

		pSlave->state = SLAVE_STATE_SDO_IDENT_2;

		/* check productcode required ? */
		(void)coOdGetObj_u32(0x1f86u, pSlave->nodeId, &deviceProduct);
		if (deviceProduct != 0u)  {
			/* request value by sdo */
			retVal = coSdoRead(pSlave->sdoNr, 0x1018u, 2u,
				(UNSIGNED8 *)&pSlave->u32, 4u, 1u, CO_MGRBOOT_SDO_TO); /*lint !e928 */
			/* Derogation MisraC2004 R.11.4 cast from pointer to pointer */

			if (retVal != RET_OK)  {
				pSlave->state = SLAVE_STATE_FAILURE;
				managerErrorIndication(pSlave, ERROR_STATE_M);
			}
			return;
		}
	}

	/* productcode received ? */
	if (pSlave->state == SLAVE_STATE_SDO_IDENT_2)  {
		/* get productcode from OD */
		(void)coOdGetObj_u32(0x1f86u, pSlave->nodeId, &deviceProduct);
		/* and compare it with received from sdo value */
		if ((deviceProduct != 0u)
		 && (deviceProduct != pSlave->u32)) {
			pSlave->state = SLAVE_STATE_FAILURE;
			managerErrorIndication(pSlave, ERROR_STATE_M);
			return;
		}

		pSlave->state = SLAVE_STATE_SDO_IDENT_3;

		/* check revision required ? */
		(void)coOdGetObj_u32(0x1f87u, pSlave->nodeId, &deviceRevision);
		if (deviceRevision != 0u)  {
			/* start sdo transfer */
			retVal = coSdoRead(pSlave->sdoNr, 0x1018u, 3u,
				(UNSIGNED8 *)&pSlave->u32, 4u, 1u, CO_MGRBOOT_SDO_TO); /*lint !e928 */
			/* Derogation MisraC2004 R.11.4 cast from pointer to pointer */

			if (retVal != RET_OK)  {
				pSlave->state = SLAVE_STATE_FAILURE;
				managerErrorIndication(pSlave, ERROR_STATE_N);
			}
			return;
		}
	}

	/* revision received ? */
	if (pSlave->state == SLAVE_STATE_SDO_IDENT_3)  {
		/* revision from OD */
		(void)coOdGetObj_u32(0x1f87u, pSlave->nodeId, &deviceRevision);
		/* and compare it with received value from sdo */
		if ((deviceRevision != 0u)
		 && (deviceRevision != pSlave->u32)) {
			pSlave->state = SLAVE_STATE_FAILURE;
			managerErrorIndication(pSlave, ERROR_STATE_N);
			return;
		}

		pSlave->state = SLAVE_STATE_SDO_IDENT_4;

		/* check serial number required ? */
		(void)coOdGetObj_u32(0x1f88u, pSlave->nodeId, &deviceSerial);
		if (deviceSerial != 0u)  {
			/* start sdo transfer */
			retVal = coSdoRead(pSlave->sdoNr, 0x1018u, 4u,
				(UNSIGNED8 *)&pSlave->u32, 4u, 1u, CO_MGRBOOT_SDO_TO); /*lint !e928 */
			/* Derogation MisraC2004 R.11.4 cast from pointer to pointer */

			if (retVal != RET_OK)  {
				pSlave->state = SLAVE_STATE_FAILURE;
				managerErrorIndication(pSlave, ERROR_STATE_O);
			}
			return;
		}
	}

	/* serial number received ? */
	if (pSlave->state == SLAVE_STATE_SDO_IDENT_4)  {
		/* get snr from OD */
		(void)coOdGetObj_u32(0x1f88u, pSlave->nodeId, &deviceSerial);
		/* and compare it with value from sdo */
		if ((deviceSerial != 0u)
		 && (deviceSerial != pSlave->u32)) {
			pSlave->state = SLAVE_STATE_FAILURE;
			managerErrorIndication(pSlave, ERROR_STATE_O);
			return;
		}
	}

	/* keep alive bit set for this node ? */
	slaveAssign = getSlaveAssign(pSlave);
	if ((slaveAssign & CO_NMT_SLAVE_ASSIGN_BIT_RESETCOMM) != 0u) {
		/* check node state */







	}

	/* check RestoreDefaultValues reset (Bit 7 from 0x1f81) */
	checkRestoreDefaultValues(pSlave);

}
	

/***************************************************************************/
/**
* \internal
*
* \brief checkRestoreDefaultValues - reset to default values?
*
* Check if restoreDefaultValues is necessary
* Send restore and reset communication
*
*/
static void checkRestoreDefaultValues(
		SLAVE_DATA_T	*pSlave
	)
{
UNSIGNED8	rCfg = 0u;
UNSIGNED32	slaveAssign;
RET_T		retVal;

	pSlave->state = SLAVE_STATE_CHECK_RESTORE;
	slaveAssign = getSlaveAssign(pSlave);

	/* restore bit set ? */
	if ((slaveAssign & CO_NMT_SLAVE_ASSIGN_BIT_RESTORE) != 0u) {
		/* get 0x1f8a - restore configuration */
		coOdGetObj_u8(CO_INDEX_NMT_RESTORE_CONFIG, pSlave->nodeId, &rCfg);

		if (rCfg != 0u)  {
			/* write restore config (ignore sdo answer!) */
			pSlave->u32 = CO_STORE_SIGNATURE_LOAD;
			retVal = coSdoWrite(pSlave->sdoNr,
				CO_INDEX_RESTORE_PARA, rCfg,
				(UNSIGNED8 *)&pSlave->u32, 4u, 1u, CO_MGRBOOT_SDO_TO); /*lint !e928 */

			if (retVal != RET_OK)  {
				managerErrorIndication(pSlave, ERROR_STATE_J);
			}

			/* send reset communication */
			coNmtStateReq(pSlave->nodeId, CO_NMT_STATE_RESET_COMM,
				CO_FALSE);
			return;
		}
	}

	/* call check software */
	checkSoftwareVersion(pSlave);
}


/***************************************************************************/
/**
* \internal
*
* \brief checkSoftwareVersion - check software version 
*
* If check software version is necessary,
* call application function.
* Application is responsible for software update.
*
*/
static void	checkSoftwareVersion(
		SLAVE_DATA_T	*pSlave
	)
{
UNSIGNED32	slaveAssign;

	pSlave->state = SLAVE_STATE_CHECK_SW;
	slaveAssign = getSlaveAssign(pSlave);

	/* sw update bit set ? */
	if ((slaveAssign & CO_NMT_SLAVE_ASSIGN_BIT_SWVERSION) != 0u) {
		/* part of the application */
		managerIndication(pSlave->nodeId, CO_MANAGER_EVENT_UPDATE_SW);

		/* continue by coManagerContinueSwUpdate() */

	} else {
		/* check Configuration */
		(void)coManagerContinueSwUpdate(pSlave->nodeId, RET_OK);
	}
}


/***************************************************************************/
/**
*
* \brief coManagerContinueSwUpdate - continue after software update
*
* This function continues startup for the given node
* after software was updates by application
*
* If state of this node is not in correct state,
* the function returns RET_INVALID_PARAMETER
*
*/
RET_T coManagerContinueSwUpdate(
		UNSIGNED8		slave,			/**< slave */
		RET_T			result			/**< result of software update */
	)
{
RET_T	retVal1, retVal2;
UNSIGNED32	cDate, cTime;
SLAVE_DATA_T *pSlave;

	pSlave = getSlaveByNodeId(slave);
	if (pSlave == NULL)  {
		return(RET_INVALID_PARAMETER);
	}

	/* correct state for this node ? */
	if (pSlave->state != SLAVE_STATE_CHECK_SW)  {
		return(RET_INVALID_PARAMETER);
	}

	if (result != RET_OK)  {
		managerErrorIndication(pSlave, ERROR_STATE_G);
		return(RET_OK);
	}

	/* configuration set? */
	retVal1 = coOdGetObj_u32(0x1f26u, pSlave->nodeId, &cDate);
	retVal2 = coOdGetObj_u32(0x1f27u, pSlave->nodeId, &cTime);

	/* check for valid configuration entries at OD */
	if ((retVal1 != RET_OK) || (retVal2 != RET_OK)
	 || (cDate == 0u) || (cTime == 0u))  {
		/* objects not available or 0 */
		/* call application to update configuration */
		updateConfiguration(pSlave);

	} else {
		/* get object 0x1020 from slave */
		retVal1 = coSdoRead(pSlave->sdoNr, 0x1020u, 1u,
				(UNSIGNED8 *)&pSlave->u32, 4u, 1u, CO_MGRBOOT_SDO_TO); /*lint !e928 */
		/* Derogation MisraC2004 R.11.4 cast from pointer to pointer */

		if (retVal1 != RET_OK)  {
			pSlave->state = SLAVE_STATE_FAILURE;
			managerErrorIndication(pSlave, ERROR_STATE_J);
			return(RET_OK);
		}
		/* wait for sdo anser from slave */
		pSlave->state = SLAVE_STATE_SDO_1020_1;
	}

	return(RET_OK);
}


/***************************************************************************/
/**
* \internal
*
* \brief checkConfigData - compare config date from slave
*
* Compare received config data from slave with entries from OD
*
*/
static void	checkConfigData(
		SLAVE_DATA_T	*pSlave
	)
{
RET_T	retVal;
UNSIGNED32	cDate = 0u, cTime = 0u;

	/* waiting for SDO 1020:1 ? */
	if (pSlave->state == SLAVE_STATE_SDO_1020_1)  {
		/* get data from OD */
		(void)coOdGetObj_u32(0x1f26u, pSlave->nodeId, &cDate);
		/* and compare it with received data from SDO */
		if (cDate == pSlave->u32)  {
			/* get object 0x1020:2 from slave */
			retVal = coSdoRead(pSlave->sdoNr, 0x1020u, 2u,
					(UNSIGNED8 *)&pSlave->u32, 4u, 1u, CO_MGRBOOT_SDO_TO); /*lint !e928 */
			/* Derogation MisraC2004 R.11.4 cast from pointer to pointer */

			if (retVal != RET_OK)  {
				pSlave->state = SLAVE_STATE_FAILURE;
				managerErrorIndication(pSlave, ERROR_STATE_J);
				return;
			}
			pSlave->state = SLAVE_STATE_SDO_1020_2;
			return;
		}
	}

	/* waiting for SDO 1020:2 ? */
	if (pSlave->state == SLAVE_STATE_SDO_1020_2)  {
		/* get data from OD */
		(void)coOdGetObj_u32(0x1f27u, pSlave->nodeId, &cTime);
		/* and compare it with received data from SDO */
		if (cTime == pSlave->u32)  {
			/* config ok, continue */
			(void)coManagerContinueConfigUpdate(pSlave->nodeId, RET_OK);
			return;
		}
	}

	/* call application to update configuration */
	updateConfiguration(pSlave);
}


/***************************************************************************/
/**
* \internal
*
* \brief updateConfiguration - update configuration by application
*
* Call application to update configuration
*
*/
static void	updateConfiguration(
		SLAVE_DATA_T	*pSlave
	)
{
	/* part of the application do update the configuration */
	pSlave->state = SLAVE_STATE_UPDATE_CFG;

	managerIndication(pSlave->nodeId, CO_MANAGER_EVENT_UPDATE_CONFIG);

	/* continue by application by calling coManagerContinueConfigUpdate() */
}


/***************************************************************************/
/**
*
* \brief coManagerContinueConfigUpdate - continue configuration
*
* This function has to be called from application
* after configuration for the given node was finished
* The result should be RET_OK or another error code.
*
* If state of this node is not in correct state,
* the function returns RET_INVALID_PARAMETER
*
*/
RET_T coManagerContinueConfigUpdate(
		UNSIGNED8	slave,			/**< slave (1.. 127) */
		RET_T		result			/**< result of configuration process */
	)
{
SLAVE_DATA_T *pSlave;

	pSlave = getSlaveByNodeId(slave);
	if (pSlave == NULL)  {
		return(RET_INVALID_PARAMETER);
	}

	/* check for correct slave state */
	if (pSlave->state != SLAVE_STATE_UPDATE_CFG)  {
		return(RET_INVALID_PARAMETER);
	}

	if (result != RET_OK)  {
		managerErrorIndication(pSlave, ERROR_STATE_J);
		return(RET_OK);
	}

	/* start error control */
	startErrorControl(pSlave);

	return(RET_OK);
}


/***************************************************************************/
/**
* \internal
*
* \brief startErrorControl - start error control service
*
*
*/
static void startErrorControl(
		SLAVE_DATA_T	*pSlave
	)
{
UNSIGNED32	slaveAssign;
RET_T		retVal;
UNSIGNED8	hbConsCnt;
UNSIGNED32	hbEntry;
UNSIGNED8	i;
UNSIGNED8	hbConsumer;

	/* get number of HB consumer */
	retVal = coOdGetObj_u8(0x1016u, 0u, &hbConsCnt);
	if (retVal == RET_OK)  {
		/* search for entry with this node-id */
		for (i = 0u; i < hbConsCnt; i++)  {
			retVal = coOdGetObj_u32(0x1016u, i + 1u, &hbEntry);
			if (retVal == RET_OK)  {
				if (((hbEntry >> 16u) == pSlave->nodeId)
				 && ((hbEntry & 0xffffu) != 0u))  {
					/* hb entry found */
					/* start HB monitoring again and wait for HB */
					hbConsumer = (UNSIGNED8)((hbEntry >> 16) & 0xffu);
					(void)coHbConsumerStart(hbConsumer);
					pSlave->state = SLAVE_STATE_WAIT_ERRCTRL;
					return;
				}
			}
		}
	}

	/* guarding enabled ? */

	slaveAssign = getSlaveAssign(pSlave);
	if ((slaveAssign >> 16) != 0u)  {
		retVal = coGuardingMasterStart(pSlave->nodeId);
		if (retVal != RET_OK)  {
			managerErrorIndication(pSlave, ERROR_STATE_K);
		} else {
			pSlave->state = SLAVE_STATE_WAIT_ERRCTRL;
		}
		return;
	}

	errorCtrlReady(pSlave);

	return;
}


/***************************************************************************/
/**
* \internal
*
* \brief errorCtrlReady - error control is started
*
* This function is called after HB was received successfully
*
*/
static void errorCtrlReady(
		SLAVE_DATA_T	*pSlave
	)
{
UNSIGNED32	slaveAssign;
UNSIGNED32	nmtStartup = 0u;

	slaveAssign = getSlaveAssign(pSlave);

	/* boot this slave ? */
	if ((slaveAssign & CO_NMT_SLAVE_ASSIGN_BIT_BOOT) == 0u)  {
		/* no, dont send NMT command */
		pSlave->state = SLAVE_STATE_BOOTED;
	} else {

		/* start nodes automatically ? */
		nmtStartup = icoNmtMasterGetStartupObj();

		/* start nodes automatically to OPER ? */
		if ((nmtStartup & CO_NMT_STARTUP_BIT_STARTNODE) == 0u)  {
			/* start nodes individually ? */
			if ((nmtStartup & CO_NMT_STARTUP_BIT_STARTNMT0) == 0u)  {
				/* yes, start node */
				sendNmtCmd(pSlave->nodeId, CO_NMT_STATE_OPERATIONAL);
			} else {
				/* is master already in operational ? */
				if (coNmtGetState() == CO_NMT_STATE_OPERATIONAL)  {
					sendNmtCmd(pSlave->nodeId, CO_NMT_STATE_OPERATIONAL);
				}
			}
		}

		/* slave succesfully booted */
		pSlave->state = SLAVE_STATE_BOOTED;

		/* inform application ? */
		managerIndication(pSlave->nodeId, CO_MANAGER_EVENT_BOOTED);

		/* check all mandatory slaves */
		checkMandatorySlavesBooted();
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief checkMandatorySlavesBooted - check are all mandatory slaves booted
*
*
*
*/
static void checkMandatorySlavesBooted(
		void	/* no parameter */
	)
{
UNSIGNED32	slaveAssign;
UNSIGNED8	i;
UNSIGNED8	notBooted = 0u;
UNSIGNED32	startUp;
SLAVE_DATA_T *pSlave;


	/* for all slaves in slave list */
	for (i = 0u; i < slaveCnt; i++)  {
		/* mandatory slave ? */
		pSlave = getSlave(i);
		slaveAssign = getSlaveAssign(pSlave);
		if ((slaveAssign & CO_NMT_SLAVE_ASSIGN_BIT_MANDATORY) != 0u) {

			/* slave booted ? */
			if (pSlave->state != SLAVE_STATE_BOOTED)  {
				notBooted++;
			}
		}
	}

	/* anywhere not booted yet ? */
	if (notBooted != 0u)  {
		return;
	}

	/* manager is ready for OPERATIONAL */
	managerState = MGR_STATE_RDY_OPERATIONAL;

	startUp = icoNmtMasterGetStartupObj();

	/* enter automatically to OPERATIONAL ? */
	if ((startUp & CO_NMT_STARTUP_BIT_STARTITSELF) != 0u)  {
		/* wait for application to enter OPER */
		managerIndication(0u, CO_MANAGER_EVENT_RDY_OPERATIONAL);
	} else {
		/* enter myself to OPER */
		(void)coManagerContinueOperational();
	}
}


/***************************************************************************/
/**
*
* \brief coManagerContinueOperational - continue to OPERATIONAL
*
* This function continues Bootup Procedure to state OPERATIONAL,
* if the start bit at 0x1f80 is not set.
* The application can start the nodes itself, or call this function
* to do that.
*
* If state of this node is not in correct state,
* the function returns RET_INVALID_PARAMETER
*
*/
RET_T coManagerContinueOperational(
		void	/* no parameter */
	)
{
UNSIGNED32	slaveAssign;
UNSIGNED32	startUp;
UNSIGNED8	notBooted = 0u;
UNSIGNED8	i;
RET_T		retVal;
SLAVE_DATA_T *pSlave;

	/* correct manager state ? */
	if (managerState != MGR_STATE_RDY_OPERATIONAL)  {
		return(RET_INVALID_PARAMETER);
	}

	/* myself to OPER */
	retVal = coNmtLocalStateReq(CO_NMT_STATE_OPERATIONAL);
	if (retVal != RET_OK)  {
		return(retVal);
	}

	/* start slave with NMT all nodes ? */
	startUp = icoNmtMasterGetStartupObj();
	if (((startUp & CO_NMT_STARTUP_BIT_STARTNMT0) != 0u)
	 && ((startUp & CO_NMT_STARTUP_BIT_STARTNODE) == 0u))  {

		/* all optional slaves already booted ? */
		for (i = 0u; i < slaveCnt; i++)  {
			pSlave = getSlave(i);
			slaveAssign = getSlaveAssign(pSlave);
			if ((slaveAssign & CO_NMT_SLAVE_ASSIGN_BIT_BOOT) != 0u) {

				/* slave booted ? */
				if (pSlave->state != SLAVE_STATE_BOOTED)  {
					notBooted++;
				}
			}
		}

		if (notBooted == 0u)  {
			/* NMT for all nodes */
			sendNmtCmd(0u, CO_NMT_STATE_OPERATIONAL);
		} else {
			for (i = 0u; i < slaveCnt; i++)  {
				pSlave = getSlave(i);
				slaveAssign = getSlaveAssign(pSlave);
				if (((slaveAssign & CO_NMT_SLAVE_ASSIGN_BIT_BOOT) != 0u)
				 &&	(pSlave->state == SLAVE_STATE_BOOTED))  {
					sendNmtCmd(pSlave->nodeId, CO_NMT_STATE_OPERATIONAL);
				}
			}
		}
	}

	managerIndication(0u, CO_MANAGER_EVENT_FINISHED);

	return(RET_OK);
}


/***************************************************************************/
/**
* \internal
*
* \brief sendNmtCmd - send a NMT command
*
* Function request to send a NMT command.
* If the inhibit time is not up, try it again later
*
*/
static void sendNmtCmd(
		UNSIGNED8		slave,
		CO_NMT_STATE_T	cmd
	)
{
SLAVE_DATA_T *pSlave;

	if (slave == 0u)  {
		/* save NMT command */
		masterNmtCmd = cmd;
		/* call transmit routine */
		nmtCmd(NULL);
	} else {
		pSlave = getSlaveByNodeId(slave);

		/* save NMT command */
		pSlave->nmtCmd = cmd;
		/* call transmit routine */
		nmtCmd(pSlave);
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief nmtCmd - send a NMT command
*
* If the inhibit time is not up, try it again later
*
*/
static void nmtCmd(
		void			*pData
	)
{
RET_T	retVal;
SLAVE_DATA_T	*pSlave = (SLAVE_DATA_T *)pData;

	/* for all nodes ? */
	if (pData == NULL)  {
		/* master start for all nodes */
		retVal = coNmtStateReq(0u, masterNmtCmd, CO_FALSE);
		if (retVal != RET_OK)  {
			/* call function again */
			(void)icoEventStart(&managerEvent, nmtCmd, NULL); /*lint !e960 */
			/* Derogation MisraC2004 R.16.9 function identifier used without '&'
			 * or parenthesized parameter */
		}
	} else {
		/* slave */
		retVal = coNmtStateReq(pSlave->nodeId, pSlave->nmtCmd, CO_FALSE);
		if (retVal != RET_OK)  {
			/* call function again */
			(void)icoEventStart(&pSlave->event, nmtCmd, pData); /*lint !e960 */
			/* Derogation MisraC2004 R.16.9 function identifier used without '&'
			 * or parenthesized parameter */
		}
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief guardingIndication - heartbeat indication function
*
* Is called at heartbea/bootup events
*
*/
static void guardingIndication(
		UNSIGNED8		nodeId,			/* nodeId */
		CO_ERRCTRL_T	event,			/* event */
		CO_NMT_STATE_T	nmtState		/* actual nmt state */
	)
{
SLAVE_DATA_T *pSlave;
(void)nmtState;

	/* only for slaves at 0x1f81 */
	pSlave = getSlaveByNodeId(nodeId);
	if (pSlave == NULL)  {
		return;
	}

	/* HB started or new NMT state */
	if (((event == CO_ERRCTRL_HB_STARTED) || (event == CO_ERRCTRL_NEW_STATE))
	 && (pSlave->state == SLAVE_STATE_WAIT_ERRCTRL))  {

		/* continue boot */
		errorCtrlReady(pSlave);
	}

	/* Bootup after restore parameter */
	if ((event == CO_ERRCTRL_BOOTUP)
	 && (pSlave->state == SLAVE_STATE_CHECK_RESTORE))  {
		/* continue with check software */
		checkSoftwareVersion(pSlave);
	}

	/* HB failed */
	if ((event == CO_ERRCTRL_HB_FAILED)
	 || (event == CO_ERRCTRL_MGUARD_FAILED))  {
		/* bootup phase finished ? */
		if (managerState == MGR_STATE_RDY_OPERATIONAL)  {
			/* eror handler */
			errorHandler(pSlave);
		} else {
			/* error State K */
			managerErrorIndication(pSlave, ERROR_STATE_K);
		}
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief sdoReadAnswer - sdo read indication
*
*
*
*/
static void sdoReadAnswer(
		UNSIGNED8	sdoNr,				/* sdo number */
		UNSIGNED16	index,				/* object index */
		UNSIGNED8	subIndex,			/* object subindex */
		UNSIGNED32	size,				/* size of received data */
		UNSIGNED32	result				/* result of transfer */
	)
{
SLAVE_DATA_T	*pSlave;
SLAVE_STATE_T	state;
(void)index;
(void)subIndex;
(void)size;

	pSlave = getSlave(sdoNr - 1u);
	if (pSlave == NULL)  {
		return;
	}

	/* if error */
	if (result != 0ul)  {
		state = pSlave->state;
		pSlave->state = SLAVE_STATE_FAILURE;

		/* if 0x1000:0 failed
		 * or 0x1018:1 returns with invalid value (get wrong multiplexer)
		 * try it again after 1 sec
		 */
		if ((state == SLAVE_STATE_SDO_1000)
		 || ((state == SLAVE_STATE_SDO_IDENT_1) && (result == 0x06090030))) {
			/* start it again after 1 sec */
			(void)coTimerStart(&pSlave->timer, 1000ul * 1000u,
				readObj1000, (void *)pSlave, CO_TIMER_ATTR_ROUNDUP); /*lint !e960 */
			/* Derogation MisraC2004 R.16.9 function identifier used without '&'
			 * or parenthesized parameter */

			/* stay in state read 1000 */
			pSlave->state = SLAVE_STATE_SDO_1000;
			return;
		}
		if (state == SLAVE_STATE_SDO_IDENT_1)  {
			managerErrorIndication(pSlave, ERROR_STATE_D);
		}
		if (state == SLAVE_STATE_SDO_IDENT_2)  {
			managerErrorIndication(pSlave, ERROR_STATE_M);
		}
		if (state == SLAVE_STATE_SDO_IDENT_3)  {
			managerErrorIndication(pSlave, ERROR_STATE_N);
		}
		if (state == SLAVE_STATE_SDO_IDENT_4)  {
			managerErrorIndication(pSlave, ERROR_STATE_O);
		}

		if (state == SLAVE_STATE_SDO_1020_1)  {
			updateConfiguration(pSlave);
		}
		if (state == SLAVE_STATE_SDO_1020_2)  {
			updateConfiguration(pSlave);
		}


		return;
	}

	/* normal answer received - call appriate function */
	if ((pSlave->state == SLAVE_STATE_SDO_1000)
	 || (pSlave->state == SLAVE_STATE_SDO_IDENT_1)
	 || (pSlave->state == SLAVE_STATE_SDO_IDENT_2)
	 || (pSlave->state == SLAVE_STATE_SDO_IDENT_3)
	 || (pSlave->state == SLAVE_STATE_SDO_IDENT_4))  {
		/* read identity */
		readIdentity(pSlave);
		return;
	}

	if ((pSlave->state == SLAVE_STATE_SDO_1020_1)
	 || (pSlave->state == SLAVE_STATE_SDO_1020_2)) {
		checkConfigData(pSlave);
	}
}




/***************************************************************************/
/**
* \internal
*
* \brief readObj1000 - call reading 0x1000 again after timeout
*
*
*
*/
static void readObj1000(
		void			*ptr
	)
{
UNSIGNED32	slaveAssign;
RET_T	retVal;
SLAVE_DATA_T *pSlave = (SLAVE_DATA_T *)ptr;

	/* mandatory slave are only restarted if bootup time was not ellapsed */
	slaveAssign = getSlaveAssign(pSlave);

	/* mandatory slave ? */
	if ((slaveAssign & CO_NMT_SLAVE_ASSIGN_BIT_MANDATORY) != 0u)  {
		/* call it again until boot timer is ellapsed */
		if (coTimerIsActive(&bootupTimer) == CO_FALSE)  {
			managerErrorIndication(pSlave, ERROR_STATE_B);
			return;
		}
	}

	/* start SDO transfer again */
	retVal = coSdoRead(pSlave->sdoNr, 0x1000u, 0u,
			(UNSIGNED8 *)&pSlave->u32, 4u, 1u, CO_MGRBOOT_SDO_TO); /*lint !e928 */
	/* Derogation MisraC2004 R.11.4 cast from pointer to pointer */
	if (retVal != RET_OK)  {
		/* set error */
		managerErrorIndication(pSlave, ERROR_STATE_B);
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief getSlaveAssign - get slave assign value for 1 node
*
*
*
*/
static UNSIGNED32 getSlaveAssign(
		CO_CONST SLAVE_DATA_T	*pSlave
	)
{
UNSIGNED32	slaveAssign;

	/* get pointer to assign list */
	slaveAssign = icoNmtMasterGetSlaveAssignment(pSlave->nodeId - 1u);

	/* return requested node */
	return(slaveAssign);
}


/***************************************************************************/
/**
* \internal
*
* \brief bootTimeFinished - boot time finished
*
*
*
*/
static void bootTimeFinished(
		void			*ptr
	)
{
(void)ptr;

/*	printf("Manager: boottime is over\n"); */
	if (managerState != MGR_STATE_RDY_OPERATIONAL)  {
		managerIndication(0u, CO_MANAGER_EVENT_FAILURE);
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief errorHandler - called for failed slaves
*
*
*
*/
static void errorHandler(
		SLAVE_DATA_T	*pSlave
	)
{
UNSIGNED32	startUp;
UNSIGNED32	slaveAssign;

	startUp = icoNmtMasterGetStartupObj();

	/* slave in network list */
	slaveAssign = getSlaveAssign(pSlave);
	if ((slaveAssign & CO_NMT_SLAVE_ASSIGN_BIT_SLAVE) == 0u)  {
		return;
	}

	/* inform application */
	managerIndication(pSlave->nodeId, CO_MANAGER_EVENT_ERROR_NODE);

	/* mandatory node + stop all nodes ? */
	if (((slaveAssign & CO_NMT_SLAVE_ASSIGN_BIT_MANDATORY) != 0u)
	 &&	((startUp & CO_NMT_STARTUP_BIT_STOPNODES) != 0u))  {
		/* stop remote nodes with 0 */
		sendNmtCmd(0u, CO_NMT_STATE_STOPPED);
	} else
	if (((slaveAssign & CO_NMT_SLAVE_ASSIGN_BIT_MANDATORY) != 0u)
	 &&	((startUp & CO_NMT_STARTUP_BIT_RESETNODES) != 0u))  {
		/* reset remote nodes with 0 */
		sendNmtCmd(0u, CO_NMT_STATE_RESET_NODE);
	} else {
		/* reset node individually */
		sendNmtCmd(pSlave->nodeId, CO_NMT_STATE_RESET_NODE);

		/* boot slave again */
		bootSlave(pSlave);
	}
}


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/**
* \internal
*
* \brief managerErrorIndication
*
*
*
*/
static void managerErrorIndication(
		SLAVE_DATA_T	*pSlave,
		MANAGER_EVENT_T	event
	)
{
UNSIGNED32	slaveAssign;

	switch (event)  {
		case ERROR_STATE_A:
			return;
			break;
		case ERROR_STATE_B:
			managerIndication(pSlave->nodeId, CO_MANAGER_EVENT_ERROR_B);
			break;
		case ERROR_STATE_C:
			managerIndication(pSlave->nodeId, CO_MANAGER_EVENT_ERROR_C);
			break;
		case ERROR_STATE_J:
			managerIndication(pSlave->nodeId, CO_MANAGER_EVENT_ERROR_J);
			break;
		case ERROR_STATE_G:
			managerIndication(pSlave->nodeId, CO_MANAGER_EVENT_ERROR_G);
			break;
		case ERROR_STATE_D:
			managerIndication(pSlave->nodeId, CO_MANAGER_EVENT_ERROR_D);
			break;
		case ERROR_STATE_K:
			managerIndication(pSlave->nodeId, CO_MANAGER_EVENT_ERROR_K);
			break;
		case ERROR_STATE_M:
			managerIndication(pSlave->nodeId, CO_MANAGER_EVENT_ERROR_M);
			break;
		case ERROR_STATE_N:
			managerIndication(pSlave->nodeId, CO_MANAGER_EVENT_ERROR_N);
			break;
		case ERROR_STATE_O:
			managerIndication(pSlave->nodeId, CO_MANAGER_EVENT_ERROR_O);
			break;
		default:
			break;
	}

	/* set slave failure state */
	pSlave->state = SLAVE_STATE_FAILURE;

	/* if mandatory slave ? */
	slaveAssign = getSlaveAssign(pSlave);
	if ((slaveAssign & CO_NMT_SLAVE_ASSIGN_BIT_MANDATORY) != 0u)  {
		managerState = MGR_STATE_FAILURE;

		/* inform application and finish boot process */
		(void)coTimerStop(&bootupTimer);

		managerIndication(0u, CO_MANAGER_EVENT_FAILURE);
	}
	
}


/***************************************************************************/
/**
* \brief coManagerGetUsedSdoChannel - get used SDO channel
*
* This function returns the used sdo channel for the given slave.
*
* \return 255 - invalid slave
* \return 1..127	sdo channel
*
*
*/
UNSIGNED8 coManagerGetUsedSdoChannel(
		UNSIGNED8		slave
	)
{
SLAVE_DATA_T	*pSlave;

	pSlave = getSlaveByNodeId(slave);
	if (pSlave == NULL)  {
		return(255u);
	}

	return(pSlave->sdoNr);
}


# ifdef CO_EVENT_MANAGER_CNT
/***************************************************************************/
/**
* \brief coEventRegister_MANAGER_BOOTUP - register MANAGER_BOOTUP event
*
* register indication function for MANAGER_BOOTUP Procedure events
*
* \return RET_T
*
*/

RET_T coEventRegister_MANAGER_BOOTUP(
		CO_EVENT_MANAGER_BOOTUP_T pFunction	/**< pointer to function */
    )
{
	/* set new indication function as first at the list */
	if (managerTableCnt >= CO_EVENT_MANAGER_CNT) {
		return(RET_EVENT_NO_RESSOURCE);
	}

	managerTable[managerTableCnt] = pFunction;	/* save function pointer */
	managerTableCnt++;

	return(RET_OK);
}
# endif /* CO_EVENT_MANAGER */


/***************************************************************************/
/**
* \internal
*
* \brief managerIndication - indication to application
*
*
* \return void
*
*/

static void managerIndication(
		UNSIGNED8		node,
		CO_MANAGER_EVENT_T	event
	)
{
#  ifdef CO_EVENT_MANAGER_CNT
UNSIGNED16	cnt;

	cnt = managerTableCnt;
	while (cnt > 0u)  {
		cnt--;

		/* check user indication */
		managerTable[cnt](node, event);
	}
#  endif /* CO_EVENT_MANAGER_MANAGER */

#  ifdef CO_EVENT_STATIC_MANAGER
	coEventManagerInd(node, event);
#  endif /* CO_EVENT_STATIC_MANAGER */

	(void)node;
	(void)event;
}


/***************************************************************************/
/**
* \internal
*
* \brief getSlave - return slave pointer
*
* \return void
*
*/
static SLAVE_DATA_T	*getSlave(
		UNSIGNED8		idx
	)
{
	if (idx < CO_MAX_SLAVE_ASSIGNMENT_CNT)  {
		return(&slaveData[idx]);
	}

	return(NULL);
}


/***************************************************************************/
/**
* \internal
*
* \brief getSlaveByNodeid - return slave pointer
*
* \return void
*
*/
static SLAVE_DATA_T	*getSlaveByNodeId(
		UNSIGNED8		nodeId
	)
{
UNSIGNED8 i;

	for (i = 0u; i < slaveCnt;	i++)  {
		if (nodeId == slaveData[i].nodeId)  {
			return(&slaveData[i]);
		}
	}

	return(NULL);
}


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/*
* \brief icoManagerVarInit - init manager variables
*
*/
void icoManagerVarInit(
		UNSIGNED8	*pSlaveAssign		/* line counts */
	)
{
(void)pSlaveAssign;

# ifdef CO_EVENT_MANAGER_CNT
	managerTableCnt = 0u;
# endif /* CO_EVENT_MANAGER_CNT */

	initState = CO_FALSE;

}
#endif /* CO_BOOTUP_MANAGER */
