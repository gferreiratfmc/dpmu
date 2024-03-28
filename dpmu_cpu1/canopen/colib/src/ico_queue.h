/*
* ico_queue.h - contains defines for internal queue handling
*
* Copyright (c) 2012-2019 emotas embedded communication GmbH
*
*-------------------------------------------------------------------
* $Id: ico_queue.h 30486 2020-02-04 15:33:49Z boe $
*
*-------------------------------------------------------------------
*
*
*/

/**
* \file
* \brief dataty type header
*/

#ifndef ICO_QUEUE_H
#define ICO_QUEUE_H 1


/* datatypes */

typedef struct {
	CO_SERVICE_T 	service;		/* canopen service */
	UNSIGNED16		spec;			/* service specification */
	CO_CAN_REC_MSG_T	msg;		/* can message */
} CO_REC_DATA_T;


/* function prototypes */

BOOL_T	icoQueueGetReceiveMessage(CO_REC_DATA_T *pRecData);
RET_T	icoTransmitMessage(COB_REFERENZ_T cobRef,
			CO_CONST UNSIGNED8 *pData, UNSIGNED8 flags);
void	icoQueueHandler(void);
void	icoQueueDisable(BOOL_T on);
void	icoQueueDeleteInhibit(COB_REFERENZ_T cobRef);
void	icoQueueVarInit(CO_CONST UNSIGNED16 *recQueueCnt, CO_CONST UNSIGNED16 *trQueueCnt);
BOOL_T	icoQueueInhibitActive(COB_REFERENZ_T cobRef);
void	icoQueueSetSyncId(UNSIGNED32 syncId, UNSIGNED8 flags, BOOL_T enable);
UNSIGNED32 icoQueueGetTransBufFillState(void);

#endif /* ICO_QUEUE_H */

