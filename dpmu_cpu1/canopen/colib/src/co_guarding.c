/*
* co_guarding.c - contains guarding master services
*
* Copyright (c) 2012-2022 emotas embedded communication GmbH
*-------------------------------------------------------------------
* $Id: co_guarding.c 41686 2022-08-15 13:40:31Z boe $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \brief Gaurding Master services
*
* \file co_guarding.c
* Contains gurading master routines.
*
* Functions are only available in Classical CAN mode
*/


/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <stddef.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <gen_define.h>

#ifdef CO_GUARDING_CNT
#include <co_datatype.h>
#include <co_timer.h>
#include <co_odaccess.h>
#include <co_drv.h>
#include <co_emcy.h>
#include <co_led.h>
#include <co_nmt.h>
#include "ico_indication.h"
#include "ico_cobhandler.h"
#include "ico_queue.h"
#include "ico_emcy.h"
#include "ico_nmt.h"

/* constant definitions
---------------------------------------------------------------------------*/
#define GUARD_TOGGLE_BIT	0x80u

/* local defined data types
---------------------------------------------------------------------------*/
typedef struct {
	CO_TIMER_T			timer;
	COB_REFERENZ_T		cob;
	CO_NMT_STATE_T		nmtState;
	UNSIGNED16			guardTime;
	UNSIGNED8			node;
	UNSIGNED8			toggleBit;
	UNSIGNED8			lifeTime;
	UNSIGNED8			guardFailCnt;
} CO_GUARDING_T;


/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
static void guardingRequest(void *pData);
static void guardSendRequest(CO_GUARDING_T *pGuard);
static CO_GUARDING_T *getGuardingSlaveIdx(UNSIGNED16 idx);
static CO_GUARDING_T *getGuardingSlave(UNSIGNED8 node);

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
static CO_GUARDING_T	guardSlave[CO_GUARDING_CNT];


/***************************************************************************/
/**
*
* \brief coGuardingMasterStart - start master node guarding
*
* This function is only available in classical CAN mode.
*
* This function starts the master node guarding monitoring
* for the given node-id 
* and the configured monitoring time from object dictionary.
*
* Please note: The NMT state is set to unknown until next guarding was received
*
* \return RET_T
* \retval RET_PARAMETER_INCOMPATIBLE
*	invalid node id
*
*/

RET_T coGuardingMasterStart(
		UNSIGNED8	node			/**< node id */
	)
{
UNSIGNED8	ownNodeId;
RET_T		retVal;
UNSIGNED32	slaveAssign;
UNSIGNED32	id;
CO_GUARDING_T	*pGuard;

	/* check for valid node id */
	ownNodeId = coNmtGetNodeId();
	if ((node == 0u) || (node == ownNodeId) || (node > 127u))  {
		return(RET_PARAMETER_INCOMPATIBLE);
	}

	pGuard = getGuardingSlave(node);
	if (pGuard == NULL)  {
		return(RET_EVENT_NO_RESSOURCE);
	}

	/* get guarding para - located at 0x1f81 */
	slaveAssign = icoNmtMasterGetSlaveAssignment(node - 1u);

	pGuard->guardTime = (UNSIGNED16)((slaveAssign >> 16) & 0xffffu);
	pGuard->lifeTime = (UNSIGNED8)((slaveAssign >> 8) & 0xffu);

	/* guard time have to be != 0 */
	if (pGuard->guardTime == 0u)  {
		return(RET_INVALID_PARAMETER);
	}

	id = 0x700ul + node;
	retVal = icoCobSet(pGuard->cob, id, CO_COB_RTR, 1u);
	if (retVal != RET_OK)  {
		return(retVal);
	}

	pGuard->node = node;
	pGuard->toggleBit = 0u;
	pGuard->guardFailCnt = 0u;

	/* send first request */
	guardSendRequest(pGuard);

	pGuard->nmtState = CO_NMT_STATE_UNKNOWN;

	return(retVal);
}


/***************************************************************************/
/**
*
* \brief coGuardingMasterStop - stop master node guarding
*
* This function is only available in classical CAN mode.
*
* This function stops the master node guarding monitoring
* for the given node-id 
*
* \return RET_T
* \retval RET_PARAMETER_INCOMPATIBLE
*	invalid node id
*
*/

