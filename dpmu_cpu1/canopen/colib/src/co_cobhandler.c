/*
* co_cobhandler.c - contains functions for cob handling
*
* Copyright (c) 2012-2021 emotas embedded communication GmbH
*-------------------------------------------------------------------
* $Id: co_cobhandler.c 41218 2022-07-12 10:35:19Z boe $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \brief Functions for COB handling
*
* \file co_cobhandler.c contains functions for cob handling
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
#include <co_drv.h>
#include "ico_common.h"
#include "ico_cobhandler.h"
#include "ico_queue.h"

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
static RET_T cobFilterSet(COB_REFERENZ_T cobRef);
#ifdef CO_COB_SORTED_LIST
static void icoCobSortList(const CO_COB_T *pCob);
#endif /* CO_COB_SORTED_LIST */
static CO_COB_T *getCobData(COB_REFERENZ_T idx);


/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
static COB_REFERENZ_T	cobRefCnt;
static CO_COB_T			cobData[CO_COB_CNT];
#ifdef CO_COB_SORTED_LIST
static COB_REFERENZ_T	cobSortCnt;
static COB_REFERENZ_T	cobSortedList[CO_COB_CNT];
#endif /* CO_COB_SORTED_LIST */


/***************************************************************************/
/**
*
* \internal
*
* \brief cobFilterSet - configure filter
*
* \return RET_T
*
*/
static RET_T cobFilterSet(
		COB_REFERENZ_T	cobRef
	)
{
RET_T retval = RET_OK;
#ifdef CO_DRV_FILTER
UNSIGNED8	enable;
#endif /* CO_DRV_FILTER */
#if defined(CO_SYNC_SUPPORTED) || defined(CO_DRV_FILTER)
CO_COB_T *pCob;
#endif /* defined(CO_SYNC_SUPPORTED) || defined(CO_DRV_FILTER) */

#if defined(CO_SYNC_SUPPORTED) || defined(CO_DRV_FILTER)
	pCob = getCobData(cobRef);
#endif /* defined(CO_SYNC_SUPPORTED) || defined(CO_DRV_FILTER) */

#ifdef CO_SYNC_SUPPORTED
	if ((pCob->service == CO_SERVICE_SYNC_TRANSMIT)
	 || (pCob->service == CO_SERVICE_SYNC_RECEIVE))  {
		if (((pCob->canCob.flags & CO_COBFLAG_ENABLED) != 0u)
		 && (pCob->type == CO_COB_TYPE_RECEIVE))  {
			icoQueueSetSyncId(pCob->canCob.canId,
					pCob->canCob.flags, CO_TRUE);
		} else {
			icoQueueSetSyncId(pCob->canCob.canId,
					pCob->canCob.flags, CO_FALSE);
		}
	}
#endif /* CO_SYNC_SUPPORTED */

#ifdef CO_DRV_FILTER
	/* special case: change to TRANSMIT without RTR
	 * disable it temporary, call filter function, enable it again
	 */
	if ((pCob->type == CO_COB_TYPE_TRANSMIT)
	 && ((pCob->canCob.flags & CO_COBFLAG_RTR) == 0u))  
	{
		/* save cob enable state */
		enable = pCob->canCob.flags & CO_COBFLAG_ENABLED;
		pCob->canCob.flags &= ~CO_COBFLAG_ENABLED;
		/* inform driver - disable filter */
		retval = codrvCanSetFilter(&pCob->canCob);
		/* restore enable state */
		pCob->canCob.flags |= enable;
	} else 
		/* special case: change to Receive with RTR Request
	 	 * -> receive data frames
	 	*/

	if ((pCob->type == CO_COB_TYPE_RECEIVE)
	 && ((pCob->canCob.flags & CO_COBFLAG_RTR) != 0u))  
	{	
		/* receive data frame */
		pCob->canCob.flags &= ~CO_COBFLAG_RTR;
		retval = codrvCanSetFilter(&pCob->canCob);
		pCob->canCob.flags |= CO_COBFLAG_RTR; /* restore old value */

	} else
	{
		/* inform driver */
		retval = codrvCanSetFilter(&pCob->canCob);
	}
#else /* CO_DRV_FILTER */
	(void)cobRef;
#endif /* CO_DRV_FILTER */

	return(retval);
}


