/*
* co_heartbt.c - contains heartbeat services
*
* Copyright (c) 2012-2022 emotas embedded communication GmbH
*-------------------------------------------------------------------
* $Id: co_errctrl.c 41643 2022-08-10 10:29:15Z boe $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \brief Error control handling (Heartbeat, Guarding)
*
* \file co_errctrl.c
* Contains error control routines to handle Heartbeat or Guarding.
*
*/


/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <stddef.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <gen_define.h>
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
#ifdef CO_EVENT_DYNAMIC_ERRCTRL
# ifdef CO_EVENT_PROFILE_ERRCTRL
#  define CO_EVENT_ERRCTRL_CNT	(CO_EVENT_DYNAMIC_ERRCTRL + CO_EVENT_PROFILE_ERRCTRL)
# else /* CO_EVENT_PROFILE_ERRCTRL */
#  define CO_EVENT_ERRCTRL_CNT	(CO_EVENT_DYNAMIC_ERRCTRL)
# endif /* CO_EVENT_PROFILE_ERRCTRL */
#else /* CO_EVENT_DYNAMIC_ERRCTRL */
# ifdef CO_EVENT_PROFILE_ERRCTRL
#  define CO_EVENT_ERRCTRL_CNT	(CO_EVENT_PROFILE_ERRCTRL)
# endif /* CO_EVENT_PROFILE_ERRCTRL */
#endif /* CO_EVENT_DYNAMIC_ERRCTRL */


/* local defined data types
---------------------------------------------------------------------------*/
typedef struct {
	CO_TIMER_T			timer;
#ifdef CO_ONE_HB_CONSUMER_COB
#else /* CO_ONE_HB_CONSUMER_COB */
	COB_REFERENZ_T		cob;
#endif /* CO_ONE_HB_CONSUMER_COB */
	UNSIGNED16			hbTime;
	UNSIGNED8			node;
	UNSIGNED32			odVal;
	CO_NMT_STATE_T		nmtState;
} CO_HB_CONSUMER_T;


/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/
#ifdef CO_EVENT_STATIC_ERRCTRL
extern CO_CONST CO_EVENT_ERRCTRL_T coEventErrCtrlInd;
#endif /* CO_EVENT_STATIC_ERRCTRL */


/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
#ifdef CO_HB_CONSUMER_CNT
static RET_T setupHbConsumer(CO_HB_CONSUMER_T	*pHbCons);
static void hbConsFailure(void *pData);
static CO_INLINE CO_HB_CONSUMER_T *getHbConsumer(UNSIGNED8 idx);
#endif /* CO_HB_CONSUMER_CNT */
static void hbProdSendMessage(void *ptr);
#ifdef CO_GUARDING_SLAVE
static void guardFail(void *ptr);
#endif /* CO_GUARDING_SLAVE */

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CO_EVENT_ERRCTRL_CNT
static CO_EVENT_ERRCTRL_T	errCtrlTable[CO_EVENT_ERRCTRL_CNT];
static UNSIGNED8			errCtrlTableCnt;
#endif /* CO_EVENT_ERRCTRL_CNT */
static COB_REFERENZ_T		errCtrlCob;
static UNSIGNED16			hbProdTime;
static CO_TIMER_T			hbProdTimer;
static UNSIGNED8			errorBehavior;
#ifdef CO_HB_CONSUMER_CNT
static CO_HB_CONSUMER_T		hbConsumer[CO_HB_CONSUMER_CNT];
# ifdef CO_ONE_HB_CONSUMER_COB
static COB_REFERENZ_T		hbConsCobRef;
# endif /* CO_ONE_HB_CONSUMER_COB */
#endif /* CO_HB_CONSUMER_CNT */
#ifdef CO_GUARDING_SLAVE
static UNSIGNED8			toggleBit;
static UNSIGNED16			guardTime;
static UNSIGNED8			lifeTimeFactor;
static UNSIGNED8			guardFailCnt; 
#endif /* CO_GUARDING_SLAVE */