RET_T coGuardingMasterStop(
		UNSIGNED8	node			/**< node id */
	)
{
UNSIGNED8	ownNodeId;
RET_T		retVal;
CO_GUARDING_T	*pGuard;

	/* check for valid node id */
	ownNodeId = coNmtGetNodeId();
	if ((node == 0u) || (node == ownNodeId) || (node > 127u))  {
		return(RET_PARAMETER_INCOMPATIBLE);
	}

	pGuard = getGuardingSlave(node);
	if (pGuard == NULL)  {
		return(RET_PARAMETER_INCOMPATIBLE);
	}

	/* guarding active ? */
	if (coTimerIsActive(&pGuard->timer) == CO_FALSE)  {
		return(RET_OK);
	}

	pGuard->nmtState = CO_NMT_STATE_UNKNOWN;
	pGuard->node = 0u;

	/* disable cob */
	retVal = icoCobSet(pGuard->cob, CO_COB_INVALID, CO_COB_RTR, 0u);

	/* stop timer */
	(void)coTimerStop(&pGuard->timer);

	return(retVal);
}


/***************************************************************************/
/**
* \internal
*
* \brief guardSendRequest - send guarding request
*
*
* is called from timer
*
* \return none
*
*/
static void guardSendRequest(
		CO_GUARDING_T	*pGuard
	)
{
UNSIGNED8	transmitData[CO_CAN_MAX_DATA_LEN];	/* data */

	transmitData[0] = 0u;
	(void)icoTransmitMessage(pGuard->cob, &transmitData[0], 0u);

	/* start timer */
	(void)coTimerStart(&pGuard->timer, pGuard->guardTime * 1000ul,
				guardingRequest, pGuard, CO_TIMER_ATTR_ROUNDUP); /*lint !e960 */
	/* Derogation MisraC2004 R.16.9 function identifier used without '&'
	 * or parenthesized parameter */
}


