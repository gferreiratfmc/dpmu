/*
* co_event.c - contains event routines
*
* Copyright (c) 2012-2022 emotas embedded communication GmbH
*-------------------------------------------------------------------
* $Id: co_event.c 39658 2022-02-17 10:15:29Z boe $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \brief event routines
*
* \file co_event.c
* contains event routines
*
*/

/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <string.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <gen_define.h>

#include <co_datatype.h>
#include "ico_commtask.h"
#include "ico_event.h"

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

/* local defined variables
---------------------------------------------------------------------------*/
static CO_EVENT_T	*pCoEvent;
static CO_EVENT_T	*pCoPendEvent;


/***************************************************************************/
/**
* \internal
*
* \brief coEventStart - start a event
*
* This function add an event at end of the event list
*
* \return
*	RET_T
*/
RET_T icoEventStart(
		CO_EVENT_T	*pEvent,			/* pointer to eventstruct */
		CO_EVENT_FCT_T	ptrToFct,		/* function for event */
		void		*pData				/* pointer for own data */
	)
{
CO_EVENT_T	*pT;

	pEvent->pFct = ptrToFct;
	pEvent->pData = pData;

	/* first event ? */
	if (pCoEvent == NULL)  {
		pCoEvent = pEvent;
	} else {
		/* add at end of list */
		pT = pCoEvent;
		while (pT->pNext != NULL) {
			pT = pT->pNext;
		}
		pT->pNext = pEvent;
	}

	pEvent->pNext = NULL;
	coCommTaskSet(CO_COMMTASK_EVENT_NEW_EVENT);

	return(RET_OK);
}


/***************************************************************************/
/**
* \brief icoEventStop - stop an event
*
* This function removes an element from the event list
* as well as from the pending events list
*
* \return
*	RET_T
*/
RET_T icoEventStop(
		CO_EVENT_T	*pEvent			/* pointer to event struct */
	)
{
CO_EVENT_T * pCurEvent;
CO_EVENT_T * pLastEvent;

	/* remove from event list */
	pCurEvent = pCoEvent;
	pLastEvent = NULL;
	while (pCurEvent != NULL)  {
		if (pCurEvent == pEvent)  {
			if (pLastEvent == NULL)  {
				pCoEvent = pCurEvent->pNext;
			} else {
				pLastEvent->pNext = pCurEvent->pNext;
			}
		}
		pLastEvent = pCurEvent;
		pCurEvent = pCurEvent->pNext;
	}

	/* remove from pending event list */
	pCurEvent = pCoPendEvent;
	pLastEvent = NULL;
	while (pCurEvent != NULL)  {
		if (pCurEvent == pEvent)  {
			if (pLastEvent == NULL)  {
				pCoPendEvent = pCurEvent->pNext;
			} else {
				pLastEvent->pNext = pCurEvent->pNext;
			}
		}
		pLastEvent = pCurEvent;
		pCurEvent = pCurEvent->pNext;
	}

	return(RET_OK);
}


/***************************************************************************/
/**
* \internal
*
* \brief icoEventPend - pends a event
*
* This function adds an event at end of the event pending list. The events
* in that list are not active.
*
* \return
*	RET_T
*/
RET_T icoEventPend(
		CO_EVENT_T	*pEvent,			/* pointer to eventstruct */
		CO_EVENT_FCT_T	ptrToFct,		/* function for event */
		void		*pData,				/* pointer for own data */
		CO_EVENT_TYPE_T	type			/* type of pending event */
	)
{
CO_EVENT_T	*pT;

	pEvent->pFct = ptrToFct;
	pEvent->pData = pData;
	pEvent->type = type;

	/* first event ? */
	if (pCoPendEvent == NULL)  {
		pCoPendEvent = pEvent;
	} else {
		/* add at end of list */
		pT = pCoPendEvent;
		while (pT->pNext != NULL) {
			pT = pT->pNext;
		}
		pT->pNext = pEvent;
	}

	pEvent->pNext = NULL;

	return(RET_OK);
}


/***************************************************************************/
/**
* \internal
*
* \brief coEventIsActive - check if event is active
*
* With this function can be ckecked,
* if a event is currently in the event list.
*
* \return BOOL_T
* \retval CO_TRUE
*	event is active
* \retval CO_FALSE
*	event is not active
*
*/
BOOL_T icoEventIsActive(
		CO_CONST CO_EVENT_T	*pEvent		/* pointer to event struct */
	)
{
CO_EVENT_T	*pT = pCoEvent;

	while (pT != NULL)  {
		if (pT == pEvent)  {
			return(CO_TRUE);
		}
		pT = pT->pNext;
	}

	return(CO_FALSE);
}


/***************************************************************************/
/**
* \internal
*
* \brief icoEventIsPending - check if event is in the pending list
*
* With this function can be ckecked,
* if a event is currently in the pending event list.
*
* \return BOOL_T
* \retval CO_TRUE
*	event is in the list
* \retval CO_FALSE
*	event is not in the list
*
*/
BOOL_T icoEventIsPending(
		CO_CONST CO_EVENT_T	*pEvent		/* pointer to event struct */
	)
{
CO_EVENT_T	*pT = pCoPendEvent;

	while (pT != NULL)  {
		if (pT == pEvent)  {
			return(CO_TRUE);
		}
		pT = pT->pNext;
	}

	return(CO_FALSE);
}


/***************************************************************************/
/**
* \internal
*
* \brief icoEventsAvailable - check for events
*
* \return BOOL_T
*
*/
BOOL_T icoEventsAvailable(
		void	/* no parameter */
	)
{
	if (pCoEvent == NULL)  {
		return(CO_FALSE);
	}

	return(CO_TRUE);
}


/***************************************************************************/
/**
* \internal
*
* \brief icoEventCheck - check next event
*
*
*/
void icoEventCheck(
		void	/* no parameter */
	)
{
CO_EVENT_T	*pAct = NULL;
CO_EVENT_T	*pEv = pCoEvent;

	pCoEvent = NULL;

	while (pEv != NULL)  {
		pAct = pEv;

		/* remove event from list */
		pEv = pAct->pNext;
		pAct->pNext = NULL;

		/* call event functions */
		if (pAct->pFct != NULL)  {
			pAct->pFct(pAct->pData);
		}
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief icoEventPost - post event for a given type to the active list
*
*
*/
void icoEventPost(
		CO_EVENT_TYPE_T	type				/* event type */
	)
{
CO_EVENT_T	*pAct = NULL;
CO_EVENT_T	*pEv = pCoPendEvent;

	if (pEv == NULL)  {
		return;
	}

	while (pEv->pNext != NULL)  {
		pAct = pEv->pNext;
		if (pAct->type == type) {
			pEv->pNext = pAct->pNext;
			(void)icoEventStart(pAct, pAct->pFct, pAct->pData);
		} else {
			pEv = pAct;
		}
	}

	/* test the head */
	pAct = pCoPendEvent;

	if (pAct->type == type)  {
		pCoPendEvent = pAct->pNext;
		(void)icoEventStart(pAct, pAct->pFct, pAct->pData);
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief icoEventInit - init event interval
*
* This function initializes the internal event handling.
*
*
*/
void icoEventInit(
		void
	)
{

	pCoEvent = NULL;
	pCoPendEvent = NULL;
}
