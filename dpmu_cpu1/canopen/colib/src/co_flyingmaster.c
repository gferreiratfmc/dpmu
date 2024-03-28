/*
* co_nmtflyingmaster.c - contains flying master services
*
* Copyright (c) 2012-2022 emotas embedded communication GmbH
*-------------------------------------------------------------------
* $Id: co_flyingmaster.c 39658 2022-02-17 10:15:29Z boe $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \brief flying master handling
*
* \file co_flyingmaster.c
* contains flying master services
*
* \internal
* 					state		cobs		ftc
* 					------------------------------------------
* initNMT			POWERON
*
* icoNmtFlymaInit				create
* 
* resetComm									check 1f80
* 	not flyma		(SLAVE)
*								set cobids
* 	POWERON/DETECT							negotiation timer
*
*
* negotiation timer							detect active master
*
*
* detect active master
*	IF no active master				
* 		IF POWERON	DETECT					reset comm
*		IF DETECT							flymaNego
*	ELSE active master				
*											checkPriority
*
* flymaNego						change slave
* 											send flyma nego
* 											startNegoTimer
*
*
* startNegoTimer
* 					MASTER		change mstr	sendActiveMaster
*
*
* checkPriority
*	> own			SLAVE
* 	< own									force Negotiation
* 
* 
* 
*/

/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <stddef.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <gen_define.h>

#ifdef CO_FLYING_MASTER_SUPPORTED
#include <co_datatype.h>
#include <co_odaccess.h>
#include <co_timer.h>
#include <co_drv.h>
#include <co_nmt.h>
#include <co_flyingmaster.h>
#include <ico_cobhandler.h>
#include <ico_queue.h>
#include <ico_nmt.h>

#ifdef CO_SLEEP_454
#include <ico_sleep.h> 
#endif /* CO_SLEEP_454 */

/* constant definitions
---------------------------------------------------------------------------*/
#define CO_FLYMA_COB_ACT_MSTR_ANSWER	0x71u
#define CO_FLYMA_COB_NEGOTIATION_REQ	0x72u
#define CO_FLYMA_COB_ACT_MSTR_REQ		0x73u
#define CO_FLYMA_COB_DETECT_MASTER_REQ	0x75u
#define CO_FLYMA_COB_DETECT_MASTER_ANSWER 0x74u
#define CO_FLYMA_COB_FORCE_NEGOTIATION	0x76u

#ifdef CO_EVENT_DYNAMIC_FLYMA
# define CO_EVENT_FLYMA_CNT     (CO_EVENT_DYNAMIC_FLYMA)
#endif /* CO_EVENT_DYNAMIC_FLYMA */


/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/
#ifdef CO_EVENT_STATIC_FLYMA
extern CO_CONST CO_EVENT_FLYMA_T coEventFlymaInd;
#endif /* CO_EVENT_STATIC_FLYMA */

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
static void negotiationFinished(void *ptr);
static void noActiveMaster(void *ptr);
static void noMasterAvail(void *ptr);
static void flymaStartNegotiation(UNSIGNED8 remote);
static void flymaMaster(void *ptr);
static void flymaSlave(UNSIGNED8 node, UNSIGNED8 prior);
static void sendActiveMaster(void);
static void checkActiveMasterData(UNSIGNED8 priority,
				UNSIGNED8	node);
static void flymaSetCobTypes(UNSIGNED8	master);
static void flymaInd(CO_FLYMA_STATE_T state, UNSIGNED8 node,
				UNSIGNED8 prior);
static void multipleMasterCheck(void *ptr);

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
static UNSIGNED16	negotiationTime = { 500u };
static UNSIGNED16	activeMasterTime = { 100u };
static UNSIGNED16	priorityTimeSlot = { 1500u };
static UNSIGNED16	deviceTimeSlot = { 10u };
static UNSIGNED16	priorityLevel = { 1u };
static UNSIGNED16	detectMasterTime = { 100u };
static UNSIGNED16	multipleMasterTime = { 4000 };
static UNSIGNED8	activeMasterNodeId = { 0 };