/***************************************************************************/
/**
* \internal
*
* \brief nmtErrorCtrlHandler - error control handler
*
* is called, if new error control message was received
*
*
* \return none
*
*/
void icoGuardingHandler(
		CO_CONST CO_REC_DATA_T	*pRecData	/* pointer to receive data */
	)
{
BOOL_T	startMonitoring = CO_FALSE;
CO_NMT_STATE_T	oldState;
CO_GUARDING_T	*pGuard;

	/* check for correct message len */
	if (pRecData->msg.len != 1u)  {
		return;
	}

	/* unknown bootup message ? */
	if (pRecData->service != CO_SERVICE_GUARDING)  {
		if (pRecData->msg.data[0u] == 0u) {
			icoErrCtrlInd((UNSIGNED8)(pRecData->msg.canId & 0x7fu),
				CO_ERRCTRL_BOOTUP,
				CO_NMT_STATE_PREOP);
		}
		return;
	}

	pGuard = getGuardingSlaveIdx(pRecData->spec);
	oldState = pGuard->nmtState;

	/* check toggle bit */
	if ((pRecData->msg.data[0u] & GUARD_TOGGLE_BIT) != pGuard->toggleBit)  {
		/* doesn't fit */
		pGuard->nmtState = CO_NMT_STATE_UNKNOWN;
		icoErrCtrlInd(pGuard->node, CO_ERRCTRL_MGUARD_TOGGLE,
			pGuard->nmtState);
		return;
	}

	if (pGuard->toggleBit != 0u)  {
		pGuard->toggleBit = 0u;
	} else {
		pGuard->toggleBit = GUARD_TOGGLE_BIT;
	}
	pGuard->guardFailCnt = 0u;

	/* save nmt state */
	switch (pRecData->msg.data[0u] & (UNSIGNED8)(~GUARD_TOGGLE_BIT))  {
		case 0u:
			pGuard->nmtState = CO_NMT_STATE_PREOP;
			/* now further indication */
			oldState = pGuard->nmtState;

			/* user indication */
			icoErrCtrlInd(pGuard->node, CO_ERRCTRL_BOOTUP,
				pGuard->nmtState);
			break;
		case 4u:
			pGuard->nmtState = CO_NMT_STATE_STOPPED;
			startMonitoring = CO_TRUE;
			break;
		case 5u:
			pGuard->nmtState = CO_NMT_STATE_OPERATIONAL;
			startMonitoring = CO_TRUE;
			break;
		case 127u:
			pGuard->nmtState = CO_NMT_STATE_PREOP;
			startMonitoring = CO_TRUE;
			break;
		default:
			pGuard->nmtState = CO_NMT_STATE_UNKNOWN;
			break;
	}

	/* start heartbeat monitoring (again) */
	if (startMonitoring == CO_TRUE)  {
#ifdef CO_EVENT_LED
		/* set led */
		coLedSetState(CO_LED_STATE_FLASH_2, CO_FALSE);
#endif /* CO_EVENT_LED */
	}

	/* user indication */
	if (oldState != pGuard->nmtState)  {
		/* user indication */
		icoErrCtrlInd(pGuard->node, CO_ERRCTRL_NEW_STATE,
			pGuard->nmtState);
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief guardingRequest - guarding failure
*
* Function is called, if guarding time is over
* Check, if a guarding was received
* else check if lifetime is over.
* In this case, stop monitoring and inform applicaton
*
* \return none
*
*/
static void guardingRequest(
		void			*pData		/* pointer guard */
	)
{
CO_GUARDING_T	*pGuard;

	pGuard = (CO_GUARDING_T *)pData;

	if (pGuard->guardFailCnt < pGuard->lifeTime)  {
		pGuard->guardFailCnt++;

		/* send next request */
		guardSendRequest(pGuard);
		return;
	}

	/* inform application */
	icoErrCtrlInd(pGuard->node, CO_ERRCTRL_MGUARD_FAILED, CO_NMT_STATE_UNKNOWN);

	/* disable cob */
	(void)icoCobSet(pGuard->cob, CO_COB_INVALID, CO_COB_RTR, 0u);
	pGuard->node = 0u;

#ifdef CO_EMCY_PRODUCER
	{
	UNSIGNED8 strg[5] = { 0u, 0u, 0u, 0u, 0u };

	(void)icoEmcyWriteReq(CO_EMCY_ERRCODE_COMM_ERROR, strg);
	}
#endif /* CO_EMCY_PRODUCER */

#ifdef CO_EVENT_LED
	/* set led */
	coLedSetState(CO_LED_STATE_FLASH_2, CO_TRUE);
#endif /* CO_EVENT_LED */

	/* change to preop */
	icoErrorBehavior();

#ifdef CO_FLYING_MASTER_SUPPORTED
	icoNmtFlymaErrCtrlFailure(pGuard->node);
#endif /* CO_FLYING_MASTER_SUPPORTED */
}


/***************************************************************************/
/**
* \internal
*
* \brief coNmtGetRemoteNodeState - get remote node state
*
* This function is only available in classical CAN mode.
*
* This function returns the NMT state of a remote node.
* If guarding monitoring of this node is disabled or has been failed,
* CO_NMT_STATE_UNKNOWN is returned.
*
*
* \return CO_NMT_STATE_T 
*
*/
CO_NMT_STATE_T icoGuardGetRemoteNodeState(
		UNSIGNED8 nodeId			/* remote node id */
	)
{
CO_GUARDING_T	*pGuard;

	pGuard = getGuardingSlave(nodeId);
	if (pGuard == NULL)  {
		return(CO_NMT_STATE_UNKNOWN);
	}

	/* guarding active ? */
	if (coTimerIsActive(&pGuard->timer) == CO_TRUE)  {
		return(pGuard->nmtState);
	}
	return(CO_NMT_STATE_UNKNOWN);
}


/***************************************************************************/
/**
* \internal
*
* \brief getGuardingSlaveIdx
*
*
*/
static CO_GUARDING_T *getGuardingSlaveIdx(
		UNSIGNED16		idx
	)
{
	return(&guardSlave[idx]);
}


/***************************************************************************/
/**
* \internal
*
* \brief getGuardingSlave
*
*
*/
static CO_GUARDING_T *getGuardingSlave(
		UNSIGNED8		slave
	)
{
UNSIGNED16	i;
CO_GUARDING_T *pSlave;
CO_GUARDING_T *pFreeSlave = NULL;

	/* for all slave structures */
	for (i = 0u; i < CO_GUARDING_CNT; i++)  {
		pSlave = getGuardingSlaveIdx(i);

		/* slave already in list ? */
		if (pSlave->node == slave)  {
			return(pSlave);
		}

		/* first free entry ? */
		if ((pSlave->node == 0u) && (pFreeSlave == NULL))  {
			pFreeSlave = pSlave;
		}
	}

	return(pFreeSlave);
}


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/*
* \brief icoGuardingVarInit - init guarding variables
*
*/
void icoGuardingVarInit(
		UNSIGNED8	*pSlaveAssign		/* line counts */
	)
{
UNSIGNED16		i;
(void)pSlaveAssign;

	{
	}

	for (i = 0u; i < CO_GUARDING_CNT; i++)  {
		guardSlave[i].cob = 0xffffu;
	}
}


/***************************************************************************/
RET_T coGuardingInit(
		void	/* no parameter */
	)
{
UNSIGNED16	i;

	for (i = 0u; i < CO_GUARDING_CNT; i++)  {
		guardSlave[i].cob = icoCobCreate(CO_COB_TYPE_RECEIVE,
				CO_SERVICE_GUARDING, i + 1u);
		if (guardSlave[i].cob == 0xffffu)  {
			return(RET_NO_COB_AVAILABLE);
		}
	}

	return(RET_OK);
}

#endif /* CO_GUARDING_CNT */