/***************************************************************************/
/**
*
* \internal
*
* \brief icoCobCreate
*
* \return COB_REFERENZ_T (not line depending!)
*
*/
COB_REFERENZ_T icoCobCreate(
		CO_COB_TYPE_T	cobType,			/* cob type */
		CO_SERVICE_T	service,			/* service type */
		UNSIGNED16		serviceNr			/* service number */
	)
{
COB_REFERENZ_T	cobRef;
CO_COB_T *pCob;

	/* cob(s) available ? */
	if (cobRefCnt >= CO_COB_CNT)  {
		return(0xffffu);
	}

	cobRef = cobRefCnt;
	cobRefCnt++;

	pCob = getCobData(cobRef);

	/* fill into cob structure */
	pCob->cobNr = cobRef + 1u;
	pCob->type = cobType;
	pCob->canCob.flags = CO_COBFLAG_NONE;
	pCob->canCob.canId = CO_COB_INVALID;
	pCob->canCob.ignore = 0u;
	pCob->canCob.canChan = 0xFFFFu;
	pCob->service = service;
	pCob->serviceNr = serviceNr;
	pCob->inhibit = 0u;

#ifdef CO_SYNC_SUPPORTED
	if ((pCob->service == CO_SERVICE_SYNC_TRANSMIT)
	 || (pCob->service == CO_SERVICE_SYNC_RECEIVE))  {
		icoQueueSetSyncId(pCob->canCob.canId, 0u, CO_FALSE);
	}
#endif /* CO_SYNC_SUPPORTED */

	return(cobRef);
}


/***************************************************************************/
/**
*
* \internal
*
* \brief icoCobSet - set cob data 
*
*
* \return RET_T
*
*/
RET_T icoCobSet(
		COB_REFERENZ_T	cobRef,			/* cob reference */
		UNSIGNED32		cobId,			/* cob-id */
		CO_COB_RTR_T	rtr,			/* rtr flag */
		UNSIGNED8		len				/* data len */
	)
{
CO_COB_T	*pCob;
RET_T		retval;

	if (cobRef > (cobRefCnt))  {
		return(RET_NO_COB_AVAILABLE);
	}

	pCob = &cobData[cobRef];
	if ((cobId & CO_COB_VALID_MASK) == CO_COB_INVALID)  {
		pCob->canCob.flags &= ~CO_COBFLAG_ENABLED;
	} else {
		if (len > CO_CAN_MAX_DATA_LEN)  {
			return(RET_SDO_INVALID_VALUE);
		}
		pCob->canCob.flags |= CO_COBFLAG_ENABLED;
	}

	pCob->canCob.canId = cobId & CO_COB_ID_MASK;

	if ((cobId & CO_COB_29BIT_MASK) == CO_COB_29BIT)  {
		pCob->canCob.flags |= CO_COBFLAG_EXTENDED;
	} else {
		pCob->canCob.flags &= ~CO_COBFLAG_EXTENDED;
	}

	if (rtr == CO_COB_RTR_NONE)  {
		pCob->canCob.flags &= ~CO_COBFLAG_RTR;
	} else {
		pCob->canCob.flags |= CO_COBFLAG_RTR;
	}

	pCob->len = len;

	retval = cobFilterSet(cobRef);
	if (retval != RET_OK) {
		return retval;
	}

#ifdef CO_COB_SORTED_LIST
	icoCobSortList(pCob);
#endif /* CO_COB_SORTED_LIST */

	return(RET_OK);
}


#ifdef CO_COB_SORTED_LIST
/******************************************************************/
/**
*
* \internal
*
* \brief icoCobSortList - sort cob list
*
*
* \return RET_T
*
*/
static void icoCobSortList(
		const CO_COB_T	*pCob
	)
{
UNSIGNED16 cobCnt;
UNSIGNED16 cnt;
UNSIGNED16 tmpCobNbr = 0u;
UNSIGNED32 mask;

	/* use only receive cobs or RTR */
	if ((pCob->type != CO_COB_TYPE_RECEIVE) &&
		((pCob->type == CO_COB_TYPE_TRANSMIT) &&
		((pCob->canCob.flags & CO_COBFLAG_RTR) == 0u)))  {
		return;
	}

	cobCnt = cobSortCnt;
	cnt = 0u;

	/* First see if cob already used */
	for ( ; cnt < cobCnt; cnt++) {
		if (cobSortedList[cnt] == pCob->cobNr) {
			/* yes already used */
			cobSortCnt--;
			break;
		}
	}
	/* if already used delete it */
	for ( ; cnt < cobCnt; cnt++) {
		if ((cnt + 1u) == cobCnt) {
			cobSortedList[cnt] = 0u;
		} else {
			cobSortedList[cnt] = cobSortedList[cnt + 1u];
		}
	}

	mask = pCob->canCob.ignore;

	if ((pCob->canCob.flags & CO_COBFLAG_ENABLED) != 0u) {
		cobSortCnt++;
		cnt = 0u;
		cobCnt = cobSortCnt;
		/* put new cob in sorted list */
		for ( ; cnt < cobCnt; cnt++ ) {
			if (cobSortedList[cnt] == 0u) {
				cobSortedList[cnt] = pCob->cobNr;
				break;
			}

			if ((pCob->canCob.canId & ~mask) < (cobData[cobSortedList[cnt] - 1u].canCob.canId & ~mask)) {
				tmpCobNbr = cobSortedList[cnt];
				cobSortedList[cnt] = pCob->cobNr;
				pCob = &cobData[tmpCobNbr - 1u];
			}
		}
	}
}
#endif /* CO_COB_SORTED_LIST */