static CO_TIMER_T	flymaTimer;
static CO_TIMER_T	detectMasterTimer;
static COB_REFERENZ_T	negotiationReqCob_rx;
static COB_REFERENZ_T	negotiationReqCob_tx;
static COB_REFERENZ_T	activeMstrReqCob;
static COB_REFERENZ_T	activeMstrAnswerCob;
static COB_REFERENZ_T	forceNegotiationCob_rx;
static COB_REFERENZ_T	forceNegotiationCob_tx;
static COB_REFERENZ_T	detectMasterReqCob_rx;
static COB_REFERENZ_T	detectMasterReqCob_tx;
static COB_REFERENZ_T	detectMasterAnswerCob_rx;
static COB_REFERENZ_T	detectMasterAnswerCob_tx;

#ifdef CO_EVENT_FLYMA_CNT
static CO_EVENT_FLYMA_T	flymaEventTable[CO_EVENT_FLYMA_CNT];
static UNSIGNED8		flymaEventTableCnt = 0u;
#endif /* CO_EVENT_FLYMA_CNT */



/***************************************************************************/
/**
* \internal
*
* \brief
*
*
* \return none
*
*/
void icoNmtFlymaHandler(
		CO_CONST CO_REC_DATA_T *pRecData	/* pointer to receive data */
	)
{
UNSIGNED32	nmtStartup;
UNSIGNED8 ownNodeId;

	/* switch depending on cob-id - start with NMT command */
	switch (pRecData->msg.canId)  {
		case CO_FLYMA_COB_ACT_MSTR_ANSWER:
			if (pRecData->msg.len != 2u)  {
				return;
			}
			checkActiveMasterData(pRecData->msg.data[0],
					pRecData->msg.data[1]);
			break;

		case CO_FLYMA_COB_NEGOTIATION_REQ:
			flymaStartNegotiation(1u);
			break;
		case CO_FLYMA_COB_FORCE_NEGOTIATION:
			ownNodeId = coNmtGetNodeId();
			if (activeMasterNodeId == ownNodeId)  {
				flymaStartNegotiation(0u);
			}
			break;

		case CO_FLYMA_COB_ACT_MSTR_REQ:
			sendActiveMaster();
			break;

		case CO_FLYMA_COB_DETECT_MASTER_REQ:
			/* master features allowed ? */
			nmtStartup = icoNmtMasterGetStartupObj();
			if ((nmtStartup & CO_NMT_STARTUP_BIT_MASTER) != 0u)  {
				icoTransmitMessage(detectMasterAnswerCob_tx,
					NULL, 0u);
			}
			break;

		case CO_FLYMA_COB_DETECT_MASTER_ANSWER:
			flymaInd(CO_FLYMA_STATE_MASTERS_AVAILABLE, 0u, 0u);
			break;
	}
}


/***************************************************************************/
/**
*
* \brief coFlymaDetectMaster - detect master capable devices
*
* This function starts the detection of nodes with master capabilities.
* It sends the master detection protocol on the bus.
* After an answer was received or the timeout was occured,
* the registered indication function is called.
* (Timing values are located at index 0x1f91)
*
* \return RET_T
*
*/
RET_T coNmtFlymaDetectMaster(
		void	/* no parameter */
	)
{
RET_T	retVal;

	retVal = icoTransmitMessage(detectMasterReqCob_tx,
		NULL, 0u);

	/* start timer */
	coTimerStart(&detectMasterTimer,
		detectMasterTime * 1000ul,
		noMasterAvail, NULL, CO_TIMER_ATTR_ROUNDDOWN); /*lint !e960 */
	/* Derogation MisraC2004 R.16.9 function identifier used without '&'
	 * or parenthesized parameter */

	return(retVal);
}


