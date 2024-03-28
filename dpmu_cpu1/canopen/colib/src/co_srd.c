/*
* co_srd.c - contains SRD services
*
* Copyright (c) 2014-2022 emotas embedded communication GmbH
*-------------------------------------------------------------------
* $Id: co_srd.c 39658 2022-02-17 10:15:29Z boe $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \brief Service Request Device (SDO Manager Slave)
*
* \file co_srd.c
* contains routines for SRD slave handling
*
*/


/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <stdlib.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <gen_define.h>

#ifdef CO_SRD_SUPPORTED
#include <co_datatype.h>
#include <co_odaccess.h>
#include <co_drv.h>
#include <co_timer.h>
#include <co_srd.h>
#include <co_sdo.h>
#include <co_nmt.h>
#include "ico_indication.h"
#include "ico_srd.h"
#include "ico_event.h"
#include "ico_cobhandler.h"
#include "ico_queue.h"


/* constant definitions
---------------------------------------------------------------------------*/
#ifdef CO_EVENT_DYNAMIC_SRD
# ifdef CO_EVENT_PROFILE_SRD
#  define CO_EVENT_SRD_CNT	(CO_EVENT_DYNAMIC_SRD + CO_EVENT_PROFILE_SRD)
# else /* CO_EVENT_PROFILE_SRD */
#  define CO_EVENT_SRD_CNT	(CO_EVENT_DYNAMIC_SRD)
# endif /* CO_EVENT_PROFILE_SRD */
#else /* CO_EVENT_DYNAMIC_SRD */
# ifdef CO_EVENT_PROFILE_SRD
#  define CO_EVENT_SRD_CNT	(CO_EVENT_PROFILE_SRD)
# endif /* CO_EVENT_PROFILE_SRD */
#endif /* CO_EVENT_DYNAMIC_SRD */


#define CO_SRD_STATE_MASK		((UNSIGNED32)3u << 1u)
#define CO_SRD_STATE_ERROR		(0ul)					/* request unsuccessful */
#define CO_SRD_STATE_OK			((UNSIGNED32)1u << 1u)	/* registration succesful */
#define CO_SRD_STATE_ALL_OK		((UNSIGNED32)2u << 1u)	/* all sdo succesful */
#define CO_SRD_STATE_NODE_OK	((UNSIGNED32)3u << 1u)	/* to node successful */

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/
#ifdef CO_EVENT_STATIC_SRD
extern CO_CONST CO_EVENT_SRD_T coEventSrdInd;
#endif /* CO_EVENT_STATIC_SRD */


/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
static void srdIndikation(CO_SRD_RESULT_T result, UNSIGNED8 errorCode);
static void srdEvalAnswer(void *ptr);
static void srdTimeOut(void *ptr);

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
static UNSIGNED32		dynSdoConnState = { 0ul };
static COB_REFERENZ_T	dynReqCob;
static CO_TIMER_T		srdTimer;
static CO_EVENT_T		srdEvent;
static UNSIGNED16		sdoRegisterClientOffs;
static UNSIGNED8		sdoRegisterClient;
#ifdef CO_EVENT_SRD_CNT
static UNSIGNED8		srdTableCnt = 0u;
static CO_EVENT_SRD_T	srdTable[CO_EVENT_SRD_CNT];
#endif /* CO_EVENT_SRD_CNT */
static BOOL_T			srdRegistered;



