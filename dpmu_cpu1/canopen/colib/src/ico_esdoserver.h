/*
* ico_sdoserver.h - contains internal defines for SDO
*
* Copyright (c) 2018-2019 emotas embedded communication GmbH
*
*-------------------------------------------------------------------
* $Id: ico_esdoserver.h 37884 2021-09-17 15:11:22Z phi $

*-------------------------------------------------------------------
*
*
*/

/**
* \file
* \brief dataty type header
*/

#ifndef ICO_ESDO_SERVER_H
#define ICO_ESDO_SERVER_H 1


/* constants */
#define ESDO_MAX_SAVE_DATA	4u


/* datatypes */

typedef enum {
	CO_ESDO_STATE_FREE,
	CO_ESDO_STATE_UPLOAD,
	CO_ESDO_STATE_DOWNLOAD,
	CO_ESDO_STATE_UPLOAD_SEGMENT,
	CO_ESDO_STATE_DOWNLOAD_SEGMENT,
	CO_ESDO_STATE_UPLOAD_BLOCK_INIT,
	CO_ESDO_STATE_UPLOAD_BLOCK,
    CO_ESDO_STATE_UPLOAD_BLOCK_RESP,
    CO_ESDO_STATE_UPLOAD_BLOCK_LAST,
    CO_ESDO_STATE_UPLOAD_BLOCK_END,
    CO_ESDO_STATE_DOWNLOAD_BLOCK,
    CO_ESDO_STATE_DOWNLOAD_BLOCK_END,
	CO_ESDO_STATE_SPLIT_INDICATION,
	CO_ESDO_STATE_WR_SPLIT_INDICATION
} CO_ESDO_STATE_T;

typedef struct co_usdo_server_con_t {
	struct co_usdo_server_con_t* pNext; /* pointer to next active connection */
	struct co_usdo_server_con_t* pPrev; /* pointer to prev active connection */
	
	CO_ESDO_STATE_T	state;			/* sdo state */
	BOOL_T			changed;		/* object was changed */
	
	UNSIGNED16		index;			/* index */
	UNSIGNED8		subIndex;		/* sub index */
	UNSIGNED8		clientNode;		/* node id of client */
    UNSIGNED8		saveData[ESDO_MAX_SAVE_DATA];
	CO_CONST CO_OBJECT_DESC_T *pObjDesc;		/* object description pointer */

	UNSIGNED32		objSize;		/* object size */
	UNSIGNED32		transferedSize;	/* transfered size */

	UNSIGNED16		connRef;
	UNSIGNED8		segCnt;
	UNSIGNED8		toggle;
	UNSIGNED8       seqNr;            /* sequence number */
    UNSIGNED8       blockSize;        /* max number of blocks for one transfer */
    UNSIGNED8       blockSaveData[7]; /* save data for last block */
    CO_EVENT_T      blockEvent;        /* event structure */
# ifdef CO_EVENT_DYNAMIC_SESDO_DOMAIN_WRITE
	BOOL_T			domainTransfer;	/* object with domain transfer */
	UNSIGNED32		domainTransferedSize;	/* overall transfered size */
# endif /* CO_EVENT_DYNAMIC_SESDO_DOMAIN_WRITE */
} CO_ESDO_SERVER_CON_T;






/* function prototypes */

void icoEsdoServerVarInit(void);

void icoEsdoServerAbort(CO_ESDO_SERVER_CON_T *pEsdo, RET_T errorReason, BOOL_T fromClient);

RET_T icoEsdoCheckUserReadInd(UNSIGNED8 client, UNSIGNED16 index, UNSIGNED8 subIndex);
RET_T icoEsdoCheckUserCheckWriteInd(UNSIGNED8 client, UNSIGNED16 index, UNSIGNED8 subIndex, const UNSIGNED8	*pData);
RET_T icoEsdoCheckUserWriteInd(const CO_ESDO_SERVER_CON_T *pEsdo);

void icoEsdoServerHandler(const CO_REC_DATA_T *pRecData);


void icoEsdoServerReset(void);	

#endif /* ICO_ESDO_SERVER_H */
