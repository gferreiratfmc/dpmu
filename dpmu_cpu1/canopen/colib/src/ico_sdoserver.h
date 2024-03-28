/*
* ico_sdoserver.h - contains internal defines for SDO
*
* Copyright (c) 2013-2022 emotas embedded communication GmbH
*
*-------------------------------------------------------------------
* $Id: ico_sdoserver.h 40827 2022-05-30 12:59:49Z boe $

*-------------------------------------------------------------------
*
*
*/

/**
* \file
* \brief dataty type header
*/

#ifndef ICO_SDO_SERVER_H
#define ICO_SDO_SERVER_H 1

/* ensure, ico_indication.h is inluded before */
#ifndef ICO_INDICATION_H
#warning "please include ico_indication.h before..."
#endif /* ICO_INDICATION_H */

/* constants */
#define MAX_SAVE_DATA	4u

/* SDO Server Read */
#ifdef CO_EVENT_DYNAMIC_SDO_SERVER_READ
# ifdef CO_EVENT_PROFILE_SDO_SERVER_READ
#  define CO_EVENT_SDO_SERVER_READ_CNT	(CO_EVENT_DYNAMIC_SDO_SERVER_READ + CO_EVENT_PROFILE_SDO_SERVER_READ)
# else /* CO_EVENT_PROFILE_SDO_SERVER_READ */
#  define CO_EVENT_SDO_SERVER_READ_CNT	(CO_EVENT_DYNAMIC_SDO_SERVER_READ)
# endif /* CO_EVENT_PROFILE_SDO_SERVER_READ */
#else /* CO_EVENT_DYNAMIC_SDO_SERVER_READ */
# ifdef CO_EVENT_PROFILE_SDO_SERVER_READ
#  define CO_EVENT_SDO_SERVER_READ_CNT	(CO_EVENT_PROFILE_SDO_SERVER_READ)
# endif /* CO_EVENT_PROFILE_SDO_SERVER_READ */
#endif /* CO_EVENT_DYNAMIC_SDO_SERVER_READ */

#if defined(CO_EVENT_STATIC_SDO_SERVER_READ) || defined(CO_EVENT_SDO_SERVER_READ_CNT)
# define CO_EVENT_SDO_SERVER_READ   1u
#endif /* defined(CO_EVENT_STATIC_SDO_SERVER_READ) || defined(CO_EVENT_SDO_SERVER_READ_CNT) */


/* SDO Server Write */
#ifdef CO_EVENT_DYNAMIC_SDO_SERVER_WRITE
# ifdef CO_EVENT_PROFILE_SDO_SERVER_WRITE
#  define CO_EVENT_SDO_SERVER_WRITE_CNT	(CO_EVENT_DYNAMIC_SDO_SERVER_WRITE + CO_EVENT_PROFILE_SDO_SERVER_WRITE)
# else /* CO_EVENT_PROFILE_SDO_SERVER_WRITE */
#  define CO_EVENT_SDO_SERVER_WRITE_CNT	(CO_EVENT_DYNAMIC_SDO_SERVER_WRITE)
# endif /* CO_EVENT_PROFILE_SDO_SERVER_WRITE */
#else /* CO_EVENT_DYNAMIC_SDO_SERVER_WRITE */
# ifdef CO_EVENT_PROFILE_SDO_SERVER_WRITE
#  define CO_EVENT_SDO_SERVER_WRITE_CNT	(CO_EVENT_PROFILE_SDO_SERVER_WRITE)
# endif /* CO_EVENT_PROFILE_SDO_SERVER_WRITE */
#endif /* CO_EVENT_DYNAMIC_SDO_SERVER_WRITE */


/* SDO Server Check Write */
#ifdef CO_EVENT_DYNAMIC_SDO_SERVER_CHECK_WRITE
# ifdef CO_EVENT_PROFILE_SDO_SERVER_CHECK_WRITE
#  define CO_EVENT_SDO_SERVER_CHECK_WRITE_CNT	(CO_EVENT_DYNAMIC_SDO_SERVER_CHECK_WRITE + CO_EVENT_PROFILE_SDO_SERVER_CHECK_WRITE)
# else /* CO_EVENT_PROFILE_SDO_SERVER_CHECK_WRITE */
#  define CO_EVENT_SDO_SERVER_CHECK_WRITE_CNT	(CO_EVENT_DYNAMIC_SDO_SERVER_CHECK_WRITE)
# endif /* CO_EVENT_PROFILE_SDO_SERVER_CHECK_WRITE */
#else /* CO_EVENT_DYNAMIC_SDO_SERVER_CHECK_WRITE */
# ifdef CO_EVENT_PROFILE_SDO_SERVER_CHECK_WRITE
#  define CO_EVENT_SDO_SERVER_CHECK_WRITE_CNT	(CO_EVENT_PROFILE_SDO_SERVER_CHECK_WRITE)
# endif /* CO_EVENT_PROFILE_SDO_SERVER_CHECK_WRITE */
#endif /* CO_EVENT_DYNAMIC_SDO_SERVER_CHECK_WRITE */


