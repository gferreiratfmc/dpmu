/*
* ico_esdoclient.h - contains internal defines for SDO Client
*
* Copyright (c) 2018-2019 emotas embedded communication GmbH
*
*-------------------------------------------------------------------
* $Id: ico_esdoclient.h 40554 2022-04-22 10:49:00Z ged $

*-------------------------------------------------------------------
*
*
*/

/**
* \file
* \brief dataty type header
*/

#ifndef ICO_ESDO_CLIENT_H
#define ICO_ESDO_CLIENT_H 1


/* constants */

/* datatypes */
typedef enum {
	CO_ESDO_C_NODE_FREE,
	CO_ESDO_C_NODE_RESPONSE,
	CO_ESDO_C_NODE_WAIT,
	CO_ESDO_C_NODE_FAILED
}CO_ESDO_CLIENT_NODE_STATE;

typedef enum {
	CO_ESDO_CLIENT_STATE_FREE,
	CO_ESDO_CLIENT_STATE_UPLOAD_INIT,
	CO_ESDO_CLIENT_STATE_UPLOAD_SEGMENT,
	CO_ESDO_CLIENT_STATE_DOWNLOAD_INIT,
	CO_ESDO_CLIENT_STATE_DOWNLOAD_SEGMENT,
	CO_ESDO_CLIENT_STATE_BLK_UL_INIT,
	CO_ESDO_CLIENT_STATE_BLK_UL_BLK,
	CO_ESDO_CLIENT_STATE_BLK_UL_END,
	CO_ESDO_CLIENT_STATE_BLK_DL_INIT,
	CO_ESDO_CLIENT_STATE_BLK_DL,
	CO_ESDO_CLIENT_STATE_BLK_DL_ACQ,
	CO_ESDO_CLIENT_STATE_BLK_DL_END,
	CO_ESDO_CLIENT_STATE_NETWORK_IND,
	CO_ESDO_CLIENT_STATE_NETWORK_READ_REQ,
	CO_ESDO_CLIENT_STATE_NETWORK_WRITE_REQ
} CO_ESDO_CLIENT_STATE_T;

typedef struct {
	UNSIGNED8		nodeId;			/* nodeId */
	CO_ESDO_CLIENT_STATE_T	state;	/* sdo state */
	UNSIGNED16		index;			/* index */
	UNSIGNED8		subIndex;		/* sub index */
	UNSIGNED8		node;			/* subindex 3 */
	UNSIGNED32		trCobId;
	UNSIGNED32		recCobId;
	UNSIGNED8		*pData;
	UNSIGNED32		size;
	UNSIGNED32		restSize;
	UNSIGNED8		toggle;
	UNSIGNED32		timeOutVal;
	CO_TIMER_T		timer;
	CO_TIMER_T		waitTimer;
	UNSIGNED16		numeric;
	BOOL_T			domain;			/* domain transfer */
	UNSIGNED8		blockSize;
	UNSIGNED8		seqNr;
	UNSIGNED8		saveData[7];
	CO_EVENT_T		blockEvent;
	UNSIGNED8		responseNodeId;		/* we only look for the 1st response node id */
#ifdef CO_EVENT_CSDO_DOMAIN_WRITE
	CO_EVENT_SDO_CLIENT_DOMAIN_WRITE_T pFunction;
	void			*pFctData;
	UNSIGNED32		msgCnt;
	BOOL_T			split;
#endif /* CO_EVENT_CSDO_DOMAIN_WRITE */
} CO_ESDO_CLIENT_T;


/* function prototypes */

void	icoSdoClientAbort(CO_ESDO_CLIENT_T *pSdo, RET_T errorReason);
void	icoSdoClientTimeOut(void *pData);
void	icoSdoClientUserInd(const CO_ESDO_CLIENT_T	*pSdo,
			CO_ESDO_CLIENT_STATE_T	state, UNSIGNED32 result);
void	icoSdoClientVarInit(CO_CONST UNSIGNED8 *pList);
void	icoSdoQueueVarInit(void);

UNSIGNED8 icoSdoClientBusy(UNSIGNED8 sdoNr);

void	icoSdoClientReadBlockInit(CO_ESDO_CLIENT_T *pSdo,
			const CO_CAN_REC_MSG_T	*pRecData);
void	icoSdoClientReadBlock(CO_ESDO_CLIENT_T	*pSdo,
			const CO_CAN_REC_MSG_T	*pRecData);
void	icoSdoClientReadBlockEnd(CO_ESDO_CLIENT_T *pSdo,
			const CO_CAN_REC_MSG_T	*pRecData);
void	icoSdoClientWriteBlockInit(CO_ESDO_CLIENT_T *pSdo,
			const CO_CAN_REC_MSG_T	*pRecData);
void	icoSdoClientWriteBlockAcq(CO_ESDO_CLIENT_T *pSdo,
			const CO_CAN_REC_MSG_T	*pRecData);

CO_ESDO_CLIENT_T	*icoSdoClientPtr(UNSIGNED8 sdoNr);

RET_T icoSdoClientSetCobId(UNSIGNED16	index, UNSIGNED8 subIndex, 
		UNSIGNED32* pCobId, BOOL_T* pChanged);


#endif /* ICO_ESDO_CLIENT_H */