/***************************************************************************/
/**
* \internal
*
* \brief srdEvalAnswer -
*
*
* \return none
*
*/
static void srdEvalAnswer(
		void *ptr
	)
{
RET_T	retVal;
UNSIGNED32	cobId;
(void)ptr;

	/* stop timeout timer */
	(void) coTimerStop(&srdTimer);

	switch (dynSdoConnState & CO_SRD_STATE_MASK)  {
		case CO_SRD_STATE_ERROR:
			/* eval error code */
			srdIndikation(CO_SRD_RESULT_ERROR, (UNSIGNED8)(dynSdoConnState >> 8));
			break;

		case CO_SRD_STATE_OK:
			/* registration ok */
			srdRegistered = CO_TRUE;

			/* call indication */
			srdIndikation(CO_SRD_RESULT_SUCCESS, 0u);
			break;

		case CO_SRD_STATE_ALL_OK:
			/* call indication */
			srdIndikation(CO_SRD_RESULT_ALL_REQUEST_SUCCESS, 0u);
			break;

		case CO_SRD_STATE_NODE_OK:
			/* call indication */
			srdIndikation(CO_SRD_RESULT_NODE_REQUEST_SUCCESS, 0u);
			break;


		default:
			break;
	}

	/* check, if channel to sdo manager exist */
	retVal = coOdGetObj_u32(0x1280u + sdoRegisterClientOffs, 1u, &cobId);
	if ((retVal != RET_OK) || ((cobId & 0x80000000u) != 0u)) {
		srdRegistered = CO_FALSE;
	}
}



/***************************************************************************/
/**
* \brief coSrdRegister - register SRD at SDO manager
*
* This function is only available in classical CAN mode.
*
* Request register as SRD at the SDO manager
* 
* If reqType == CO_SRD_REQ_TYPE_ALL_SDOS
*	sdoClientChannel is ignored
* If reqType == CO_SRD_REQ_TYPE_NORMAL
*	SDO client channel have to be from 1..128 (0x1280..0x12ff)
* This channel will be used as SDO client to the SDO manager.
*
* The answer will be done by calling function registered by coEventRegister_SRD()
*
*/
RET_T coSrdRequestRegister(
		CO_SRD_REQ_TYPE_T reqType,	/**< request type */
		UNSIGNED8 sdoClientChannel,	/**< sdo client channel to SDO Manager */
		UNSIGNED32 timeOut			/**< time out until service is aborted in msec */
	)
{
CO_CONST CO_OBJECT_DESC_T *pDescPtr;
RET_T	retVal;

	if (srdRegistered == CO_TRUE)  {
		return(RET_SERVICE_ALREADY_INITIALIZED);
	}

	/* check if SRD request is active */
	if (coTimerIsActive(&srdTimer) == CO_TRUE)  {
		return(RET_SERVICE_BUSY);
	}

	if ((sdoClientChannel < 1u) || (sdoClientChannel > 128u)) {
		return(RET_INVALID_PARAMETER);
	}

	sdoRegisterClient = sdoClientChannel;
	sdoRegisterClientOffs = (UNSIGNED16)sdoClientChannel - 1u;

	/* check for available SDO client channel */
	retVal = coOdGetObjDescPtr(0x1280u + sdoRegisterClientOffs, 0u, &pDescPtr);
	if (retVal != RET_OK)  {
		return(RET_INVALID_PARAMETER);
	}

	/* save sdo client channel */
	dynSdoConnState = ((0x1280ul + sdoRegisterClientOffs) << 16)
		| (UNSIGNED32)reqType;
	/* send registration message */
	retVal = icoTransmitMessage(dynReqCob, NULL, 0u);
	if (retVal != RET_OK)  {
		return(retVal);
	}

	/* start timeout timer */
	(void) coTimerStart(&srdTimer, timeOut * 1000ul,
				srdTimeOut, NULL, CO_TIMER_ATTR_ROUNDUP); /*lint !e960 */
	/* Derogation MisraC2004 R.16.9 function identifier used without '&'
	 * or parenthesized parameter */

	return(RET_OK);
}


