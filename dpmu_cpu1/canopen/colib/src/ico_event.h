/*
* ico_event.h - contains internal defines for event handling
*
* Copyright (c) 2012-2020 emotas embedded communication GmbH
*
*-------------------------------------------------------------------
* $Id: ico_event.h 39658 2022-02-17 10:15:29Z boe $

*-------------------------------------------------------------------
*
*
*/

/**
* \file
* \brief dataty type header
*/

#ifndef ICO_EVENT_H
#define ICO_EVENT_H 1


/* datatypes */
typedef enum  {
	CO_EVENT_TYPE_UNKNOWN = 0,
	CO_EVENT_TYPE_J1939_BAM_TX
}CO_EVENT_TYPE_T;


/**
* event structure 
*/
typedef struct co_event {
	struct co_event	*pNext;			/**< pointer to next event */
	void			(*pFct)(void *para);/**< pointer to own function */
	void			*pData;			/**< pointer for own data */
	CO_EVENT_TYPE_T	type;
} xEvent;
typedef struct co_event	CO_EVENT_T;


/** \brief function pointer to event indication
 * \param pFct - pointer to timer up function
 */
typedef void (* CO_EVENT_FCT_T)(void *pFct);


/* function prototypes */

void	icoEventCheck(void);
void	icoEventInit(void);
RET_T	icoEventStart(CO_EVENT_T *pEvent,
			CO_EVENT_FCT_T	ptrToFct, void *pData);
RET_T 	icoEventStop(	CO_EVENT_T	*pEvent);
BOOL_T	icoEventIsActive(CO_CONST CO_EVENT_T *pEvent);
BOOL_T	icoEventsAvailable(void);

RET_T	icoEventPend(CO_EVENT_T* pEvent,
			CO_EVENT_FCT_T ptrToFct, void* pData, CO_EVENT_TYPE_T type);
void	icoEventPost(CO_EVENT_TYPE_T type);
BOOL_T icoEventIsPending(CO_CONST CO_EVENT_T* pEvent);

#endif /* ICO_EVENT_H */