/******************************************************************/
/**
*
* \internal
*
* \brief icoCobChangeType - change cob type
*
*
* \return RET_T
*
*/
RET_T icoCobChangeType(
		COB_REFERENZ_T	cobRef,			/* cob reference */
		CO_COB_TYPE_T	cobType			/* new cob type */
	)
{
RET_T retval;
CO_COB_T *pCob;

	pCob = getCobData(cobRef);
	if (pCob == NULL)  {
		return(RET_NO_COB_AVAILABLE);
	}

	pCob->type = cobType;

	retval = cobFilterSet(cobRef);

	return(retval);
}


/******************************************************************/
/**
*
* \internal
*
* \brief icoCobChangeService - change cob service
*
*
* \return RET_T
*
*/
RET_T icoCobChangeService(
		COB_REFERENZ_T	cobRef,			/* cob reference */
		CO_SERVICE_T	service,		/* service type */
		UNSIGNED16		serviceNr		/* service number */
	)
{
CO_COB_T *pCob;

	pCob = getCobData(cobRef);
	if (pCob == NULL)  {
		return(RET_NO_COB_AVAILABLE);
	}

	pCob->service = service;
	pCob->serviceNr = serviceNr;

	return(RET_OK);
}


/******************************************************************/
/**
*
* \internal
*
* \brief icoCobSetInhibit - set inhibit time
*
*
* \return RET_T
*
*/
RET_T icoCobSetInhibit(
		COB_REFERENZ_T	cobRef,			/* cob reference */
		UNSIGNED16	inhibit				/* inhibit time */
	)
{
CO_COB_T *pCob;

	pCob = getCobData(cobRef);
	if (pCob == NULL)  {
		return(RET_NO_COB_AVAILABLE);
	}

	pCob->inhibit = inhibit;

	/* delete old messages from inhibit list if new time = 0 */
	if (inhibit == 0u)  {
		icoQueueDeleteInhibit(cobRef);
	}

	return(RET_OK);
}




/******************************************************************/
/**
*
* \internal
*
* \brief icoCobSetIgnore - set ignore mask
*
*
* \return RET_T
*
*/
RET_T icoCobSetIgnore(
	COB_REFERENZ_T	cobRef,			/* cob reference */
	UNSIGNED32		mask			/* ignore mask */
	)
{
RET_T retval;
CO_COB_T *pCob;

	pCob = getCobData(cobRef);
	if (pCob == NULL)  {
		return(RET_NO_COB_AVAILABLE);
	}

	pCob->canCob.ignore = mask;

	retval = cobFilterSet(cobRef);

	return(retval);
}


/***************************************************************************/
/**
*
* \internal
*
* \brief icoCobSetLen - update len information for COB
*
*
* \return RET_T
*
*/
RET_T icoCobSetLen(
		COB_REFERENZ_T	cobRef,			/* cob reference */
		UNSIGNED8		len				/* data len */
	)
{
CO_COB_T *pCob;

	pCob = getCobData(cobRef);
	if (pCob == NULL)  {
		return(RET_NO_COB_AVAILABLE);
	}

	if (len > CO_CAN_MAX_DATA_LEN)  {
		return(RET_SDO_INVALID_VALUE);
	}

	pCob->len = len;

	return(RET_OK);
}


/******************************************************************/
/**
* \internal
*
* \brief icoCobGet - get a cob rerenced by cobref
*
*
* \return cob
*
*/
UNSIGNED32 icoCobGet(
		COB_REFERENZ_T	cobRef			/* cob reference */
	)
{
UNSIGNED32	cobId;
CO_COB_T *pCob;

	pCob = getCobData(cobRef);
	if (pCob == NULL)  {
		return(0ul);
	}

	cobId = pCob->canCob.canId;
	if ((cobData[cobRef].canCob.flags & CO_COBFLAG_EXTENDED) != 0u) {
		cobId |= CO_COB_29BIT;
	}
	if ((pCob->canCob.flags & CO_COBFLAG_ENABLED) == 0u) {
		cobId |= CO_COB_INVALID;
	}

	return(cobId);
}


