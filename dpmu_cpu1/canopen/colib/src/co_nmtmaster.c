/*
* co_nmtmaster.c - contains NMT master services
*
* Copyright (c) 2014-2021 emotas embedded communication GmbH
*-------------------------------------------------------------------
* $Id: co_nmtmaster.c 41218 2022-07-12 10:35:19Z boe $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \brief NMT master services
*
* \file co_nmtmaster.c
* contains NMT master services
*
*/

/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <string.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <gen_define.h>

#ifdef CO_NMT_MASTER

#include <co_datatype.h>
#include <co_drv.h>
#include <co_timer.h>
#include <co_odaccess.h>
#include <co_nmt.h>
#include "ico_cobhandler.h"
#include "ico_event.h"
#include "ico_queue.h"
#include "ico_nmt.h"

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
static void startNmtReq(void *ptr);
#ifdef CO_BOOTUP_MANAGER
static UNSIGNED32 *getNmtSlaveAssignment(UNSIGNED8 idx);
#endif /* CO_BOOTUP_MANAGER */

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
static UNSIGNED16	nmtInhibit = { 0u };
static UNSIGNED32	nmtStartup = { 0u };
static CO_NMT_MASTER_STATE_T	nmtMasterState;
static CO_NMT_STATE_T	requestNmt;
static UNSIGNED8	requestNode;
static CO_EVENT_T	reQuestNmtEvent;
#ifdef CO_BOOTUP_MANAGER
static UNSIGNED32	nmtSlaveAssignment[CO_SLAVE_ASSIGNMENT_CNT];
#endif /* CO_BOOTUP_MANAGER */




/***************************************************************************/
/**
*
* \brief coNmtStateReq - request NMT state change
*
* Request NMT state change for the given node 1..127.
* If \e node == 0, the NMT request is sent to all nodes.
* If the NMT sending master should enter the same NMT state
* as the addressed node
* the \e master flag has to be set to CO_TRUE.
* This is true for \e node == 0 too. 
*
* If \e node == the masters own nodeId, the requested state is only valid for the own node.
*
* If the inhibit time is set (see object 0x102a, NMT inhibit time),
* NMT command is not sent if time hasn't been elapsed.
*
* \return RET_T
*
*/
RET_T coNmtStateReq(
		UNSIGNED8		node,			/**< node */
		CO_NMT_STATE_T	reqState,		/**< new requested state */
		BOOL_T			master			/**< valid for master */
	)
{
UNSIGNED8		trData[CO_CAN_MAX_DATA_LEN];	/* data */
COB_REFERENZ_T	cobRef;
CO_REC_DATA_T	nmtRec;
UNSIGNED8		cmd;
RET_T			retVal;

	/* check for master mode */
	if (nmtMasterState == CO_NMT_MASTER_STATE_SLAVE)  {
		return(RET_SERVICE_NOT_INITIALIZED);
	}

	switch (reqState)  {
		case CO_NMT_STATE_OPERATIONAL:
			cmd = 1u;
			break;
		case CO_NMT_STATE_STOPPED:
			cmd = 2u;
			break;
		case CO_NMT_STATE_PREOP:
			cmd = 128u;
			break;
		case CO_NMT_STATE_RESET_NODE:
			cmd = 129u;
			break;
		case CO_NMT_STATE_RESET_COMM:
			cmd = 130u;
			break;
		case CO_NMT_STATE_UNKNOWN:
		default:
			return(RET_INVALID_PARAMETER);
	}

	cobRef = icoNmtGetCob();

	/* own node requested ? */
	if (node == coNmtGetNodeId()) {
		master = CO_TRUE;
	} else {
		trData[0] = cmd;
		trData[1] = (node & 0x7Fu);
		retVal = icoTransmitMessage(cobRef, &trData[0], 0u);
		if (retVal != RET_OK)  {
			return(retVal);
		}
	}

	if (master == CO_TRUE)  {
		/* simulate command for own node */
		nmtRec.msg.canId = 0u;
		nmtRec.msg.len = 2u;
		nmtRec.msg.data[0] = cmd;
		nmtRec.msg.data[1] = coNmtGetNodeId();
		icoNmtMsgHandler(&nmtRec);
	}

	return(RET_OK);
}