/***************************************************************************/
/**
* \brief coSrdRequestConnection - request connection to remote node
*
* This function is only available in classical CAN mode.
*
* Request SDO connection to remote node 
* 
*
* The answer will be done by calling function registered coEventRegister_SRD()
*
*/
RET_T coSrdRequestConnection(
		UNSIGNED8 sdoClientChannel,	/**< sdo client channel to node */
		UNSIGNED8 remoteNodeId,		/**< node id of remote node */
		UNSIGNED32 timeOut			/**< time out until service is aborted */
	)
{
static UNSIGNED32	requestVal;
UNSIGNED8	nodeId = coNmtGetNodeId();
RET_T	retVal;
CO_CONST CO_OBJECT_DESC_T *pDescPtr;
UNSIGNED16	sdoClientOffs = (UNSIGNED16)sdoClientChannel - 1u;
void	*ptr = &requestVal;

	/* check if SRD request is active */
	if (coTimerIsActive(&srdTimer) == CO_TRUE)  {
		return(RET_SERVICE_BUSY);
	}

	if (srdRegistered != CO_TRUE)  {
		return(RET_SERVICE_NOT_INITIALIZED);
	}

	if ((sdoClientChannel < 1u) || (sdoClientChannel > 128u)) {
		return(RET_INVALID_PARAMETER);
	}

	/* check for available SDO client channel */
	retVal = coOdGetObjDescPtr(0x1280u + sdoClientOffs, 0u, &pDescPtr);
	if (retVal != RET_OK)  {
		return(RET_INVALID_PARAMETER);
	}

	/* same channel as to SDO manager ? */
	if (sdoClientChannel == sdoRegisterClient) {
		return(RET_INVALID_PARAMETER);
	}

	requestVal = ((0x1280ul + sdoClientOffs) << 16);
	requestVal |= ((UNSIGNED32)nodeId << 8u) | remoteNodeId;

	/* actual we don't check the sdo transfer state ...*/
	retVal = coSdoWrite(sdoRegisterClient, 0x1f00u, 0u,
			ptr, 4u, 1u, timeOut / 2u);
	if (retVal != RET_OK)  {
		return(retVal);
	}

	dynSdoConnState = ((0x1280ul + sdoClientOffs) << 16);

	/* start timeout timer */
	(void) coTimerStart(&srdTimer, timeOut * 1000ul,
				srdTimeOut, NULL, CO_TIMER_ATTR_ROUNDUP); /*lint !e960 */
	/* Derogation MisraC2004 R.16.9 function identifier used without '&'
	 * or parenthesized parameter */

	return(retVal);
}


/***************************************************************************/
/**
* \brief coSrdReleaseConnection - release connection to remote node
*
* This function is only available in classical CAN mode.
*
* release SDO connection to remote node 
* 
* If sdoClientChannel = 0, release all connections
* If remoteNodeId = 0 deregister at sdo manager
*
* The answer will be done by calling function registered coEventRegister_SRD()
*
*/
RET_T coSrdReleaseConnection(
		UNSIGNED8 sdoClientChannel,	/**< sdo client channel to node */
		UNSIGNED8 remoteNodeId,		/**< node id of remote node */
		UNSIGNED32 timeOut			/**< time out until service is aborted */
	)
{
static UNSIGNED32	requestVal;
UNSIGNED8	nodeId = coNmtGetNodeId();
RET_T	retVal;
UNSIGNED16	sdoClientOffs;
void	*ptr = &requestVal;

	/* check if SRD request is active */
	if (coTimerIsActive(&srdTimer) == CO_TRUE)  {
		return(RET_SERVICE_BUSY);
	}

	if (sdoClientChannel > 128u) {
		return(RET_INVALID_PARAMETER);
	}

	requestVal = ((UNSIGNED32)nodeId << 8) | remoteNodeId;
	dynSdoConnState = 0u;
	if (sdoClientChannel != 0u)  {	
		sdoClientOffs = (UNSIGNED16)sdoClientChannel - 1u;
		dynSdoConnState = ((0x1280ul + sdoClientOffs) << 16);
		requestVal |= dynSdoConnState;
	}

	/* actual we don't check the sdo transfer state ...*/
	retVal = coSdoWrite(sdoRegisterClient, 0x1f01u, 0u,
			ptr, 4u, 1u, timeOut / 2u);

	/* start timeout timer */
	(void) coTimerStart(&srdTimer, timeOut * 1000ul,
				srdTimeOut, NULL, CO_TIMER_ATTR_ROUNDUP); /*lint !e960 */
	/* Derogation MisraC2004 R.16.9 function identifier used without '&'
	 * or parenthesized parameter */

	return(retVal);
}



