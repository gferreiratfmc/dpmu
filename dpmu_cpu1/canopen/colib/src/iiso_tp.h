/*
* iiso_tp.h - contains defines and internal iso-tp services
*
* Copyright (c) 2018-2021 emotas embedded communication GmbH
*
*-------------------------------------------------------------------
* $Id: $

*-------------------------------------------------------------------
*
*
*/

/**
* \file
* \brief internal header for iso tp
*/

#ifndef IISO_TP_H
#define IISO_TP_H 1

/* datatypes */
typedef enum {
	ISOTP_STATE_SERVER_FREE,
	ISOTP_STATE_SERVER_WAIT_DATA,
	ISOTP_STATE_CLIENT_FREE,
	ISOTP_STATE_CLIENT_WAIT_CTS
} ISOTP_SERVER_STATE_T;


/* function prototypes */
void iisoTpClientMessageHandler(const CO_REC_DATA_T *pRecData);
void iisoTpServerMessageHandler(const CO_REC_DATA_T *pRecData);
RET_T iisoTpClientTxAck(COB_REFERENZ_T cobRef, CO_CAN_TR_MSG_T* pMsg);


#endif /* IISO_TP_H */