/***************************************************************************/
/**
*
* \brief icoNmtInhibitActive - check if inhibit is active
*
* \return inhibit state
*
*/
BOOL_T coNmtInhibitActive(
		void	/* no parameter */
	)
{
COB_REFERENZ_T	cobRef;
BOOL_T	state;

	cobRef = icoNmtGetCob();
	state = icoQueueInhibitActive(cobRef);
	
	return(state);
}


#ifdef xxx
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
void icoNmtMasterHandler(
		CO_CONST CO_REC_DATA_T *pRecData		/* pointer to receive data */
	)
{
	/* switch depending on cob-id */
	switch (pRecData->msg.canId)  {
		case CO_FLYMA_COB_ACT_MSTR_ANSWER:
			if (pRecData->msg.len != 2u)  {
				return;
			}
			checkActiveMasterData(pRecData->msg.data[0], pRecData->msg.data[1]);
			break;

	}
}
#endif /* xxx */


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/**
* \internal
*
* \brief icoNmtGetObjectAddr - get Nmt object address
*
* \return none
*
*/
void *icoNmtGetObjectAddr(
		UNSIGNED16	index,			/* index */
		UNSIGNED8	subIndex		/* subindex */
	)
{
void	*pAddr = NULL;

	switch (index)  {
		case 0x102au:
			pAddr = (void *)&nmtInhibit;
			break;
		case 0x1f80u:
			pAddr = (void *)&nmtStartup;
			break;

#ifdef CO_BOOTUP_MANAGER
		case 0x1f81u:
			pAddr = getNmtSlaveAssignment(subIndex - 1u);
			break;
#endif /* CO_BOOTUP_MANAGER */

		case 0x1f82u:
			pAddr = (void *)&requestNmt;
#ifdef CO_HB_CONSUMER_CNT
			requestNmt = coNmtGetRemoteNodeState(subIndex);
#else /* CO_HB_CONSUMER_CNT */
			requestNmt = CO_NMT_REQ_STATE_UNKNOWN;
			(void)subIndex;
#endif /* CO_HB_CONSUMER_CNT */
			break;

		default:
			break;
	}

	return(pAddr);
}


/***************************************************************************/
/**
* \internal
*
* \brief icoNmtObjChanged - Nmt object changed
*
*
* \return RET_T
*
*/
RET_T icoNmtObjChanged(
		UNSIGNED16	index,			/* index */
		UNSIGNED8	subIndex		/* subindex */
	)
{
COB_REFERENZ_T	cobRef;
RET_T	retVal = RET_OK;

	switch (index)  {
		case 0x102a:
			/* get nmt cob */
			cobRef = icoNmtGetCob();

			/* setup inhibit time (given in 100 usec) */
			retVal = icoCobSetInhibit(cobRef, nmtInhibit);
			break;
		case 0x1f82:
			switch((UNSIGNED16)requestNmt) {
				case CO_NMT_REQ_STATE_RESET_NODE:
					requestNmt = CO_NMT_STATE_RESET_NODE;
					break;
				case CO_NMT_REQ_STATE_RESET_COMM:
					requestNmt = CO_NMT_STATE_RESET_COMM;
					break;
				default:
					break;
			}
			requestNode = subIndex;
			retVal = icoEventStart(&reQuestNmtEvent,
				startNmtReq, NULL); /*lint !e960 */
			/* Derogation MisraC2004 R.16.9 function identifier used without '&'
			 * or parenthesized parameter */
			break;
		default:
			break;
	}
	return(retVal);
}


/***************************************************************************/
/**
* \internal
*
* \brief startNmtReq - eventfunction for NMT requests
*
* \return none
*
*/
static void startNmtReq(
		void *ptr
	)
{
(void)ptr;

	(void)coNmtStateReq(requestNode,
		requestNmt, CO_FALSE);
}