/* SDO Server Domain Write */
#ifdef CO_EVENT_DYNAMIC_SSDO_DOMAIN_WRITE
# ifdef CO_EVENT_PROFILE_SSDO_DOMAIN_WRITE
#  define CO_EVENT_SSDO_DOMAIN_WRITE_CNT	(CO_EVENT_DYNAMIC_SSDO_DOMAIN_WRITE + CO_EVENT_PROFILE_SSDO_DOMAIN_WRITE)
# else /* CO_EVENT_PROFILE_SSDO_DOMAIN_WRITE */
#  define CO_EVENT_SSDO_DOMAIN_WRITE_CNT	(CO_EVENT_DYNAMIC_SSDO_DOMAIN_WRITE)
# endif /* CO_EVENT_PROFILE_SSDO_DOMAIN_WRITE */
#else /* CO_EVENT_DYNAMIC_SSDO_DOMAIN_WRITE */
# ifdef CO_EVENT_PROFILE_SSDO_DOMAIN_WRITE
#  define CO_EVENT_SSDO_DOMAIN_WRITE_CNT	(CO_EVENT_PROFILE_SSDO_DOMAIN_WRITE)
# endif /* CO_EVENT_PROFILE_SSDO_DOMAIN_WRITE */
#endif /* CO_EVENT_DYNAMIC_SSDO_DOMAIN_WRITE */


/* SDO Server Domain Read */
#ifdef CO_EVENT_DYNAMIC_SSDO_DOMAIN_READ
# ifdef CO_EVENT_PROFILE_SSDO_DOMAIN_READ
#  define CO_EVENT_SSDO_DOMAIN_READ_CNT    (CO_EVENT_DYNAMIC_SSDO_DOMAIN_READ + CO_EVENT_PROFILE_SSDO_DOMAIN_READ)
# else /* CO_EVENT_PROFILE_SSDO_DOMAIN_READ */
#  define CO_EVENT_SSDO_DOMAIN_READ_CNT    (CO_EVENT_DYNAMIC_SSDO_DOMAIN_READ)
# endif /* CO_EVENT_PROFILE_SSDO_DOMAIN_READ */
#else /* CO_EVENT_DYNAMIC_SSDO_DOMAIN_READ */
# ifdef CO_EVENT_PROFILE_SSDO_DOMAIN_READ
#  define CO_EVENT_SSDO_DOMAIN_READ_CNT    (CO_EVENT_PROFILE_SSDO_DOMAIN_READ)
# endif /* CO_EVENT_PROFILE_SSDO_DOMAIN_READ */
#endif /* CO_EVENT_DYNAMIC_SSDO_DOMAIN_READ */


/* datatypes */

typedef enum {
	CO_SDO_STATE_FREE,
	CO_SDO_STATE_UPLOAD_INIT,
	CO_SDO_STATE_UPLOAD_SEGMENT,
/*	CO_SDO_STATE_DOWNLOAD_INIT, */
	CO_SDO_STATE_DOWNLOAD_SEGMENT,
	CO_SDO_STATE_BLOCK_UPLOAD_INIT,
	CO_SDO_STATE_BLOCK_UPLOAD,
	CO_SDO_STATE_BLOCK_UPLOAD_RESP,
	CO_SDO_STATE_BLOCK_UPLOAD_LAST,
	CO_SDO_STATE_BLOCK_UPLOAD_END,
	CO_SDO_STATE_BLOCK_DOWNLOAD,
	CO_SDO_STATE_BLOCK_DOWNLOAD_END,
	CO_SDO_STATE_FD_UPLOAD_SEGMENT,
	CO_SDO_STATE_FD_DOWNLOAD_SEGMENT,
	CO_SDO_STATE_NETWORK_IND,
	CO_SDO_STATE_WR_SPLIT_INDICATION,
	CO_SDO_STATE_WR_SEG_SPLIT_INDICATION,
	CO_SDO_STATE_WR_BLOCK_SPLIT_INDICATION
} CO_SDO_STATE_T;