/******************************************************************/
/**
*
* \internal
*
* \brief icoCobEnable - enable a cob
*
*
* \return RET_T
*
*/
RET_T icoCobEnable(
		COB_REFERENZ_T	cobRef			/* cob reference */
	)
{
RET_T retval;
CO_COB_T *pCob;

	pCob = getCobData(cobRef);
	if (pCob == NULL)  {
		return(RET_NO_COB_AVAILABLE);
	}

	pCob->canCob.flags |= CO_COBFLAG_ENABLED;

	retval = cobFilterSet(cobRef);
	if (retval != RET_OK) {
		return retval;
	}

#ifdef CO_COB_SORTED_LIST
	icoCobSortList(pCob);
#endif /* CO_COB_SORTED_LIST */

	return(RET_OK);
}


/******************************************************************/
/**
*
* \internal
*
* \brief icoCobDisable - disable a cob
*
*
* \return RET_T
*
*/
RET_T icoCobDisable(
		COB_REFERENZ_T	cobRef			/* cob reference */
	)
{
RET_T retval;
CO_COB_T *pCob;

	pCob = getCobData(cobRef);
	if (pCob == NULL)  {
		return(RET_NO_COB_AVAILABLE);
	}

	pCob->canCob.flags &= ~CO_COBFLAG_ENABLED;

	retval = cobFilterSet(cobRef);
	if (retval != RET_OK) {
		return retval;
	}

#ifdef CO_COB_SORTED_LIST
	icoCobSortList(pCob);
#endif /* CO_COB_SORTED_LIST */

	return(RET_OK);
}


/******************************************************************/
/**
* \internal
*
* \brief icoCobGetPointer - get a pointer to cob
*
*
* \return CO_COB_T
*	pointer to cob data
*/
CO_COB_T	*icoCobGetPointer(
		COB_REFERENZ_T	cobRef			/* cob reference */
	)
{
	if (cobRef > (cobRefCnt))  {
		return(NULL);
	}

	return(&cobData[cobRef]);
}


/******************************************************************/
/**
*
* \internal
*
* \brief icoCobCheck - get cob structure for given can id
*
*
* \return RET_T
*
*/
CO_COB_T	*icoCobCheck(
		const CO_CAN_REC_MSG_T	*pRecMsg	/* can rec data */
	)
{
CO_COB_T	*pCob = NULL;
UNSIGNED32	mask;

#ifdef CO_COB_SORTED_LIST
UNSIGNED16	left = 0u;
UNSIGNED16	right = cobSortCnt - 1u;
UNSIGNED16	middle = 0u;
BOOL_T		found = CO_FALSE;
BOOL_T		breakLoop = CO_FALSE;

	while (left <= right) {
		/* l + r / 2 */
		middle = (left + right) >> 1;
		pCob = &cobData[cobSortedList[middle] - 1u];
		mask = pCob->canCob.ignore;
		if (((pRecMsg->canId & ~mask) == (pCob->canCob.canId & ~mask)) 
		 && ((pRecMsg->flags & CO_COBFLAG_EXTENDED) == (pCob->canCob.flags & CO_COBFLAG_EXTENDED))
			)  {
			/* receive cob or transmit_rtr */
			if ((CO_COB_TYPE_RECEIVE == pCob->type)
			 || ((CO_COB_TYPE_TRANSMIT == pCob->type)
					&& ((pRecMsg->flags & CO_COBFLAG_RTR) != 0u)
					&& ((pRecMsg->flags & CO_COBFLAG_RTR) == (pCob->canCob.flags & CO_COBFLAG_RTR)))) {
				found = CO_TRUE;
				breakLoop = CO_TRUE;	/* break; */
			}
		}
		if ((pCob->canCob.canId & ~mask) > (pRecMsg->canId & ~mask)) {
			if (middle == 0u) {
				breakLoop = CO_TRUE;	/* break; */
			}
			right = middle - 1u;
		}
		else {
			left = middle + 1u;
		}
		/* finish loop ? */
		if (breakLoop == CO_TRUE) {
			break;
		}
	}

	if (found == CO_TRUE) {
		return(pCob);
	}

#else /* CO_COB_SORTED_LIST */
UNSIGNED16	cnt;

	for (cnt = 0u; cnt < cobRefCnt; cnt++)  {
		pCob = &cobData[cnt];
		mask = pCob->canCob.ignore;

#ifdef PRINT_COB_DEBUG
		printf("COBcheck: recId: %x, cobid: %x, mask %x\n",
			pRecMsg->canId,  pCob->canCob.canId, mask);
		printf("COBcheck masked: recId: %x, cobId: %x\n",
			pRecMsg->canId & mask,  pCob->canCob.canId & mask);
		printf("COBcheck flags: recFlags: %x, cobFlags: %x\n",
			pRecMsg->flags, pCob->canCob.flags);
		printf("\n");
#endif /* PRINT_COB_DEBUG */

		if (((pRecMsg->canId & ~mask) == (pCob->canCob.canId & ~mask) ) 
		 && ((pRecMsg->flags & CO_COBFLAG_EXTENDED) == (pCob->canCob.flags & CO_COBFLAG_EXTENDED))
		 && ((pCob->canCob.flags & CO_COBFLAG_ENABLED) != 0u))  {
			/* receive cob or transmit_rtr */
			if ((CO_COB_TYPE_RECEIVE == pCob->type) 
			 || (  (CO_COB_TYPE_TRANSMIT == pCob->type)
				&& ((pRecMsg->flags & CO_COBFLAG_RTR) != 0u)
				&& ((pRecMsg->flags & CO_COBFLAG_RTR) == (pCob->canCob.flags & CO_COBFLAG_RTR)))) {
				return(pCob);
			}
		}
	}
#endif /* CO_COB_SORTED_LIST */
	return(NULL);
}