/***************************************************************************/
/**
* \internal
*
* \brief icoNmtSetDefaultValue - set predefined connection set
*
* \return none
*
*/
void icoNmtSetDefaultValue(
		void	/* no parameter */
	)
{
RET_T	retVal;
UNSIGNED16	u16;
UNSIGNED32	u32;
#ifdef CO_BOOTUP_MANAGER
UNSIGNED8	i;
UNSIGNED8	subIndex;
#endif /* CO_BOOTUP_MANAGER */

	/* setup reset values */
	retVal = coOdGetDefaultVal_u16(0x102au, 0u, &u16);
	if (retVal == RET_OK)  {
		nmtInhibit = u16;
	}

	retVal = coOdGetDefaultVal_u32(0x1f80u, 0u, &u32);
	if (retVal == RET_OK)  {
		nmtStartup = u32;
	}

#ifdef CO_BOOTUP_MANAGER
	for (i = 0u; i < CO_SLAVE_ASSIGNMENT_CNT; i++)  {
		subIndex = i + 1u;
		retVal = coOdGetDefaultVal_u32(0x1f81u, subIndex, &u32);
		if (retVal == RET_OK)  {
			*getNmtSlaveAssignment(i) = u32;
		}
	}
#endif /* CO_BOOTUP_MANAGER */
}


/***************************************************************************/
/**
* \internal
*
* \brief icoNmtMasterGetStartupObj - get startup object 0x1f80
*
* \return none
*
*/
UNSIGNED32	icoNmtMasterGetStartupObj(
		void	/* no parameter */
	)
{
	return(nmtStartup);
}


#ifdef CO_BOOTUP_MANAGER
/***************************************************************************/
/**
* \internal
*
* \brief icoNmtMasterGetSlaveAssignment - get object 0x1f81
*
* \return none
*
*/
UNSIGNED32 icoNmtMasterGetSlaveAssignment(
		UNSIGNED8		idx				/* subindex - 1 */
	)
{
	return(nmtSlaveAssignment[idx]);
}


static UNSIGNED32 *getNmtSlaveAssignment(
		UNSIGNED8		idx
	)
{
UNSIGNED32 *sa;

	sa = &nmtSlaveAssignment[idx];

	return(sa);
}
#endif /* CO_BOOTUP_MANAGER */

/***************************************************************************/
/**
* \internal
*
* \brief icoNmtMasterSetState - set master state
*
* \return none
*
*/
void icoNmtMasterSetState(
		CO_NMT_MASTER_STATE_T mState
	)
{
COB_REFERENZ_T	cobRef;

	nmtMasterState = mState;

	/* change cob type NMT */
	cobRef = icoNmtGetCob();
	if (nmtMasterState == CO_NMT_MASTER_STATE_SLAVE)  {
		(void)icoCobChangeType(cobRef, CO_COB_TYPE_RECEIVE);
	} else {
		(void)icoCobChangeType(cobRef, CO_COB_TYPE_TRANSMIT);
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief icoNmtMasterGetState - get nmt master state
*
* \return none
*
*/
CO_NMT_MASTER_STATE_T icoNmtMasterGetState(
		void	/* no parameter */
	)
{
	return(nmtMasterState);
}


/***************************************************************************/
/**
*
* \brief coNmtNodeIsMaster - detect if node is master
*
* \retval CO_TRUE - node is master
* \retval CO_FALSE - node is not master
*
*/
BOOL_T coNmtNodeIsMaster(
		void	/* no parameter */
	)
{
	if (nmtMasterState == CO_NMT_MASTER_STATE_MASTER)  {
		return(CO_TRUE);
	} else {
		return(CO_FALSE);
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
* \brief icoNmtMasterVarInit - init nmt variables
*
* \return none
*
*/
void icoNmtMasterVarInit(
		UNSIGNED8	*pSlaveAssign
	)
{
#ifdef CO_BOOTUP_MANAGER
#endif /* CO_BOOTUP_MANAGER */
(void)pSlaveAssign;

	{
		nmtInhibit = 0u;

#ifdef CO_BOOTUP_MANAGER
#endif /* CO_BOOTUP_MANAGER */
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief icoNmtMasterReset - NMT master reset
*
*
* \return void
*
*/
void icoNmtMasterReset(
		void	/* no parameter */
	)
{
COB_REFERENZ_T	cobRef;

	/* get nmt cob */
	cobRef = icoNmtGetCob();

	/* setup inhibit time (given in 100 usec) */
	(void)icoCobSetInhibit(cobRef, nmtInhibit);
}

#endif /* CO_NMT_MASTER */