/***************************************************************************/
/**
* \internal
*
* \brief srdTimeout - time out over 
*
*
* \return none
*
*/
static void srdTimeOut(
		void			*ptr
	)
{
(void)ptr;

	srdIndikation(CO_SRD_RESULT_TIMEOUT, 0u);
}


/***************************************************************************/
/**
* \internal
*
* \brief srdIndikation - call registered indications
*
*
* \return none
*
*/
void *icoSrdGetObjectAddr(
		UNSIGNED16	idx
	)
{
(void) idx;

	return(&dynSdoConnState);
}


/***************************************************************************/
/**
* \internal
*
* \brief icoSrdCheckObjLimit_u32 -
*
*
* \return none
*
*/
RET_T icoSrdCheckObjLimit_u32(
		UNSIGNED32	value
	)
{
RET_T	retVal;
(void)value;

	/* call evaluation of sdo manager answer */
	retVal = icoEventStart(&srdEvent, srdEvalAnswer, NULL); /*lint !e960 */
	/* Derogation MisraC2004 R.16.9 function identifier used without '&'
	 * or parenthesized parameter */

	return(retVal);
}


/***************************************************************************/
/**
* \internal
*
* \brief srdIndikation - call registered indications
*
*
* \return none
*
*/
static void srdIndikation(
		CO_SRD_RESULT_T	result,		/* result of action */
		UNSIGNED8		errorCode	/* error code */
	)
{
#ifdef CO_EVENT_SRD_CNT
UNSIGNED8	i;

	for (i = 0u; i < srdTableCnt; i++)  {
		srdTable[i](result, errorCode);
	}
#endif /* CO_EVENT_SRD_CNT */

#ifdef CO_EVENT_STATIC_SRD
	coEventSrdInd(result, errorCode);
#endif /* CO_EVENT_STATIC_SRD */
}


#ifdef CO_EVENT_SRD_CNT
/***************************************************************************/
/**
* \brief coEventRegister_SRD - register SRD event
*
* This function is only available in classical CAN mode.
*
* register indication function for SRD events
*
* \return RET_T
*
*/

RET_T coEventRegister_SRD(
		CO_EVENT_SRD_T pFunction	/**< pointer to function */
    )
{
	/* set new indication function as first at the list */
	if (srdTableCnt >= CO_EVENT_SRD_CNT) {
		return(RET_EVENT_NO_RESSOURCE);
	}

	srdTable[srdTableCnt] = pFunction;	/* save function pointer */
	srdTableCnt++;

	return(RET_OK);
}
#endif /* CO_EVENT_SRD_CNT */


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
void icoSrdVarInit(
		void
	)
{

	{
		dynSdoConnState = 0ul;
		srdRegistered = CO_FALSE;

		dynReqCob = 0xffffu;

/*
static UNSIGNED16		sdoRegisterClientOffs;
static UNSIGNED8		sdoRegisterClient;
*/
#ifdef CO_EVENT_SRD_CNT
		srdTableCnt = 0u;
#endif /* CO_EVENT_SRD_CNT */
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief icoSrdReset
*
*
* \return none
*
*/
void icoSrdReset(
		void	/* no parameter */
	)
{
	(void) icoCobSet(dynReqCob, 0x6e0u, CO_COB_RTR_NONE, 0u);
}


/***************************************************************************/
/**
* \brief coInitSrd - init Srd functionality
*
*
* \return RET_T
*
*/
RET_T coSrdInit(
		void	/* no parameter */
	)
{
	dynReqCob = icoCobCreate(CO_COB_TYPE_TRANSMIT, CO_SERVICE_SRD_TRANSMIT, 0u);
	if (dynReqCob == 0xffffu)  {
		return(RET_NO_COB_AVAILABLE);
	}

	return(RET_OK);
}
#endif /* CO_SRD_SUPPORTED */