#ifdef CO_HB_CONSUMER_CNT
/***************************************************************************/
/**
*
* \brief coHbConsumerSet - setup heartbeat consumer
*
* This function configures a hearbeat consumer for the given node-id 
* and the monitoring time.
* The data are automatically saved at the object dictionary.
* If an entry at the object dictionary already exist,
* then it will be overwritten.
* <br>The parameter node have to be valid,
* otherwise the function returns an error.
*
* \return RET_T
* \retval RET_PARAMETER_INCOMPATIBLE
*	invalid node id
*
*/
RET_T coHbConsumerSet(
		UNSIGNED8	node,			/**< node id */
		UNSIGNED16	hbTime			/**< heartbeat monitoring time */
	)
{
UNSIGNED8	ownNodeId;
UNSIGNED8	idx = 255u;
UNSIGNED8	i;
RET_T		retVal;
CO_HB_CONSUMER_T *pHbCons = NULL;

	/* check for valid node id */
	ownNodeId = coNmtGetNodeId();
	if ((node == 0u) || (node == ownNodeId) || (node > 127u))  {
		return(RET_PARAMETER_INCOMPATIBLE);
	}

	/* node already available ? */
	for (i = 0u; i < CO_HB_CONSUMER_CNT; i++)  {
		pHbCons = getHbConsumer(i);
		if (pHbCons->node == node)  {
			idx = i;
			break;
		}
	}

	/* entry found ? */
	if (idx == 255u)  {
		/* no, try to find a empty one */
		for (i = 0u; i < CO_HB_CONSUMER_CNT; i++)  {
			pHbCons = getHbConsumer(i);
			if (pHbCons->node == 0u)  {
				idx = i;
				break;
			}
		}
	}

	if (idx == 255u)  {
		retVal = RET_EVENT_NO_RESSOURCE;
	} else {

#ifdef CO_GUARDING_CNT
		/* disable guarding master */
		retVal = coGuardingMasterStop(node);
		if (retVal != RET_OK)  {
			return(retVal);
		}
#endif /* CO_GUARDING_CNT */

		pHbCons->odVal = ((UNSIGNED32)node << 16u) | hbTime;
		retVal = setupHbConsumer(pHbCons);
	}

	return(retVal);
}


/***************************************************************************/
/**
*
* \brief coHbConsumerStart - start heartbeat consumer monitoring
*
* This function starts a hearbeat consumer monitoring for the given node-id 
* and the configured monitoring time from object dictionary.
*
* Please note: The NMT state is set to unknown until next HB was received
*
* \return RET_T
* \retval RET_PARAMETER_INCOMPATIBLE
*	invalid node id
*
*/
RET_T coHbConsumerStart(
		UNSIGNED8	node			/**< node id */
	)
{
UNSIGNED8	ownNodeId;
UNSIGNED8	i;
RET_T		retVal = RET_OK;
CO_HB_CONSUMER_T	*pHbC = NULL;

	/* check for valid node id */
	ownNodeId = coNmtGetNodeId();
	if ((node == 0u) || (node == ownNodeId) || (node > 127u))  {
		return(RET_PARAMETER_INCOMPATIBLE);
	}

	/* node already available ? */
	for (i = 0u; i < CO_HB_CONSUMER_CNT; i++)  {
		pHbC = getHbConsumer(i);
		if (pHbC->node == node)  {
			break;
		} else {
			pHbC = NULL;
		}
	}

	/* entry found ? */
	if (pHbC == NULL)  {
		retVal = RET_EVENT_NO_RESSOURCE;
	} else {
		/* start timer */
		(void)coTimerStart(&pHbC->timer,
				(UNSIGNED32)pHbC->hbTime * 1000ul,
				hbConsFailure, pHbC, CO_TIMER_ATTR_ROUNDUP); /*lint !e960 */
		/* Derogation MisraC2004 R.16.9 function identifier used without '&'
		 * or parenthesized parameter */

		/* set state to unknown to get heartbeat startet indication */
		pHbC->nmtState = CO_NMT_STATE_UNKNOWN;
	}

	return(retVal);
}


/***************************************************************************/
/**
*
* \brief coHbConsumerStop - stop heartbeat consumer monitoring
*
* This function stops a hearbeat consumer monitoring for the given node-id 
*
* \return RET_T
* \retval RET_PARAMETER_INCOMPATIBLE
*	invalid node id
*
*/
RET_T coHbConsumerStop(
		UNSIGNED8	node			/**< node id */
	)
{
UNSIGNED8	ownNodeId;
UNSIGNED8	i;
RET_T		retVal = RET_OK;
CO_HB_CONSUMER_T	*pHbC = NULL;

	/* check for valid node id */
	ownNodeId = coNmtGetNodeId();
	if ((node == 0u) || (node == ownNodeId) || (node > 127u))  {
		return(RET_PARAMETER_INCOMPATIBLE);
	}

	/* node already available ? */
	for (i = 0u; i < CO_HB_CONSUMER_CNT; i++)  {
		pHbC = getHbConsumer(i);
		if (pHbC->node == node)  {
			break;
		} else {
			pHbC = NULL;
		}
	}

	/* entry found ? */
	if (pHbC == NULL)  {
		retVal = RET_EVENT_NO_RESSOURCE;
	} else {
		/* start timer */
		(void)coTimerStop(&pHbC->timer);

		pHbC->nmtState = CO_NMT_STATE_UNKNOWN;
	}

	return(retVal);
}
#endif /* CO_HB_CONSUMER_CNT */