typedef struct co_sdo_server_t {
	CO_CONST CO_OBJECT_DESC_T *pObjDesc;		/* object description pointer */
	UNSIGNED32		trCobId;		/* cob id */
	UNSIGNED32		recCobId;		/* cob id */
	UNSIGNED32		objSize;		/* object size */
	UNSIGNED32		transferedSize;	/* transfered size */
	UNSIGNED16		index;			/* index */
	UNSIGNED8		saveData[MAX_SAVE_DATA];
	UNSIGNED8		sdoNr;			/* sdo number */
	UNSIGNED8		subIndex;		/* sub index */
	UNSIGNED8		node;			/* subindex 3 */
	UNSIGNED8		toggle;
	COB_REFERENZ_T	trCob;
	COB_REFERENZ_T	recCob;
	CO_SDO_STATE_T	state;			/* sdo state */
	BOOL_T			changed;		/* object was changed */
# ifdef CO_SDO_BLOCK
	UNSIGNED8		seqNr;			/* sequence number */
	UNSIGNED8		blockSize;		/* max number of blocks for one transfer */
	BOOL_T			blockCrcUsed;	/* use CRC */
	UNSIGNED16		blockCrc;		/* CRC */
	UNSIGNED32		blockCrcSize;	/* size of calculated crc sum */
	CO_EVENT_T		blockEvent;		/* event structure */
	UNSIGNED8		blockSaveData[7]; /* save data for last block */
# endif /* CO_SDO_BLOCK */
#if defined(CO_EVENT_SSDO_DOMAIN_WRITE_CNT) || defined(CO_EVENT_SSDO_DOMAIN_READ_CNT)
	BOOL_T			domainTransfer;	/* object with domain transfer */
	UNSIGNED32		domainTransferedSize;	/* overall transfered size */
#endif /* CO_EVENT_SSDO_DOMAIN_WRITE_CNT */
#ifdef CO_SDO_SPLIT_INDICATION
	BOOL_T			split;
#endif /* CO_SDO_SPLIT_INDICATION */
# ifdef CO_SDO_NETWORKING
	UNSIGNED8		networkCanLine;
	UNSIGNED8		clientSdoNr;
	UNSIGNED8		localRequest;
	struct co_sdo_server_t *pLocalReqSdo;
# endif /* CO_SDO_NETWORKING */
} CO_SDO_SERVER_T;


/* function prototypes */

void	icoSdoServerVarInit(CO_CONST UNSIGNED8 *pList);
void	icoSdoServerReset(void);
void	icoSdoServerSetDefaultValue(void);
void	icoSdoDeCodeMultiplexer(CO_CONST UNSIGNED8 pMp[],
			CO_SDO_SERVER_T *pSdo);
RET_T	icoSdoCheckUserWriteInd(const CO_SDO_SERVER_T *pSdo);
RET_T	icoSdoCheckUserReadInd(UNSIGNED8 sdoNr, UNSIGNED16 index,
			UNSIGNED8 subIndex);
RET_T	icoSdoCheckUserCheckWriteInd(UNSIGNED8 sdoNr,
			UNSIGNED16 index, UNSIGNED8 subIndex, const UNSIGNED8 *pData,
			UNSIGNED32 size);
RET_T	icoSdoDomainUserWriteInd(const CO_SDO_SERVER_T *pSdo);
RET_T	icoSdoDomainUserReadInd(const CO_SDO_SERVER_T *pSdo);
void	icoSdoServerAbort(CO_SDO_SERVER_T *pSdo,
			RET_T errorReason, BOOL_T fromClient);

# ifdef CO_SDO_BLOCK
RET_T	icoSdoServerBlockReadInit(CO_SDO_SERVER_T	*pSdo,
			const CO_CAN_REC_MSG_T	*pRecData);
RET_T	icoSdoServerBlockRead(CO_SDO_SERVER_T	*pSdo);
RET_T	icoSdoServerBlockReadCon(CO_SDO_SERVER_T	*pSdo,
			const CO_CAN_REC_MSG_T	*pRecData);
RET_T	icoSdoServerBlockWriteInit(CO_SDO_SERVER_T *pSdo,
			const CO_CAN_REC_MSG_T	*pRecData);
RET_T	icoSdoServerBlockWrite(CO_SDO_SERVER_T *pSdo,
			const CO_CAN_REC_MSG_T *pRecData);
RET_T	icoSdoServerBlockWriteEnd(CO_SDO_SERVER_T *pSdo,
			const CO_CAN_REC_MSG_T	*pRecData);
RET_T	icoSdoServerBlockWriteConfirmation(CO_SDO_SERVER_T *pSdo);
# endif /* CO_SDO_BLOCK */

#ifdef CO_SDO_NETWORKING
RET_T	icoSdoServerNetworkReq(CO_SDO_SERVER_T *pSdo, const CO_CAN_REC_MSG_T *pRecData);
void	icoSdoServerNetwork(CO_SDO_SERVER_T *pSdo, const CO_CAN_REC_MSG_T *pRecData);
RET_T	icoSdoServerLocalResp(	CO_SDO_SERVER_T	*pSdo,
			const UNSIGNED8	*pTrData);
void	icoSdoServerHandler(const CO_REC_DATA_T *pRecData,
			CO_SDO_SERVER_T *pLocalReqSdo);
#else /* CO_SDO_NETWORKING */
void	icoSdoServerHandler(const CO_REC_DATA_T *pRecData);
#endif /* CO_SDO_NETWORKING */

CO_SDO_SERVER_T	*icoSdoServerPtr(UNSIGNED16 sdoNr);
void	*icoSdoGetObjectAddr(UNSIGNED16 sdoNr, UNSIGNED8 subIndex);
RET_T	icoSdoCheckObjLimitNode(UNSIGNED16 sdoNr);
RET_T	icoSdoCheckObjLimitCobId(UNSIGNED16 sdoNr,
			UNSIGNED8 subIndex, UNSIGNED32 canId);
RET_T	icoSdoObjChanged(UNSIGNED16 sdoNr, UNSIGNED8 subIndex);


#endif /* ICO_SDO_SERVER_H */
