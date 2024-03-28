/*
* ico_sdoclient.h - contains internal defines for SDO Client
*
* Copyright (c) 2013-2022 emotas embedded communication GmbH
*
*-------------------------------------------------------------------
* $Id: ico_sdoclient.h 39658 2022-02-17 10:15:29Z boe $

*-------------------------------------------------------------------
*
*
*/

/**
* \file
* \brief dataty type header
*/

#ifndef ICO_SDO_CLIENT_H
#define ICO_SDO_CLIENT_H 1


/* constants */

/* datatypes */

typedef enum {
	CO_SDO_CLIENT_STATE_FREE,
	CO_SDO_CLIENT_STATE_UPLOAD_INIT,
	CO_SDO_CLIENT_STATE_UPLOAD_SEGMENT,
	CO_SDO_CLIENT_STATE_DOWNLOAD_INIT,
	CO_SDO_CLIENT_STATE_DOWNLOAD_SEGMENT,
	CO_SDO_CLIENT_STATE_BLK_UL_INIT,
	CO_SDO_CLIENT_STATE_BLK_UL_BLK,
	CO_SDO_CLIENT_STATE_BLK_UL_END,
	CO_SDO_CLIENT_STATE_BLK_DL_INIT,
	CO_SDO_CLIENT_STATE_BLK_DL,
	CO_SDO_CLIENT_STATE_BLK_DL_ACQ,
	CO_SDO_CLIENT_STATE_BLK_DL_END,
	CO_SDO_CLIENT_STATE_NETWORK_IND,
	CO_SDO_CLIENT_STATE_NETWORK_READ_REQ,
	CO_SDO_CLIENT_STATE_NETWORK_WRITE_REQ
} CO_SDO_CLIENT_STATE_T;

typedef struct {
	UNSIGNED8		sdoNr;			/* sdo number */
	CO_SDO_CLIENT_STATE_T	state;	/* sdo state */
	UNSIGNED16		index;			/* index */
	UNSIGNED8		subIndex;		/* sub index */
	UNSIGNED8		node;			/* subindex 3 */
	COB_REFERENZ_T	trCob;
	COB_REFERENZ_T	recCob;
	UNSIGNED32		trCobId;
	UNSIGNED32		recCobId;
	UNSIGNED8		*pData;
	UNSIGNED32		size;
	UNSIGNED32		restSize;
	UNSIGNED8		toggle;
	UNSIGNED32		timeOutVal;
	CO_TIMER_T		timer;
	UNSIGNED16		numeric;
	BOOL_T			domain;			/* domain transfer */
#ifdef CO_SDO_BLOCK
	UNSIGNED8		blockSize;
	UNSIGNED8		seqNr;
	UNSIGNED8		saveBlockData[7];
	CO_EVENT_T		blockEvent;
# ifdef CO_SDO_BLOCK_CRC
	UNSIGNED16		blockCrc;
	BOOL_T			blockCrcUsed;
# endif /* CO_SDO_BLOCK_CRC */
#endif /* CO_SDO_BLOCK */
#ifdef CO_SDO_NETWORKING
	UNSIGNED16		networkNr;
	UNSIGNED8		networkNode;
	UNSIGNED8		networkCanLine;
	UNSIGNED8		serverSdoNr;
	UNSIGNED8		cmdByte;
	UNSIGNED8		lastCmdByte;
#endif /* CO_SDO_NETWORKING */
#ifdef CO_EVENT_CSDO_DOMAIN_WRITE
	CO_EVENT_SDO_CLIENT_DOMAIN_WRITE_T pFunction;
	void			*pFctData;
	UNSIGNED32		msgCnt;
	BOOL_T			split;
# ifdef CO_SDO_CLIENT_DOMAIN_SPLIT_INDICATION
	UNSIGNED8		saveData[8];
	BOOL_T			splitIndication;
# endif /* CO_SDO_CLIENT_DOMAIN_SPLIT_INDICATION */
#endif /* CO_EVENT_CSDO_DOMAIN_WRITE */
} CO_SDO_CLIENT_T;


/* function prototypes */

void	icoSdoClientAbort(CO_SDO_CLIENT_T *pSdo, RET_T errorReason);
void	icoSdoClientTimeOut(void *pData);
void	icoSdoClientUserInd(const CO_SDO_CLIENT_T	*pSdo,
			CO_SDO_CLIENT_STATE_T	state, UNSIGNED32 result);
void	icoSdoClientVarInit(CO_CONST UNSIGNED8 *pList);
void	icoSdoQueueVarInit(void);

UNSIGNED8 icoSdoClientBusy(UNSIGNED8 sdoNr);

# ifdef CO_SDO_BLOCK
void	icoSdoClientReadBlockInit(CO_SDO_CLIENT_T *pSdo,
			const CO_CAN_REC_MSG_T	*pRecData);
void	icoSdoClientReadBlock(CO_SDO_CLIENT_T	*pSdo,
			const CO_CAN_REC_MSG_T	*pRecData);
void	icoSdoClientReadBlockEnd(CO_SDO_CLIENT_T *pSdo,
			const CO_CAN_REC_MSG_T	*pRecData);
void	icoSdoClientWriteBlockInit(CO_SDO_CLIENT_T *pSdo,
			const CO_CAN_REC_MSG_T	*pRecData);
void	icoSdoClientWriteBlockAcq(CO_SDO_CLIENT_T *pSdo,
			const CO_CAN_REC_MSG_T	*pRecData);
void	icoSdoClientDomainWriteBlockIndCont(CO_SDO_CLIENT_T *pSdo);
# endif /* CO_SDO_BLOCK */

CO_SDO_CLIENT_T	*icoSdoClientPtr(UNSIGNED8 sdoNr);

RET_T	icoSdoClientSetCobId(UNSIGNED16 index, UNSIGNED8 subIndex,
		UNSIGNED32* pCobId, BOOL_T* pChanged);

void	icoSdoClientReset(void);
void	icoSdoClientSetDefaultValue(void);

void	icoSdoClientHandler(const CO_REC_DATA_T *pRecData);
void	*icoSdoClientGetObjectAddr(UNSIGNED8 sdoNr,
			UNSIGNED8 subIndex);
RET_T	icoSdoClientCheckObjLimitNode(UNSIGNED16 sdoNr);
RET_T	icoSdoClientCheckObjLimitCobId(UNSIGNED16 sdoNr,
			UNSIGNED8 subIndex, UNSIGNED32 canId);
RET_T	icoSdoClientObjChanged(UNSIGNED16 sdoNr,
			UNSIGNED8 subIndex);

# ifdef CO_SDO_NETWORKING
void icoSdoClientNetworkResp(CO_SDO_CLIENT_T	*pSdo,
		const CO_CAN_REC_MSG_T *pRecData);
void icoSdoClientNetworkCon(CO_SDO_CLIENT_T	*pSdo,
		const CO_CAN_REC_MSG_T *pRecData);
# endif /* CO_SDO_NETWORKING */

void	icoSdoClientQueueInd(UNSIGNED8 sdoNr, UNSIGNED16 index,
			UNSIGNED8 subIndex, UNSIGNED32 size, UNSIGNED32	result);

#endif /* ICO_SDO_CLIENT_H */