/***************************************************************************/
/**
* \internal
*
* \brief icoHbProdStart - start HB transmission
*
*
* \return RET_T
*
*/
RET_T icoHbProdStart(
		BOOL_T			send			/* transmit hb message */
	)
{
RET_T	retVal;

	if (hbProdTime == 0u)  {
		(void)coTimerStop(&hbProdTimer);
		return(RET_OK);
	}



	/* start timer */
	retVal = coTimerStart(&hbProdTimer,
		(UNSIGNED32)hbProdTime * 1000ul,
		hbProdSendMessage, NULL,
		CO_TIMER_ATTR_ROUNDUP_CYCLIC); /*lint !e960 */
	/* Derogation MisraC2004 R.16.9 function identifier used without '&'
	 * or parenthesized parameter */

	/* don't transmit hb message at reset comm */
	if (send == CO_TRUE)  {
		/* send first heartbeat */
		hbProdSendMessage(NULL);
	}

	return(retVal);
}


/***************************************************************************/
/**
* \internal
*
* \brief hbProdSendMessage - send HB message
*
*
* is called from timer
*
*
*/
static void hbProdSendMessage(
		void *ptr					/* compatibility entry from timer */
	)
{
UNSIGNED8		trData[CO_CAN_MAX_DATA_LEN];	/* data */
CO_NMT_STATE_T	nmtState;
(void)ptr;

	nmtState = coNmtGetState();
	trData[0u] = (UNSIGNED8)nmtState;

#ifdef CO_GUARDING_SLAVE
	/* heartbeat disabled */
	if (hbProdTime == 0u)  {
		trData[0u] |= toggleBit;
		if (toggleBit == 0u)  {
			toggleBit = 0x80u;
		} else {
			toggleBit = 0x0u;
		}

		/* start guarding timer, if guardingTime and lifeTime is set */
		if ((guardTime != 0u) && (lifeTimeFactor != 0u))  {
			(void)coTimerStart(&hbProdTimer,
				(UNSIGNED32)guardTime * 1000ul,
				guardFail, NULL, CO_TIMER_ATTR_ROUNDUP_CYCLIC); /*lint !e960 */
			/* Derogation MisraC2004 R.16.9 function identifier used without '&'			 * or parenthesized parameter */

			guardFailCnt = 0u;
		}
	}
#endif /* CO_GUARDING_SLAVE */

	(void)icoTransmitMessage(errCtrlCob, &trData[0], 0u);
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
void icoNmtErrorCtrlHandler(
		CO_CONST CO_REC_DATA_T	*pRecData	/* pointer to receive data */
	)
{
#ifdef CO_HB_CONSUMER_CNT
BOOL_T	startMonitoring = CO_FALSE;
CO_NMT_STATE_T		oldState;
BOOL_T				timerActive;
# ifdef CO_ONE_HB_CONSUMER_COB
UNSIGNED8			i;
# endif /* CO_ONE_HB_CONSUMER_COB */
#endif /* CO_HB_CONSUMER_CNT */
CO_HB_CONSUMER_T	*pHbC = NULL;
UNSIGNED32	ownNodeId;

	ownNodeId = coNmtGetNodeId();

	/* RTR request ? */
	if ((pRecData->msg.flags & CO_COBFLAG_RTR) != 0u)  {
		/* for own node ? */
		if (pRecData->msg.canId == (0x700u + ownNodeId))  {
			hbProdSendMessage(NULL);
		}
		return;
	}

	/* check for correct message len */
	if (pRecData->msg.len != 1u)  {
		return;
	}

	/* check for double identifier */
	if (pRecData->msg.canId == (0x700u + ownNodeId))  {
		/* receive message with my own node id - inform application */
		icoErrCtrlInd((UNSIGNED8)(pRecData->msg.canId & 0x7fu),
				CO_ERRCTRL_DOUBLE_ID, CO_NMT_STATE_UNKNOWN);
		return;
	}

	/* unknown bootup message ? */
#ifdef CO_HB_CONSUMER_CNT
# ifdef CO_ONE_HB_CONSUMER_COB
	for (i = 0u; i < CO_HB_CONSUMER_CNT; i++)  {
		pHbC = getHbConsumer(i);
		if (pHbC->node == (pRecData->msg.canId & 0x7fu))  {
			break;
		} else {
			pHbC = NULL;
		}
	}
# else /* CO_ONE_HB_CONSUMER_COB */
	if (pRecData->service == CO_SERVICE_ERRCTRL)  {
		pHbC = getHbConsumer((UNSIGNED8)pRecData->spec);
	}
# endif /* CO_ONE_HB_CONSUMER_COB */
#endif /* CO_HB_CONSUMER_CNT */

	if (pHbC == NULL)  {
		if (pRecData->msg.data[0u] == 0u) {
			icoErrCtrlInd((UNSIGNED8)(pRecData->msg.canId & 0x7fu),
				CO_ERRCTRL_BOOTUP,
				CO_NMT_STATE_PREOP);
		}
		return;
	}

#ifdef CO_HB_CONSUMER_CNT
	oldState = pHbC->nmtState;

	/* save nmt state */
	switch (pRecData->msg.data[0u])  {
		case 0u:
			pHbC->nmtState = CO_NMT_STATE_PREOP;
			/* now further indication */
			oldState = pHbC->nmtState;

			/* user indication */
			icoErrCtrlInd(pHbC->node, CO_ERRCTRL_BOOTUP, pHbC->nmtState);
			break;
		case 4u:
			pHbC->nmtState = CO_NMT_STATE_STOPPED;
			startMonitoring = CO_TRUE;
			break;
		case 5u:
			pHbC->nmtState = CO_NMT_STATE_OPERATIONAL;
			startMonitoring = CO_TRUE;
			break;
		case 127u:
			pHbC->nmtState = CO_NMT_STATE_PREOP;
			startMonitoring = CO_TRUE;
			break;
		default:
			pHbC->nmtState = CO_NMT_STATE_UNKNOWN;
			break;
	}

	/* start heartbeat monitoring (again) */
	if (startMonitoring == CO_TRUE)  {

		/* heartbeat already started ? */
		timerActive = coTimerIsActive(&pHbC->timer);

		/* start timer */
		(void)coTimerStart(&pHbC->timer,
				(UNSIGNED32)pHbC->hbTime * 1000ul,
				hbConsFailure, pHbC, CO_TIMER_ATTR_ROUNDUP); /*lint !e960 */
		/* Derogation MisraC2004 R.16.9 function identifier used without '&'
		 * or parenthesized parameter */

		if ((timerActive == CO_FALSE) || (oldState == CO_NMT_STATE_UNKNOWN))  {
			/* no, inform user about hearbeat start */
			icoErrCtrlInd(pHbC->node, CO_ERRCTRL_HB_STARTED,
				pHbC->nmtState);

# ifdef CO_EVENT_LED
			/* set led */
			coLedSetState(CO_LED_STATE_FLASH_2, CO_FALSE);
# endif /* CO_EVENT_LED */
		}

	}

	/* user indication */
	if (oldState != pHbC->nmtState)  {
		/* user indication */
		icoErrCtrlInd(pHbC->node, CO_ERRCTRL_NEW_STATE, pHbC->nmtState);
	}
#endif /* CO_HB_CONSUMER_CNT */
}


#ifdef CO_HB_CONSUMER_CNT
/***************************************************************************/
/**
* \internal
*
* \brief hbConsFailure - heartbeat consumer failure
*
* Function is called, if heartbeat consumer has been failed
*
*
*/
static void hbConsFailure(
		void			*pData		/* pointer heartbeat consumer */
	)
{
CO_HB_CONSUMER_T	*pHbC;

	pHbC = (CO_HB_CONSUMER_T *)pData;

	/* no, inform user about hearbeat start */
	icoErrCtrlInd(pHbC->node, CO_ERRCTRL_HB_FAILED, CO_NMT_STATE_UNKNOWN);

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
	icoNmtFlymaErrCtrlFailure(pHbC->node);
#endif /* CO_FLYING_MASTER_SUPPORTED */
}


/***************************************************************************/
/**
*
* \brief coNmtGetRemoteNodeState - get remote node state
*
* This function returns the NMT state of a remote node.
* If heartbeat monitoring of this node is disabled or has been failed,
* CO_NMT_STATE_UNKNOWN is returned.
*
*
* \return CO_NMT_STATE_T 
*
*/
CO_NMT_STATE_T coNmtGetRemoteNodeState(
		UNSIGNED8 nodeId			/**< remote node id */
	)
{
UNSIGNED8	cnt;
CO_NMT_STATE_T nmtState;
CO_HB_CONSUMER_T *pHbCons;

	for (cnt = 0u; cnt < CO_HB_CONSUMER_CNT; cnt++)  {
		pHbCons = getHbConsumer(cnt);

		/* node found ? */
		if (pHbCons->node == nodeId)  {
			/* heartbeat active ? */
			if (coTimerIsActive(&pHbCons->timer)
					== CO_TRUE)  {
				return(pHbCons->nmtState);
			}
		}
	}

# ifdef CO_GUARDING_CNT
	nmtState = icoGuardGetRemoteNodeState(nodeId);
# else /* CO_GUARDING_CNT */
	nmtState = CO_NMT_STATE_UNKNOWN;
# endif /* CO_GUARDING_CNT */

	return(nmtState);
}
#endif /* CO_HB_CONSUMER_CNT */


/***************************************************************************/
/**
* \internal
*
* \brief icoErrorBehavior - execute error behavoir
*
* Function is called, if communication event is occured.
* Depending on error behavoir, go to preop, stopped or standby at actual state
*
*
*/
void icoErrorBehavior(
		void	/* no parameter */
	)
{
	if (coNmtGetState() == CO_NMT_STATE_OPERATIONAL)  {
		if (errorBehavior == 0u)  {
			icoNmtStateChange(CO_NMT_STATE_PREOP, CO_FALSE);
		} else {
			if (errorBehavior == 2u)  {
				icoNmtStateChange(CO_NMT_STATE_STOPPED, CO_FALSE);
			}
		}
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
* \brief icoErrCtrlGetObjectAddr - get address of heartbeat entry
*
*
* \return none
*
*/
void *icoErrCtrlGetObjectAddr(
		UNSIGNED16	index,				/* index */
		UNSIGNED8	subIndex			/* subindex */
	)
{
void	*pAddr = NULL;
#ifdef CO_HB_CONSUMER_CNT
#else /* CO_HB_CONSUMER_CNT */
(void)subIndex;
#endif /* CO_HB_CONSUMER_CNT */

	switch (index)  {
#ifdef CO_GUARDING_SLAVE
		case 0x100cu:
			pAddr = (void *)&guardTime;
			break;
		case 0x100du:
			pAddr = (void *)&lifeTimeFactor;
			break;
#endif /* CO_GUARDING_SLAVE */

#ifdef CO_HB_CONSUMER_CNT
		case 0x1016u:
			/* hb consumer */
			pAddr = (void *)&getHbConsumer(subIndex - 1u)->odVal;
			break;
#endif /* CO_HB_CONSUMER_CNT */

		/* hb producer */
		case 0x1017u:
			pAddr = (void *)&hbProdTime;
			break;

		case 0x1029u:
			pAddr = (void *)&errorBehavior;
			break;

		default:
			pAddr = NULL;
			break;
	}

	return(pAddr);
}


#ifdef CO_HB_CONSUMER_CNT
/***************************************************************************/
/**
* \internal
*
* \brief icoErrCtrlCheckObjLimits - check limits
*
*
* \return RET_T
*
*/
RET_T icoErrCtrlCheckObjLimit(
		UNSIGNED8	subIndex,			/* subindex */
		UNSIGNED32	newValue			/* newvalue */
	)
{
UNSIGNED8	node;
UNSIGNED16	newTime;
UNSIGNED8	i;
CO_HB_CONSUMER_T *pHbConsumer;

	node = (UNSIGNED8)((newValue >> 16u) & 0x7fu);
	newTime = (UNSIGNED16)(newValue & 0xffffu);

	/* reset value is already allowed */
	if ((node != 0u) && (newTime != 0u))  {
		/* own node id is not allowed */
		if (node == coNmtGetNodeId())  {
			return(RET_PARAMETER_INCOMPATIBLE);
		}

		/* check, the same node is not in use */
		for (i = 0u; i < CO_HB_CONSUMER_CNT; i++)  {
			pHbConsumer = getHbConsumer(i);

			/* ignore own subindex */
			if (i != ((UNSIGNED16)subIndex - 1u))  {
				/* same node and time != ? */
				if ((pHbConsumer->node == node)
				 && (pHbConsumer->hbTime != 0u)) {
					return(RET_PARAMETER_INCOMPATIBLE);
				}
			}
		}
	}

	return(RET_OK);
}
#endif /* CO_HB_CONSUMER_CNT */


/***************************************************************************/
/**
* \internal
*
* \brief icoErrCtrlObjChanged - object was changed
*
*
* \return RET_T
*
*/
RET_T icoErrCtrlObjChanged(
		UNSIGNED16	index,			/* index of changed value */
		UNSIGNED8	subIndex		/* subindex of changed value */
	)
{
RET_T	retVal = RET_OK;
#ifdef CO_HB_CONSUMER_CNT
#else  /* CO_HB_CONSUMER_CNT */
(void)subIndex;
#endif /* CO_HB_CONSUMER_CNT */

	switch (index)  {
#ifdef CO_GUARDING_SLAVE
		case 0x100cu:
			/* if heartbeat is disabled and guarding is active,
			 * start guarding timer with new value */
			if (hbProdTime == 0u)  {
				if (coTimerIsActive(&hbProdTimer) == CO_TRUE)  {
					(void)coTimerStart(&hbProdTimer,
						(UNSIGNED32)guardTime * 1000ul,
						guardFail, NULL, CO_TIMER_ATTR_ROUNDUP); /*lint !e960 */
					/* Derogation MisraC2004 R.16.9 function identifier used
					 * without '&' or parenthesized parameter */
				}
			}
			break;
		case 0x100du:
			break;
#endif /* CO_GUARDING_SLAVE */
#ifdef CO_HB_CONSUMER_CNT
		case 0x1016u:
			retVal = setupHbConsumer(
						getHbConsumer(subIndex - 1u));
			break;
#endif /* CO_HB_CONSUMER_CNT */

		case 0x1017u:
			retVal = icoHbProdStart(CO_TRUE);
			break;

		default:
			break;
	}

	return(retVal);
}


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
#ifdef CO_EVENT_ERRCTRL_CNT
/***************************************************************************/
/**
* \brief coEventRegister_ERRCTRL - register error control event
*
* \return RET_T
*
*/

RET_T coEventRegister_ERRCTRL(
		CO_EVENT_ERRCTRL_T	pFunction			/**< pointer to function */
    )
{
	if (errCtrlTableCnt >= CO_EVENT_ERRCTRL_CNT) {
		return(RET_EVENT_NO_RESSOURCE);
	}

	errCtrlTable[errCtrlTableCnt] = pFunction;	/* save function pointer */
	errCtrlTableCnt++;

	return(RET_OK);
}
#endif /* CO_EVENT_ERRCTRL_CNT */


/***************************************************************************/
/**
* \internal
*
* \brief icoErrCtrlInd - call error control user indication
*
*
*/
void icoErrCtrlInd(
		UNSIGNED8		node,			/* node */
		CO_ERRCTRL_T	state,			/* error control event */
		CO_NMT_STATE_T	nmtState		/* nmt state */
	)
{
#ifdef CO_EVENT_ERRCTRL_CNT
UNSIGNED8	cnt;

	cnt = errCtrlTableCnt;
	while (cnt > 0u)  {
		cnt--;

		/* check user indication */
		errCtrlTable[cnt](node, state, nmtState);
	}
#endif /* CO_EVENT_ERRCTRL_CNT */

#ifdef CO_EVENT_STATIC_ERRCTRL
	coEventErrCtrlInd(node, state, nmtState);
#endif /* CO_EVENT_STATIC_ERRCTRL */
}


#ifdef CO_GUARDING_SLAVE
/***************************************************************************/
/**
* \internal
*
* \brief guardFail - master guarding failure
*
*
* \return none
*
*/
static void guardFail(
		void			*ptr
	)
{
(void)ptr;

	if ((guardTime == 0u) || (lifeTimeFactor == 0u))  {
		return;
	}

	guardFailCnt++; 
	if (guardFailCnt >= lifeTimeFactor)  {
		icoErrCtrlInd(0u, CO_ERRCTRL_GUARD_FAILED, CO_NMT_STATE_UNKNOWN);

#ifdef CO_EMCY_PRODUCER
		{
		UNSIGNED8 strg[5] = { 0u, 0u, 0u, 0u, 0u };

		(void)icoEmcyWriteReq(CO_EMCY_ERRCODE_COMM_ERROR, strg);
		}
#endif /* CO_EMCY_PRODUCER */
		/* change to preop */
		icoErrorBehavior();
	} else {
		(void)coTimerStart(&hbProdTimer,
				(UNSIGNED32)guardTime * 1000ul,
				guardFail, NULL, CO_TIMER_ATTR_ROUNDUP); /*lint !e960 */
		/* Derogation MisraC2004 R.16.9 function identifier used without '&'
		 * or parenthesized parameter */
	}
}
#endif /* CO_GUARDING_SLAVE */


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/**
* \internal
*
* \brief coErrCtrlReset - reset comm for errctrl
*
*
* \return none
*
*/
void icoErrorCtrlReset(
		void	/* no parameter */
	)
{
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */
UNSIGNED8	nodeId;
RET_T		retVal;
#ifdef CO_HB_CONSUMER_CNT
UNSIGNED8	i;
#endif /* CO_HB_CONSUMER_CNT */

	nodeId = coNmtGetNodeId();
	(void)icoCobSet(errCtrlCob, 0x700ul + nodeId, CO_COB_RTR, 1u);

	/* send bootup message */
	trData[0u] = 0u;
	retVal = icoTransmitMessage(errCtrlCob, &trData[0], MSG_INDICATION);
	if (retVal != RET_OK)  {
		/* save at transmit queue failed - inform application */
		icoErrCtrlInd(nodeId, CO_ERRCTRL_BOOTUP_FAILURE,
			CO_NMT_STATE_PREOP);
	}

#ifdef CO_HB_CONSUMER_CNT
	/* reset heartbeat consumer */
	for (i = 0u; i < CO_HB_CONSUMER_CNT; i++)  {
		(void)setupHbConsumer(getHbConsumer(i));
	}
#endif /* CO_HB_CONSUMER_CNT */
}


/***************************************************************************/
/**
* \internal
*
* \brief coErrCtrlSetDefaultValue
*
*
* \return none
*
*/
void icoErrorCtrlSetDefaultValue(
		void	/* no parameter */
	)
{
RET_T	retVal;
UNSIGNED16	u16;
#ifdef CO_HB_CONSUMER_CNT
UNSIGNED8	i;
UNSIGNED32	u32;
CO_HB_CONSUMER_T *pHbCons;
#endif /* CO_HB_CONSUMER_CNT */
#ifdef CO_GUARDING_SLAVE
UNSIGNED8	u8;
#endif /* CO_GUARDING_SLAVE */

	/* reset heartbeat producer */
	retVal = coOdGetDefaultVal_u16(0x1017u, 0u, &u16);
	if (retVal == RET_OK)  {
		hbProdTime = u16;
	}

#ifdef CO_GUARDING_SLAVE
	toggleBit = 0u;
	retVal = coOdGetDefaultVal_u16(0x100cu, 0u, &u16);
	if (retVal == RET_OK)  {
		guardTime = u16;
	}
	retVal = coOdGetDefaultVal_u8(0x100du, 0u, &u8);
	if (retVal == RET_OK)  {
		lifeTimeFactor = u8;
	}
#endif /* CO_GUARDING_SLAVE */

#ifdef CO_HB_CONSUMER_CNT
	/* reset heartbeat consumer */
	for (i = 0u; i < CO_HB_CONSUMER_CNT; i++)  {
		pHbCons = getHbConsumer(i);

		/* if hb consumer initialized */
#ifdef CO_ONE_HB_CONSUMER_COB
		if (hbConsCobRef != 0xffffu)  {
#else /* CO_ONE_HB_CONSUMER_COB */
		if (pHbCons->cob != 0xffffu)  {
#endif /* CO_ONE_HB_CONSUMER_COB */
			retVal = coOdGetDefaultVal_u32(0x1016u,
					(UNSIGNED8)i + 1u, &u32);
			if (retVal == RET_OK)  {
				pHbCons->odVal = u32;
			}
		}
	}
#endif /* CO_HB_CONSUMER_CNT */

	(void)coOdGetDefaultVal_u8(0x1029u, 1u, &errorBehavior);
}


#ifdef CO_HB_CONSUMER_CNT
/***************************************************************************/
/**
* \internal
*
* \brief setupHbConsumer - setup a heartbeat consumer
*
*
* \return none
*
*/
static RET_T setupHbConsumer(
		CO_HB_CONSUMER_T	*pHbCons
	)
{
RET_T	retVal;


#ifdef CO_ONE_HB_CONSUMER_COB
	if (hbConsCobRef == 0xffffu)  {
		return(RET_NOT_INITIALIZED);
	}
#else /* CO_ONE_HB_CONSUMER_COB */	
	/* ignore not configured entries */
	if (pHbCons->cob == 0xffffu)  {
		return(RET_NOT_INITIALIZED);
	}
#endif /* CO_ONE_HB_CONSUMER_COB */

	/* stop timer alltimes, so the monitoring starts again */
	(void)coTimerStop(&pHbCons->timer);

	pHbCons->node = (UNSIGNED8)((pHbCons->odVal >> 16u) & 0x7fu);
	pHbCons->hbTime = (UNSIGNED16)(pHbCons->odVal & 0xffffu);

	if ((pHbCons->node == 0u)
	 || (pHbCons->hbTime == 0u))  {
		/* disable HB consumer */
#ifdef CO_ONE_HB_CONSUMER_COB
		pHbCons->node = 0u;
#else /* CO_ONE_HB_CONSUMER_COB */
		retVal = icoCobSet(pHbCons->cob, 0x80000000ul,
			CO_COB_RTR_NONE, 1u);
		return(retVal);
#endif /* CO_ONE_HB_CONSUMER_COB */
	}

	/* setup monitoring */
#ifdef CO_ONE_HB_CONSUMER_COB
	(void)icoCobSetIgnore(hbConsCobRef, 0x7ful);
	retVal = icoCobSet(hbConsCobRef,
		0x700ul + (UNSIGNED32)pHbCons->node, CO_COB_RTR_NONE, 1u);
#else /* CO_ONE_HB_CONSUMER_COB */
	retVal = icoCobSet(pHbCons->cob,
		0x700ul + (UNSIGNED32)pHbCons->node, CO_COB_RTR_NONE, 1u);
#endif /* CO_ONE_HB_CONSUMER_COB */
	pHbCons->nmtState = CO_NMT_STATE_UNKNOWN;

	return(retVal);
}


/***************************************************************************/
/*
* \internal
* 
* \brief getHbConsumer - get HB consumer data
*
*/
static CO_INLINE CO_HB_CONSUMER_T *getHbConsumer(
		UNSIGNED8		idx
	)
{
	return(&hbConsumer[idx]);
}
#endif /* CO_HB_CONSUMER_CNT */


/***************************************************************************/
/*
* \brief icoErrCtrlVarInit - init error control variables
*
*/
void icoErrorCtrlVarInit(
		UNSIGNED8	*pList		/* line counts */
	)
{
#ifdef CO_HB_CONSUMER_CNT
# ifdef CO_ONE_HB_CONSUMER_COB
# else /* CO_ONE_HB_CONSUMER_COB */
UNSIGNED16		i;
# endif /* CO_ONE_HB_CONSUMER_COB */
#endif /* CO_HB_CONSUMER_CNT */
(void)pList;

	{

		errCtrlCob = 0xffffu;
		errorBehavior = 0u;
#ifdef CO_ONE_HB_CONSUMER_COB
		hbConsCobRef = 0xffffu;
#endif /* CO_ONE_HB_CONSUMER_COB */
	}

#ifdef CO_HB_CONSUMER_CNT
# ifdef CO_ONE_HB_CONSUMER_COB
# else /* CO_ONE_HB_CONSUMER_COB */
	for (i = 0u; i < CO_HB_CONSUMER_CNT; i++)  {
		hbConsumer[i].cob = 0xffffu;
	}
# endif /* CO_ONE_HB_CONSUMER_COB */
#endif /* CO_HB_CONSUMER_CNT */

#ifdef CO_EVENT_ERRCTRL_CNT
	errCtrlTableCnt = 0u;
#endif /* CO_EVENT_ERRCTRL_CNT */
}


/***************************************************************************/
/**
* \brief coInitNmt - init error control
*
* Setup error control handling for local node (transmit heartbeat)
* and remote node (heartbeat monitoring)
*
* \return RET_T
*
*/
RET_T coErrorCtrlInit(
		UNSIGNED16	hbTime,				/**< heartbeat producer time */
		UNSIGNED8	hbConsCnt			/**< heartbeat consumer count */
	)
{
#ifdef CO_HB_CONSUMER_CNT
# ifdef CO_ONE_HB_CONSUMER_COB
# else /* CO_ONE_HB_CONSUMER_COB */
UNSIGNED8	i;
CO_HB_CONSUMER_T *pHbCons;
# endif /* CO_ONE_HB_CONSUMER_COB */
#else /* CO_HB_CONSUMER_CNT */
(void)hbConsCnt;
#endif /* CO_HB_CONSUMER_CNT */

	errCtrlCob = icoCobCreate(CO_COB_TYPE_TRANSMIT, CO_SERVICE_ERRCTRL, 0u);
	if (errCtrlCob == 0xffffu) {
		return(RET_NO_COB_AVAILABLE);
	}
	hbProdTime = hbTime;
	errorBehavior = 0u;

#ifdef CO_GUARDING_SLAVE
	toggleBit = 0u;
#endif /* CO_GUARDING_SLAVE */

#ifdef CO_HB_CONSUMER_CNT
# ifdef CO_ONE_HB_CONSUMER_COB
	if (hbConsCnt != 0u)  {
		hbConsCobRef = icoCobCreate(CO_COB_TYPE_RECEIVE,
			CO_SERVICE_ERRCTRL, 0u);
		if (hbConsCobRef == 0xffffu) {
			return(RET_NO_COB_AVAILABLE);
		}
	}
# else /* CO_ONE_HB_CONSUMER_COB */
	for (i = 0u; i < hbConsCnt; i++)  {
		pHbCons = getHbConsumer(i);
		pHbCons->cob = icoCobCreate(CO_COB_TYPE_RECEIVE,
				CO_SERVICE_ERRCTRL, (UNSIGNED16)i);
		if (pHbCons->cob == 0xffffu)  {
			return(RET_NO_COB_AVAILABLE);
		}
	}
# endif /* CO_ONE_HB_CONSUMER_COB */
#endif /* CO_HB_CONSUMER_CNT */

	(void)coOdGetDefaultVal_u8(0x1029u, 1u, &errorBehavior);

	return(RET_OK);
}