/******************************************************************/
/**
*
* \internal
*
* \brief icoCobDisableAll - disable all cobs in are certain area
*
*
* \return none
*
*/
void icoCobDisableAll(
		CO_SERVICE_T start,			/* start enum value of the area */
		CO_SERVICE_T end			/* end enum value of the area */
	)
{
COB_REFERENZ_T	i;
const CO_COB_T *pCob;

	for (i = 0u; i < cobRefCnt; i++)  {
		pCob = getCobData(i);
		if ((pCob->service >= start)
			&& (pCob->service <= end))  {
			(void)icoCobDisable(i);
		}
	}
}




/******************************************************************/
/**
*
* \internal
*
* \brief icoCheckRestrictedCobs
*
*
* \return BOOL_T
*
*/
BOOL_T icoCheckRestrictedCobs(
		UNSIGNED32	canId,			/* new cobid */
		UNSIGNED32	exceptFirst,	/* allow cobid range from */
		UNSIGNED32	exceptLast		/* allow cobid range to */
	)
{
typedef struct {
	UNSIGNED32	first;
	UNSIGNED32	last;
} CO_NV_STORAGE RESTRICTED_COBS_T;
static const RESTRICTED_COBS_T	restrictCobs[] = {
	{ 0x000u,	0x000u },
	{ 0x001u,	0x07fu },
	{ 0x101u,	0x180u },
	{ 0x581u,	0x5ffu },
	{ 0x601u,	0x67fu },
	{ 0x6e0u,	0x6ffu },
	{ 0x701u,	0x77fu },
	{ 0x780u,	0x7ffu },
};
UNSIGNED8	i;

	for (i = 0u; i < (sizeof(restrictCobs) / sizeof(RESTRICTED_COBS_T)); i++)  {
		if ((canId >= restrictCobs[i].first)
		 && (canId <= restrictCobs[i].last))  {
			if ((canId < exceptFirst) || (canId > exceptLast))  {
				return(CO_TRUE);
			}
		}
	}

	return(CO_FALSE);
}


static CO_COB_T *getCobData(
		COB_REFERENZ_T	idx
	)
{
	if (idx > (cobRefCnt))  {
		return(NULL);
	}

	return(&cobData[idx]);
}


/******************************************************************/
/**
*
* \internal
*
* \brief icoCobHandlerInit - init cob handler variables
*
*
* \return BOOL_T
*
*/
void icoCobHandlerVarInit(
		void
	)
{
	cobRefCnt = 0u;
	
#ifdef CO_COB_SORTED_LIST
	cobSortCnt = 0u;
	{
	UNSIGNED16		cnt;
		for (cnt = 0u; cnt < CO_COB_CNT; cnt++) {
			cobSortedList[cnt] = 0u;
		}
	}
#endif /* CO_COB_SORTED_LIST */
}