/***************************************************************************/
/**
*
* \brief coNmtFlymaDetectActiveMaster - detect active master
*
* This function starts the detection of the active system master.
* It sends the actice master detection protocol on the bus.
* The active master should answer the request within the timeout time.
* After an answer was received or the timeout was occured,
* the registered indication function is called.
*
* \return RET_T
*
*/
RET_T coNmtFlymaDetectActiveMaster(
		void	/* no parameter */
	)
{
RET_T		retVal;

	retVal = icoTransmitMessage(activeMstrReqCob, NULL, 0u);

	/* start timer */
	coTimerStart(&flymaTimer,
		activeMasterTime * 1000ul,
		noActiveMaster, NULL, CO_TIMER_ATTR_ROUNDDOWN); /*lint !e960 */
	/* Derogation MisraC2004 R.16.9 function identifier used without '&'
	 * or parenthesized parameter */

	return(retVal);
}


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/**
* \internal
*
* \brief icoFlymaGetObjectAddr - get flyma object address
*
* \return none
*
*/
void *icoFlymaGetObjectAddr(
		UNSIGNED16	index,				/* index */
		UNSIGNED8	subIndex			/* index */
	)
{
void	*pAddr = NULL;

	if (index == 0x1f90u)  {
		switch (subIndex)  {
			case 1u:
				pAddr = (void *)&activeMasterTime;
				break;
			case 2u:
				pAddr = (void *)&negotiationTime;
				break;
			case 3u:
				pAddr = (void *)&priorityLevel;
				break;
			case 4u:
				pAddr = (void *)&priorityTimeSlot;
				break;
			case 5u:
				pAddr = (void *)&deviceTimeSlot;
				break;
			case 6u:
				pAddr = (void *)&multipleMasterTime;
				break;
			default:
				break;
		}
	}

	if (index == 0x1f91u)  {
		switch (subIndex)  {
			case 1u:
				pAddr = (void *)&detectMasterTime;
				break;
			default:
				break;
		}
	}

	return(pAddr);
}


#ifdef xxx
/***************************************************************************/
/**
* \internal
*
* \brief icoFlymaCheckObjLimit_u32 - check UNSIGNED32 object limits
*
*
* \return RET_T
*
*/
RET_T	icoFlymaCheckObjLimit_u16(
		UNSIGNED16	index,			/* index */
		UNSIGNED32	value			/* new value */
	)
{
CO_CONST CO_OBJECT_DESC_T *pDescPtr;

	return(RET_OK);
}


