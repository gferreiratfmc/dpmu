/*
* ico_cobhandler.h - contains internal defines for cob handling
*
* Copyright (c) 2012-2021 emotas embedded communication GmbH
*
*-------------------------------------------------------------------
* $Id: ico_cobhandler.h 38106 2021-10-04 08:46:00Z boe $
*
*-------------------------------------------------------------------
*
*
*/

/**
* \file
* \brief dataty type header
*/

#ifndef ICO_COB_HANDLER_H
#define ICO_COB_HANDLER_H 1

#include <co_cob.h>

/* datatypes */

typedef enum {
	CO_SERVICE_CANOPEN_START = 1,
	/* start of the real canopen services */
	CO_SERVICE_SDO_SERVER, CO_SERVICE_SDO_CLIENT,
	CO_SERVICE_NMT, CO_SERVICE_ERRCTRL,
	CO_SERVICE_PDO_TRANSMIT, CO_SERVICE_PDO_RECEIVE,
	CO_SERVICE_SYNC_TRANSMIT, CO_SERVICE_SYNC_RECEIVE,
	CO_SERVICE_EMCY_TRANSMIT, CO_SERVICE_EMCY_RECEIVE,
	CO_SERVICE_TIME_TRANSMIT, CO_SERVICE_TIME_RECEIVE,
	CO_SERVICE_LSS_M_TRANSMIT, CO_SERVICE_LSS_M_RECEIVE,
	CO_SERVICE_LSS_S_TRANSMIT, CO_SERVICE_LSS_S_RECEIVE,
	CO_SERVICE_GUARDING,
	CO_SERVICE_FLYMA,
	CO_SERVICE_SRD_TRANSMIT,
	CO_SERVICE_SLEEP,
	CO_SERVICE_GFC_TRANSMIT, CO_SERVICE_GFC_RECEIVE,
	CO_SERVICE_SRDO_TRANSMIT, CO_SERVICE_SRDO_RECEIVE,
	CO_SERVICE_USDO_SERVER, CO_SERVICE_USDO_CLIENT,
	CO_SERVICE_ESDO_SERVER, CO_SERVICE_ESDO_CLIENT,
	CO_SERVICE_IDNEGO_RECEIVE, CO_SERVICE_IDNEGO_TRANSMIT,
	/* end of the real canopen services */	
	CO_SERVICE_CANOPEN_END,
	/* other services and protocols */
	CO_SERVICE_USER, CO_SERVICE_CAN_DEBUG,
	CO_SERVICE_J1939_TRANSMIT, CO_SERVICE_J1939_RECEIVE,
	CO_SERVICE_RAWCAN_TRANSMIT, CO_SERVICE_RAWCAN_RECEIVE,
	CO_SERVICE_ISOTP_CLIENT, CO_SERVICE_ISOTP_SERVER,
	/* last entry */
	CO_SERVICE_LAST
} CO_SERVICE_T;

typedef UNSIGNED16	COB_REFERENZ_T;

typedef enum {
	CO_COB_TYPE_TRANSMIT, CO_COB_TYPE_RECEIVE
}	CO_COB_TYPE_T;


typedef enum {
	CO_COB_RTR_NONE, CO_COB_RTR
} CO_COB_RTR_T;


typedef struct {
	COB_REFERENZ_T	cobNr;			/* cob number */
	CO_COB_TYPE_T	type;			/* cob type */
	CO_CAN_COB_T	canCob;			/* can cob data */
	CO_SERVICE_T	service;		/* service */
	UNSIGNED16		serviceNr;		/* service number */
	UNSIGNED16		inhibit;		/* inhibit time */
	UNSIGNED8		len;			/* msg len */
	CO_TIMER_T		inhibitTimer;	/* inhibit timer */
} CO_COB_T;


/* function prototypes */

COB_REFERENZ_T	icoCobCreate(CO_COB_TYPE_T cobType,
					CO_SERVICE_T service, UNSIGNED16 serviceNr);
RET_T			icoCobSet(COB_REFERENZ_T cobRef, UNSIGNED32 cobId,
					CO_COB_RTR_T rtr, UNSIGNED8	len);
UNSIGNED32		icoCobGet(COB_REFERENZ_T cobRef);
RET_T			icoCobEnable(COB_REFERENZ_T cobRef);
RET_T			icoCobDisable(COB_REFERENZ_T cobRef);
CO_COB_T		*icoCobCheck(const CO_CAN_REC_MSG_T *pRecMsg);
CO_COB_T		*icoCobGetPointer(COB_REFERENZ_T cobRef);
RET_T			icoCobChangeType(COB_REFERENZ_T	cobRef,
					CO_COB_TYPE_T cobType);
RET_T			icoCobChangeService(COB_REFERENZ_T cobRef,
					CO_SERVICE_T service, UNSIGNED16 serviceNr);
RET_T			icoCobSetInhibit(COB_REFERENZ_T	cobRef, UNSIGNED16 inhibit);
RET_T 			icoCobSetClassical(COB_REFERENZ_T cobRef, BOOL_T classical);
RET_T			icoCobSetIgnore(COB_REFERENZ_T	cobRef, UNSIGNED32 mask);
RET_T			icoCobSetLen(COB_REFERENZ_T	cobRef,	UNSIGNED8 len);
void			icoCobDisableAll(CO_SERVICE_T start, CO_SERVICE_T end);
BOOL_T			icoCheckRestrictedCobs(UNSIGNED32 canId,
					UNSIGNED32	exceptFirst, UNSIGNED32	exceptLast);
UNSIGNED8		icoGetCanMsgLen(UNSIGNED8 dlc);
UNSIGNED8		icoGetCanTransferDlc(UNSIGNED8 len);

void			icoCobHandlerVarInit(void);
#endif /* ICO_COB_HANDLER_H */