/***************************************************************************/
/**
* \internal
*
* \brief icoFlymaObjChanged - flyma object changed
*
*
* \return RET_T
*
*/
RET_T icoFlymaObjChanged(
		UNSIGNED16	valIdx				/* index */
	)
{
RET_T	retVal = RET_OK;

	switch (valIdx)  {
	return(retVal);
}
#endif /* xxx */


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
# ifdef CO_EVENT_FLYMA_CNT
/***************************************************************************/
/**
* \brief coEventRegister_FLYMA - register FLYMA event
*
* This function registers an indication function for FLYMA events.
*
*
* \return RET_T
*
*/

RET_T coEventRegister_FLYMA(
		CO_EVENT_FLYMA_T pFunction	/**< pointer to function */
    )
{
	if (flymaEventTableCnt >= CO_EVENT_FLYMA_CNT) {
		return(RET_EVENT_NO_RESSOURCE);
	}

	/* set new indication function as first at the list */
	flymaEventTable[flymaEventTableCnt] = pFunction;
	flymaEventTableCnt++;

	return(RET_OK);
}
# endif /* CO_EVENT_FLYMA_CNT */


/***************************************************************************/
/**
*
* \internal
*
* \brief flymaInd - call flyma indication
*
* \return
*	RET_T
*/
static void flymaInd(
		CO_FLYMA_STATE_T state,
		UNSIGNED8		node,
		UNSIGNED8		prior
	)
{
# ifdef CO_EVENT_FLYMA_CNT
UNSIGNED16	cnt;

	/* call indication to execute */
	cnt = flymaEventTableCnt;
	while (cnt > 0u)  {
		cnt--;
		/* call user indication */
		flymaEventTable[cnt](state, node, prior);
	}
# else /* CO_EVENT_FLYMA_CNT */
	(void)prior;
	(void)node;
# endif /* CO_EVENT_FLYMA_CNT */

# ifdef CO_EVENT_STATIC_FLYMA
	coEventFlymaInd(state);
# endif /* CO_EVENT_STATIC_FLYMA */

	return;
}


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/


/***************************************************************************/
/**
*
* \brief coFlymaStartNegotiation - start master negotiation
*
* This function initiate the master negotiation by sending
* the master negotiation request. After that it has to wait
* until it receive an answer from an other master
* or the own waiting time is over.
* If the received answer was from a master with a higher priority,
* the node switches to works as slave.
* If the own waiting time was over, it has to send the master message
* and can work as the system master.
* In each case the registered indication function is called.
*
* \return RET_T
*
*/
static void flymaStartNegotiation(
		UNSIGNED8	remote				/* remote triggered */
	)
{
RET_T	retVal;
UNSIGNED32	waitingTime;

	/* change cobs to slave */
	flymaSetCobTypes(0u);

	if (remote == 0u)  {
		/* transmit master negotiation message */
		retVal = icoTransmitMessage(negotiationReqCob_tx,
			NULL, 0);
		if (retVal != RET_OK)  {
			return;
		}
	}

	/* start waiting time */
	waitingTime = priorityLevel * priorityTimeSlot;
	waitingTime += (coNmtGetNodeId() * deviceTimeSlot);
	(void)coTimerStart(&flymaTimer, waitingTime * 1000ul,
		flymaMaster, NULL, CO_TIMER_ATTR_ROUNDDOWN); /*lint !e960 */
	/* Derogation MisraC2004 R.16.9 function identifier used without '&'
	 * or parenthesized parameter */

	/* call indication */
	flymaInd(CO_FLYMA_STATE_NEGOTIATION_STARTED, 0u, 0u);
}


/***************************************************************************/
/**
* \internal
*
* \brief
*
* Answer from master received
* Check, if priority is lower than own prioroty
* if not, force master negotiation
*
*/
static void checkActiveMasterData(
		UNSIGNED8	priority,
		UNSIGNED8	node
	)
{
UNSIGNED8	slave = 0u;

	/* stop timer */
	coTimerStop(&flymaTimer);

	if (priority < priorityLevel)  {
		/* greater than own, work as slave */
		slave = 1u;
	} else {
		if (priority == priorityLevel)  {
			/* priority equal - check node id */
			if (node < coNmtGetNodeId())  {
				/* work as slave */
				slave = 1u;
			} else {
				/* force negotiation */
				slave = 0u;
			}
		} else {
			/* priority less - force negotiation */
			slave = 0u;
		}
	}

	if (slave == 0u)  {
		/* force negotiation */
		icoTransmitMessage(forceNegotiationCob_tx, NULL, 0u);
		icoNmtMasterSetState(CO_NMT_MASTER_STATE_DETECT);
		flymaStartNegotiation(0u);
	} else {
		/* work as slave */
		flymaSlave(node, priority);
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief negotiationFinished - negotiation time is over
*
* Now start active master negotiation
*
* \return none
*
*/
static void negotiationFinished(
		void			*ptr
	)
{
(void)ptr;

	coNmtFlymaDetectActiveMaster();
}


/***************************************************************************/
/**
* \internal
*
* \brief noActiveMaster - no master detected
*
* No active master was detected - start master negotiation
*
* \return none
*
*/
static void noActiveMaster(
		void			*ptr
	)
{
CO_NMT_MASTER_STATE_T mState;
(void)ptr;

	mState = icoNmtMasterGetState();

	/* from power on ? */
	if (mState == CO_NMT_MASTER_STATE_POWERON)  {
		icoNmtMasterSetState(CO_NMT_MASTER_STATE_DETECT);
		/* send reset comm */
		(void)coNmtStateReq(0u, CO_NMT_STATE_RESET_COMM, CO_TRUE);
	} else {
		/* start flying master process for each other state */
		flymaStartNegotiation(0u);

		/* call indication */
		flymaInd(CO_FLYMA_STATE_NO_ACTIVE_MASTER, 0u, 0u);
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief noMasterAvail - no master detected
*
* No master was detected, inform application
*
* \return none
*
*/
static void noMasterAvail(
		void			*ptr
	)
{
(void)ptr;

	/* call indication */
	flymaInd(CO_FLYMA_STATE_DETECT_NO_MASTERS, 0u, 0u);
}


/***************************************************************************/
/**
* \internal
*
* \brief flymaMaster - master time is up - we are the master
*
*
* \return none
*
*/
static void flymaMaster(
		void			*ptr
	)
{
UNSIGNED8	ownNodeId;
(void)ptr;

	icoNmtMasterSetState(CO_NMT_MASTER_STATE_MASTER);

#ifdef CO_SLEEP_454
	icoSleepSetMasterSlave(CO_NMT_MASTER_STATE_MASTER);
#endif /* CO_SLEEP_454 */

	/* change cob types to master */
	flymaSetCobTypes(1u);

	/* send out active master */
	sendActiveMaster();

	/* start cyclic transmission of force flying master negotiation */
	ownNodeId = coNmtGetNodeId();
	(void)coTimerStart(&flymaTimer,
		multipleMasterTime * 1000ul,
		multipleMasterCheck, NULL, CO_TIMER_ATTR_ROUNDUP_CYCLIC); /*lint !e960*/
	/* Derogation MisraC2004 R.16.9 function identifier used without '&'
	 * or parenthesized parameter */

	/* save active master node id */
	activeMasterNodeId = ownNodeId;

	/* call indication */
	flymaInd(CO_FLYMA_STATE_MASTER, ownNodeId, (UNSIGNED8)priorityLevel);
}


/***************************************************************************/
/**
* \internal
*
* \brief
*
*
* \return none
*
*/
static void multipleMasterCheck(
		void			*ptr
	)
{
(void)ptr;

	(void)icoTransmitMessage(forceNegotiationCob_tx, NULL, 0u);
}


/***************************************************************************/
/**
* \internal
*
* \brief flymaSlave - work as slave
*
*
* \return none
*
*/
static void flymaSlave(
		UNSIGNED8	node,
		UNSIGNED8	prior
	)
{
CO_NMT_MASTER_STATE_T	masterState;

	masterState = icoNmtMasterGetState();
	if (masterState != CO_NMT_MASTER_STATE_SLAVE)  {
		icoNmtMasterSetState(CO_NMT_MASTER_STATE_SLAVE);
	}
#ifdef CO_SLEEP_454
		icoSleepSetMasterSlave(CO_NMT_MASTER_STATE_SLAVE);
#endif /* CO_SLEEP_454 */
	/* save active master node id */
	activeMasterNodeId = node;

	/* call indication */
	flymaInd(CO_FLYMA_STATE_SLAVE, node, prior);
}


/***************************************************************************/
/**
* \internal
*
* \brief
*
*
* \return none
*
*/
static void sendActiveMaster(
		void	/* no parameter */
	)
{
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */

	/* only active master is allowed to send */
	if (icoNmtMasterGetState() != CO_NMT_MASTER_STATE_MASTER)  {
		return;
	}
	trData[0] = priorityLevel & 0xffu;
	trData[1] = coNmtGetNodeId();
	(void)icoTransmitMessage(activeMstrAnswerCob, &trData[0], 0u);
}


/***************************************************************************/
/**
* \internal
*
* \brief icoNmtFlymaErrCtrlFailure - error control failure
*
* Check, was the active master.
* In this case, set mState to unknown and start reset comm for all nodes
*
* \return none
*
*/
void icoNmtFlymaErrCtrlFailure(
		UNSIGNED8 node					/* failed node */
	)
{
	/* was active master ? */
	if (node != activeMasterNodeId)  {
		/* no, return */
		return;
	}

	/* set master state to unknown */
	icoNmtMasterSetState(CO_NMT_MASTER_STATE_DETECT);

	/* send reset comm for all nodes - starts a new master negotiation */
	(void)coNmtStateReq(0u, CO_NMT_STATE_RESET_COMM, CO_TRUE);
}


/***************************************************************************/
/**
* \internal
*
* \brief
*
*
* \return none
*
*/
static void flymaSetCobTypes(
		UNSIGNED8	master
	)
{
	if (master == 1u)  {
		(void)icoCobChangeType(activeMstrAnswerCob, CO_COB_TYPE_TRANSMIT);
		(void)icoCobChangeType(activeMstrReqCob, CO_COB_TYPE_RECEIVE);
	} else {
		(void)icoCobChangeType(activeMstrAnswerCob, CO_COB_TYPE_RECEIVE);
		(void)icoCobChangeType(activeMstrReqCob, CO_COB_TYPE_TRANSMIT);
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief icoFlymaReset
*
* \return none
*
*/
void icoNmtFlymaReset(
		void	/* no parameter */
	)
{
CO_NMT_MASTER_STATE_T mState;
UNSIGNED32	nmtStartup;

	/* check 1f80 */
	nmtStartup = icoNmtMasterGetStartupObj();
	if (((nmtStartup & CO_NMT_STARTUP_BIT_MASTER) == 0)
	 && ((nmtStartup & CO_NMT_STARTUP_BIT_FLYMA) == 0))  {
		/* don't work as any master - abort */

		/* set slave mode */
		icoNmtMasterSetState(CO_NMT_MASTER_STATE_SLAVE);
		return;
	}
	
	if ((nmtStartup & CO_NMT_STARTUP_BIT_FLYMA) == 0)  {
		/* don't work as flyma, but a normal master */
		icoNmtMasterSetState(CO_NMT_MASTER_STATE_MASTER);
		return;
	}

	/* setup cob-ids */
	icoCobSet(negotiationReqCob_rx, CO_FLYMA_COB_NEGOTIATION_REQ,
		CO_COB_RTR_NONE, 0u);
	icoCobSet(negotiationReqCob_tx, CO_FLYMA_COB_NEGOTIATION_REQ,
		CO_COB_RTR_NONE, 0u);
	icoCobSet(activeMstrReqCob , CO_FLYMA_COB_ACT_MSTR_REQ,
		CO_COB_RTR_NONE, 0u);
	icoCobSet(activeMstrAnswerCob , CO_FLYMA_COB_ACT_MSTR_ANSWER,
		CO_COB_RTR_NONE, 2u);
	icoCobSet(forceNegotiationCob_rx, CO_FLYMA_COB_FORCE_NEGOTIATION,
		CO_COB_RTR_NONE, 0u);
	icoCobSet(forceNegotiationCob_tx, CO_FLYMA_COB_FORCE_NEGOTIATION,
		CO_COB_RTR_NONE, 0u);
	icoCobSet(detectMasterReqCob_tx, CO_FLYMA_COB_DETECT_MASTER_REQ,
		CO_COB_RTR_NONE, 0u);
	icoCobSet(detectMasterReqCob_rx, CO_FLYMA_COB_DETECT_MASTER_REQ,
		CO_COB_RTR_NONE, 0u);
	icoCobSet(detectMasterAnswerCob_tx, CO_FLYMA_COB_DETECT_MASTER_ANSWER,
		CO_COB_RTR_NONE, 0u);
	icoCobSet(detectMasterAnswerCob_rx, CO_FLYMA_COB_DETECT_MASTER_ANSWER,
		CO_COB_RTR_NONE, 0u);


	mState = icoNmtMasterGetState();
	if ((mState != CO_NMT_MASTER_STATE_SLAVE)
	 &&	(mState != CO_NMT_MASTER_STATE_MASTER))  {
		/* start master negotiation time */
		coTimerStart(&flymaTimer,
			negotiationTime * 1000ul,
			negotiationFinished, NULL, CO_TIMER_ATTR_ROUNDDOWN); /*lint !e960 */
		/* Derogation MisraC2004 R.16.9 function identifier used without '&'
		 * or parenthesized parameter */
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief icoErrCtrlSetDefaultValue
*
*
* \return none
*
*/
void icoFlymaSetDefaultValue(
		void	/* no parameter */
	)
{
RET_T	retVal;
UNSIGNED16	u16;

	/* reset flying master */
	retVal = coOdGetDefaultVal_u16(0x1f90u, 1u, &u16);
	if (retVal == RET_OK)  {
		activeMasterTime = u16;
	}
	retVal = coOdGetDefaultVal_u16(0x1f90u, 2u, &u16);
	if (retVal == RET_OK)  {
		negotiationTime = u16;
	}
	retVal = coOdGetDefaultVal_u16(0x1f90u, 3u, &u16);
	if (retVal == RET_OK)  {
		priorityLevel = u16;
	}
	retVal = coOdGetDefaultVal_u16(0x1f90u, 4u, &u16);
	if (retVal == RET_OK)  {
		priorityTimeSlot = u16;
	}
	retVal = coOdGetDefaultVal_u16(0x1f90u, 5u, &u16);
	if (retVal == RET_OK)  {
		deviceTimeSlot = u16;
	}
	retVal = coOdGetDefaultVal_u16(0x1f90u, 6u, &u16);
	if (retVal == RET_OK)  {
		multipleMasterTime = u16;
	}

}


/***************************************************************************/
/**
* \internal
*
* \brief icoFlymaVarInit
*
* \return none
*
*/
void icoFlymaVarInit(
		void
	)
{

	{
		negotiationTime = 500u;
		activeMasterTime = 100u;
		priorityTimeSlot = 1500u;
		deviceTimeSlot = 10u;
		priorityLevel = 1u;
		detectMasterTime = 100u;
		multipleMasterTime = 4000u;
		activeMasterNodeId = 0u;

		negotiationReqCob_rx = 0xffffu;
		negotiationReqCob_tx = 0xffffu;
		activeMstrReqCob = 0xffffu;
		activeMstrAnswerCob = 0xffffu;
		forceNegotiationCob_rx = 0xffffu;
		forceNegotiationCob_tx = 0xffffu;
		detectMasterReqCob_rx = 0xffffu;
		detectMasterReqCob_tx = 0xffffu;
		detectMasterAnswerCob_rx = 0xffffu;
		detectMasterAnswerCob_tx = 0xffffu;
	}

#ifdef CO_EVENT_FLYMA_CNT
	flymaEventTableCnt = 0u;
#endif /* CO_EVENT_FLYMA_CNT */
}


/***************************************************************************/
/**
* \internal
*
* \brief coFlymaInit - init flyma functionality
*
* This function initializes the FLYMA functionality.
*
* If the node is a flyma producer or a flyma consumer
* depends on the value of the object dictionary index 0x1005.
* <br>Flyma counter value can also be set/reset by the value
* at the object dictionary at index 0x1019
*
* \return RET_T
*
*/
RET_T icoNmtFlymaInit(
		void	/* no parameter */
	)
{
	/* we start with slave mode */
	negotiationReqCob_rx = icoCobCreate(CO_COB_TYPE_RECEIVE,
			CO_SERVICE_FLYMA, 0u);
	if (negotiationReqCob_rx == 0xffffu)  {
		return(RET_NO_COB_AVAILABLE);
	}
	negotiationReqCob_tx = icoCobCreate(CO_COB_TYPE_TRANSMIT,
			CO_SERVICE_FLYMA, 0u);
	if (negotiationReqCob_tx == 0xffffu)  {
		return(RET_NO_COB_AVAILABLE);
	}
	activeMstrAnswerCob = icoCobCreate(CO_COB_TYPE_RECEIVE,
			CO_SERVICE_FLYMA, 0u);
	if (activeMstrAnswerCob == 0xffffu)  {
		return(RET_NO_COB_AVAILABLE);
	}
	activeMstrReqCob = icoCobCreate(CO_COB_TYPE_TRANSMIT,
			CO_SERVICE_FLYMA, 0u);
	if (activeMstrReqCob == 0xffffu)  {
		return(RET_NO_COB_AVAILABLE);
	}
	forceNegotiationCob_rx = icoCobCreate(CO_COB_TYPE_RECEIVE,
			CO_SERVICE_FLYMA, 0u);
	if (forceNegotiationCob_rx == 0xffffu)  {
		return(RET_NO_COB_AVAILABLE);
	}
	forceNegotiationCob_tx = icoCobCreate(CO_COB_TYPE_TRANSMIT,
			CO_SERVICE_FLYMA, 0u);
	if (forceNegotiationCob_tx == 0xffffu)  {
		return(RET_NO_COB_AVAILABLE);
	}
	detectMasterReqCob_rx = icoCobCreate(CO_COB_TYPE_RECEIVE,
			CO_SERVICE_FLYMA, 0u);
	if (detectMasterReqCob_rx == 0xffffu)  {
		return(RET_NO_COB_AVAILABLE);
	}
	detectMasterReqCob_tx = icoCobCreate(CO_COB_TYPE_TRANSMIT,
			CO_SERVICE_FLYMA, 0u);
	if (detectMasterReqCob_tx == 0xffffu)  {
		return(RET_NO_COB_AVAILABLE);
	}
	detectMasterAnswerCob_rx = icoCobCreate(CO_COB_TYPE_RECEIVE,
			CO_SERVICE_FLYMA, 0u);
	if (detectMasterAnswerCob_rx == 0xffffu)  {
		return(RET_NO_COB_AVAILABLE);
	}
	detectMasterAnswerCob_tx = icoCobCreate(CO_COB_TYPE_TRANSMIT,
			CO_SERVICE_FLYMA, 0u);
	if (detectMasterAnswerCob_tx == 0xffffu)  {
		return(RET_NO_COB_AVAILABLE);
	}

	return(RET_OK);
}
#endif /* CO_FLYING_MASTER_SUPPORTED */
