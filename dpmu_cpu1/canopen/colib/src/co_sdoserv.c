/*
* co_sdoserv.c - contains sdo server routines
*
* Copyright (c) 2012-2022 emotas embedded communication GmbH
*-------------------------------------------------------------------------------
* $Id: co_sdoserv.c 40827 2022-05-30 12:59:49Z boe $
*
*
*-------------------------------------------------------------------------------
*
*
*/

/******************************************************************************/
/**
* \brief SDO server routines
*
* \file co_sdoserv.c
* contains sdo server routines
*
*/

/* header of standard C - libraries
------------------------------------------------------------------------------*/
#include <string.h>

/* header of project specific types
------------------------------------------------------------------------------*/
#include <gen_define.h>

#ifdef CO_SDO_SERVER_CNT
#include <co_odaccess.h>
#include <co_drv.h>
#include <co_timer.h>
#include <co_nmt.h>
#include <co_sdo.h>
#include "ico_indication.h"
#include "ico_cobhandler.h"
#include "ico_queue.h"
#include "ico_odaccess.h"
#include "ico_nmt.h"
#include "ico_event.h"
#include "ico_sdoserver.h"
#include "ico_sdo.h"
#ifdef CO_SDO_NETWORKING
# include "ico_network.h"
#endif /* CO_SDO_NETWORKING */

/* constant definitions
------------------------------------------------------------------------------*/
typedef struct {
	RET_T	reason;			/* error reason */
	UNSIGNED32	abortCode;	/* sdo abort code */
} SDO_ABORT_CODE_TABLE_T;

static const SDO_ABORT_CODE_TABLE_T	abortCodeTable[] = {
	{ RET_SERVICE_BUSY,					0x05000000uL	},
	{ RET_TOGGLE_MISMATCH,				0x05030000uL	},
	{ RET_SDO_TIMEOUT,					0x05040000UL	},
	{ RET_SDO_UNKNOWN_CCS,				0x05040001uL	},
	{ RET_SDO_WRONG_BLOCKSIZE,			0x05040002uL	},
	{ RET_SDO_WRONG_SEQ_NR,				0x05040003uL	},
	{ RET_SDO_CRC_ERROR,				0x05040004uL	},
	{ RET_OUT_OF_MEMORY,				0x05040005uL	},
	{ RET_SDO_TRANSFER_NOT_SUPPORTED,	0x06010000uL	},
	{ RET_NO_READ_PERM,					0x06010001uL	},
	{ RET_NO_WRITE_PERM,				0x06010002uL	},
	{ RET_IDX_NOT_FOUND,				0x06020000uL	},
	{ RET_OD_ACCESS_ERROR,				0x06040047uL	},
	{ RET_HARDWARE_ERROR,				0x06060000uL	},
	{ RET_SDO_DATA_TYPE_NOT_MATCH,		0x06070010uL	},
	{ RET_SUBIDX_NOT_FOUND,				0x06090011uL	},
	{ RET_SDO_INVALID_VALUE,			0x06090030uL	},
	{ RET_MAP_ERROR,					0x06040041uL	},
	{ RET_MAP_LEN_ERROR,				0x06040042uL	},
	{ RET_PARAMETER_INCOMPATIBLE,		0x06040043uL	},
	{ RET_ERROR_STORE,					0x08000020uL	},
	{ RET_ERROR_STORE_LOCAL,			0x08000021uL	},
	{ RET_ERROR_PRESENT_DEVICE_STATE,	0x08000022uL	},
	{ RET_VALUE_NOT_AVAILABLE,			0x08000024uL	},
	{ RET_NETWORK_ID_UNKNOWN,			0x0a000001uL	},
	{ RET_NW_NODE_ID_UNKNOWN,			0x0a000002uL	},
	{ RET_NW_SDO_CHANNEL_IN_USE,		0x0a000003uL	},
	{ RET_SDO_SPLIT_INDICATION,			0x08000000uL	},
};

/* local defined data types
------------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
------------------------------------------------------------------------------*/
#ifdef CO_EVENT_STATIC_SDO_SERVER_READ
extern CO_CONST CO_EVENT_SDO_SERVER_T coEventSdoServerReadInd;
#endif /* CO_EVENT_STATIC_SDO_SERVER_READ */

#ifdef CO_EVENT_STATIC_SDO_SERVER_CHECK_WRITE
extern CO_CONST CO_EVENT_SDO_SERVER_CHECK_WRITE_T coEventSdoServerCheckWriteInd;
#endif /* CO_EVENT_STATIC_SDO_SERVER_CHECK_WRITE */

#ifdef CO_EVENT_STATIC_SDO_SERVER_WRITE
extern CO_CONST CO_EVENT_SDO_SERVER_T coEventSdoServerWriteInd;
#endif /* CO_EVENT_STATIC_SDO_SERVER_WRITE */

#ifdef CO_EVENT_STATIC_SSDO_DOMAIN_WRITE
extern CO_CONST CO_EVENT_SDO_SERVER_DOMAIN_WRITE_T coEventSdoServerDomainWriteInd;
#endif /* CO_EVENT_STATIC_SSDO_DOMAIN_WRITE */

#ifdef CO_EVENT_STATIC_SSDO_DOMAIN_READ
extern CO_CONST CO_EVENT_SDO_SERVER_DOMAIN_READ_T coEventSdoServerDomainReadInd;
#endif /* CO_EVENT_STATIC_SSDO_DOMAIN_READ */


/* list of global defined functions
------------------------------------------------------------------------------*/

/* list of local defined functions
------------------------------------------------------------------------------*/
static CO_INLINE void sdoCodeMultiplexer(CO_CONST CO_SDO_SERVER_T *pSdo,
		UNSIGNED8 pMp[]);
static CO_INLINE CO_SDO_SERVER_T *getSdoServer(UNSIGNED16	sdoIdx);

static RET_T sdoServerReadInit(CO_SDO_SERVER_T *pSdo,
		const CO_CAN_REC_MSG_T *pRecData);
static RET_T sdoServerReadSegment(CO_SDO_SERVER_T *pSdo,
		CO_CONST CO_CAN_REC_MSG_T *pRecData);
static RET_T sdoServerWriteInit(CO_SDO_SERVER_T *pSdo,
		const CO_CAN_REC_MSG_T *pRecData);
static RET_T sdoServerWriteSegment(CO_SDO_SERVER_T *pSdo,
		const CO_CAN_REC_MSG_T *pRecData);
static RET_T sdoServerWriteIndCont(CO_SDO_SERVER_T *pSdo);
static RET_T sdoServerReadIndCont(CO_SDO_SERVER_T	*pSdo);


/* external variables
------------------------------------------------------------------------------*/

/* global variables
------------------------------------------------------------------------------*/

/* local defined variables
------------------------------------------------------------------------------*/
static UNSIGNED8	sdoServerCnt = { 0u };
static CO_SDO_SERVER_T	sdoServer[CO_SDO_SERVER_CNT];
#ifdef CO_EVENT_SDO_SERVER_READ_CNT
static UNSIGNED8	sdoServerReadTableCnt = 0u;
static CO_EVENT_SDO_SERVER_T	sdoServerReadTable[CO_EVENT_SDO_SERVER_READ_CNT];
#endif /* CO_EVENT_SDO_SERVER_READ_CNT */
#ifdef CO_EVENT_SDO_SERVER_WRITE_CNT
static CO_EVENT_SDO_SERVER_T	sdoServerWriteTable[CO_EVENT_SDO_SERVER_WRITE_CNT];
static UNSIGNED8	sdoServerWriteTableCnt = 0u;
#endif /* CO_EVENT_SDO_SERVER_WRITE_CNT */
#ifdef CO_EVENT_SDO_SERVER_CHECK_WRITE_CNT
static UNSIGNED8	sdoServerCheckWriteTableCnt = 0u;
static CO_EVENT_SDO_SERVER_CHECK_WRITE_T	sdoServerCheckWriteTable[CO_EVENT_SDO_SERVER_CHECK_WRITE_CNT];
#endif /* CO_EVENT_SDO_SERVER_CHECK_WRITE_CNT */
#ifdef CO_EVENT_SSDO_DOMAIN_WRITE_CNT
static UNSIGNED8	sdoServerDomainWriteTableCnt = 0u;
static CO_EVENT_SDO_SERVER_DOMAIN_WRITE_T	sdoServerDomainWriteTable[CO_EVENT_SSDO_DOMAIN_WRITE_CNT];
#endif /* CO_EVENT_SSDO_DOMAIN_WRITE_CNT */
#ifdef CO_EVENT_SSDO_DOMAIN_READ_CNT
static UNSIGNED8    sdoServerDomainReadTableCnt = 0u;
static CO_EVENT_SDO_SERVER_DOMAIN_READ_T    sdoServerDomainReadTable[CO_EVENT_SSDO_DOMAIN_READ_CNT];
#endif /* CO_EVENT_SSDO_DOMAIN_READ_CNT */


/******************************************************************************/
/**
* \internal
*
* \brief sdoCodeMultiplexer - write multiplexer into message format
*
*
*/
static CO_INLINE void sdoCodeMultiplexer(
		CO_CONST CO_SDO_SERVER_T *pSdo,		/* pointer to sdo */
		UNSIGNED8	pMp[]					/* pointer to multiplexer */
	)
{
	pMp[0] = (UNSIGNED8)(pSdo->index & 0xffu);
	pMp[1] = (UNSIGNED8)(pSdo->index >> 8u);
	pMp[2] = pSdo->subIndex;
}


/******************************************************************************/
/**
* \internal
*
* \brief icoSdoDeCodeMultiplexer - write multiplexer into message format
*
*
*/
CO_INLINE void icoSdoDeCodeMultiplexer(
		CO_CONST UNSIGNED8	pMp[],	/* pointer to multiplexer */
		CO_SDO_SERVER_T	*pSdo		/* pointer to sdo */
	)
{
	pSdo->index = (UNSIGNED16)(((UNSIGNED16)(pMp[1])) << 8u) | (UNSIGNED16)pMp[0];
	pSdo->subIndex = pMp[2];
}


/******************************************************************************/
/**
* \internal
*
* \brief icoSdoServerHandler - sdo server handler
*
*
*/
void icoSdoServerHandler(
		const CO_REC_DATA_T	*pRecData		/* pointer to received data */
#ifdef CO_SDO_NETWORKING
		, CO_SDO_SERVER_T *pLocalReqSdo	/* local network request (only ML)*/
#endif /* CO_SDO_NETWORKING */
	)
{
CO_SDO_SERVER_T	*pSdo;
RET_T	retVal = RET_OK;
#ifdef CANOPEN_SUPPORTED
CO_NMT_STATE_T nmtState;

	nmtState = coNmtGetState();

	/* OPERATIONAL ? */
	if ((nmtState != CO_NMT_STATE_PREOP)
	 && (nmtState != CO_NMT_STATE_OPERATIONAL))  {
		return;
	}
#endif /* CANOPEN_SUPPORTED */

	/* check service index */
	if (pRecData->spec >= sdoServerCnt)  {
		return;
	}
	pSdo = getSdoServer(pRecData->spec);

	/* check for correct message len */
	if (pRecData->msg.len != 8u)  {
		return;
	}

#ifdef CO_APPL_SDO_HANDLER
	/* if sdo is unused */
	if (pSdo->state == CO_SDO_STATE_FREE)  {
		icoSdoDeCodeMultiplexer(&pRecData->msg.data[1], pSdo);

		/* forward requests for object > 0x2000 to application */
		if (pSdo->index >= 0x2000) {
			applSdoHandler(pRecData->spec, &pRecData->msg);
			return;
		}
	}
#endif /* CO_APPL_SDO_HANDLER */


#ifdef CO_SDO_BLOCK
	/* block transfer active and not an abort */
	if ((pSdo->state == CO_SDO_STATE_BLOCK_DOWNLOAD) 
	 && (pRecData->msg.data[0] != CO_SDO_CS_ABORT)) {
		(void)icoSdoServerBlockWrite(pSdo, &pRecData->msg);
		return;
	}
#endif /* CO_SDO_BLOCK */

	switch (pRecData->msg.data[0] & CO_SDO_CCS_MASK)  {
		case CO_SDO_CS_ABORT:
#ifdef CO_EVENT_SSDO_DOMAIN_WRITE
			/* send abort to domain write indication */
			if (pSdo->pObjDesc->dType == CO_DTYPE_DOMAIN)  {	
				pSdo->transferedSize = 0u;
				pSdo->domainTransferedSize = 0u;
				(void)icoSdoDomainUserWriteInd(pSdo);
			}
#endif /* CO_EVENT_SSDO_DOMAIN_WRITE */

			/* sdo abort handler */
			pSdo->state = CO_SDO_STATE_FREE;
			break;
		case CO_SDO_CCS_DOWNLOAD_INIT:		/* init download */
			retVal = sdoServerWriteInit(pSdo, &pRecData->msg);
			break;
		case CO_SDO_CCS_DOWNLOAD_SEGMENT:	/* segment download */
			retVal = sdoServerWriteSegment(pSdo, &pRecData->msg);
			break;
		case CO_SDO_CCS_UPLOAD_INIT:		/* init upload */
			retVal = sdoServerReadInit(pSdo, &pRecData->msg);
			break;
		case CO_SDO_CCS_UPLOAD_SEGMENT:		/* segment upload */
			retVal = sdoServerReadSegment(pSdo, &pRecData->msg);
			break;

#ifdef CO_SDO_BLOCK
		case CO_SDO_CCS_BLOCK_UPLOAD:		/* block upload */
			switch (pRecData->msg.data[0] & CO_SDO_CCS_BLOCK_SC_MASK)  {
				case CO_SDO_CCS_BLOCK_SC_UL_INIT:	/* block upload init */
					retVal = icoSdoServerBlockReadInit(pSdo, &pRecData->msg);
					break;
				case CO_SDO_CCS_BLOCK_SC_UL_BLK:	/* block upload blk */
					retVal = icoSdoServerBlockRead(pSdo);
					break;
				case CO_SDO_CCS_BLOCK_SC_UL_CON: 	/* block confirmation */
					retVal = icoSdoServerBlockReadCon(pSdo, &pRecData->msg);
					break;
				case CO_SDO_CCS_BLOCK_SC_UL_END:	/* block upload finished */
					pSdo->state = CO_SDO_STATE_FREE;
					break;
				default:
					break;
			}
			break;
		case CO_SDO_CCS_BLOCK_DOWNLOAD:		/* block download */
			switch (pRecData->msg.data[0] & CO_SDO_CCS_BLOCK_CS_MASK)  {
				case CO_SDO_CCS_BLOCK_CS_DL_INIT:	/* block download init */
					retVal = icoSdoServerBlockWriteInit(pSdo, &pRecData->msg);
					break;
				case CO_SDO_CCS_BLOCK_CS_DL_END:	/* block download end */
					retVal = icoSdoServerBlockWriteEnd(pSdo, &pRecData->msg);
					break;
				default:
					break;
			}
			break;
#endif /* CO_SDO_BLOCK */


		default:
			/* ignore wrong CCS */
			retVal = RET_SDO_UNKNOWN_CCS;
			icoSdoDeCodeMultiplexer(&pRecData->msg.data[1], pSdo);
			break;
	}

	if (retVal != RET_OK)  {
		icoSdoServerAbort(pSdo, retVal, CO_FALSE);
	}

	return;
}


/******************************************************************************/
/**
* \internal
*
* \brief sdoServerReadInit
*
* \return RET_T
*
*/
static RET_T sdoServerReadInit(
		CO_SDO_SERVER_T		*pSdo,		/* pointer to sdo */
		const CO_CAN_REC_MSG_T	*pRecData	/* pointer to received data */
	)
{
RET_T	retVal;
UNSIGNED16	objAttr;

	/* transfer already actice ? */
	if (pSdo->state != CO_SDO_STATE_FREE)  {
		return(RET_SERVICE_BUSY);
	}

	/* save multiplexer */
	icoSdoDeCodeMultiplexer(&pRecData->data[1], pSdo);

	/* check index/subindex/attribute/size/limits */
	retVal = coOdGetObjDescPtr(pSdo->index, pSdo->subIndex, &pSdo->pObjDesc);
	if (retVal != RET_OK)  {
		return(retVal);
	}

	/* check attribute */
	objAttr = coOdGetObjAttribute(pSdo->pObjDesc);
	if ((objAttr & CO_ATTR_READ) == 0u)  {
		return(RET_NO_READ_PERM);
	}

#ifdef CO_EVENT_SSDO_DOMAIN_READ
	if (pSdo->pObjDesc->dType == CO_DTYPE_DOMAIN) {
		pSdo->domainTransfer = CO_TRUE;
	}
	else {
		pSdo->domainTransfer = CO_FALSE;
	}
#endif /* CO_EVENT_SSDO_DOMAIN_READ */

	pSdo->transferedSize = 0u;
	/* call all user indications */
	retVal = icoSdoCheckUserReadInd(pSdo->sdoNr, pSdo->index, pSdo->subIndex);

#ifdef CO_EVENT_SSDO_DOMAIN_READ
	if (retVal == RET_OK)  {
		if (pSdo->domainTransfer == CO_TRUE)  {
			pSdo->domainTransferedSize = 0u;
			/* call indication with transfered buffer size max. */
			retVal = icoSdoDomainUserReadInd(pSdo);
		}
	}
#endif /* CO_EVENT_SSDO_DOMAIN_READ */

	if (retVal != RET_OK)  {
		/* split indikation ? */
#ifdef CO_SDO_SPLIT_INDICATION
			pSdo->split = CO_FALSE;

			if (retVal == RET_SDO_SPLIT_INDICATION) {
				pSdo->split = CO_TRUE;
				pSdo->state = CO_SDO_STATE_UPLOAD_INIT;
				retVal = RET_OK;
			}
#endif /* CO_SDO_SPLIT_INDICATION */

		return(retVal);
	}

	pSdo->state = CO_SDO_STATE_UPLOAD_INIT;
	retVal = sdoServerReadIndCont(pSdo);

	return(retVal);
}


/******************************************************************************/
/**
*  \internal
*
* \brief sdoServerReadIndCont - continue SDO server Read indication
*
*
*/
static RET_T sdoServerReadIndCont(
		CO_SDO_SERVER_T	*pSdo
	)
{
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */
UNSIGNED32	objSize;
RET_T		retVal;

#ifdef CO_SDO_BLOCK
	/* are we the block transfer and in the middle of a block? */
	if (pSdo->state == CO_SDO_STATE_BLOCK_UPLOAD) {
		retVal = icoSdoServerBlockRead(pSdo);
		return(retVal);
	}
	/* are we the block transfer and in the end of a block? */
	if ((pSdo->state == CO_SDO_STATE_BLOCK_UPLOAD_RESP)  
		|| (pSdo->state == CO_SDO_STATE_BLOCK_UPLOAD_INIT)
		|| (pSdo->state == CO_SDO_STATE_BLOCK_UPLOAD_LAST))  {
		return(RET_OK);
	}
#endif /* CO_SDO_BLOCK */

	objSize = coOdGetObjSize(pSdo->pObjDesc);

	trData[1] = (UNSIGNED8)(pSdo->index & 0xffu);
	trData[2] = (UNSIGNED8)(pSdo->index >> 8);
	trData[3] = pSdo->subIndex;
	memset(&trData[4], 0, CO_CAN_MAX_DATA_LEN - 4u);

	/* segmented transfer */
	if (objSize > 4u)  {
		if (pSdo->state == CO_SDO_STATE_UPLOAD_SEGMENT)  {
			CO_CAN_REC_MSG_T fakeMsg;
			/* fake the last message */
			if (pSdo->toggle == 0u) {
				pSdo->toggle = CO_SDO_CCS_TOGGLE_BIT;
				fakeMsg.data[0] = CO_SDO_CCS_TOGGLE_BIT;
			} else {
				pSdo->toggle = 0u;
				fakeMsg.data[0] = 0u;
			}
			retVal = sdoServerReadSegment(pSdo, &fakeMsg);
			return(retVal);
		}

		if (pSdo->state == CO_SDO_STATE_UPLOAD_INIT)  {
			pSdo->objSize = objSize;
			trData[0] = CO_SDO_SCS_UPLOAD_INIT | CO_SDO_SCS_LEN_INDICATED;
			coNumMemcpyUnpack(&trData[4], &pSdo->objSize, 4u, CO_ATTR_NUM, 0u);
			pSdo->transferedSize = 0u;

#ifdef CO_EVENT_SSDO_DOMAIN_READ
			pSdo->domainTransferedSize = 0u;
#endif /* CO_EVENT_SSDO_DOMAIN_READ */
			pSdo->toggle = 0u;

			pSdo->state = CO_SDO_STATE_UPLOAD_SEGMENT;
		}
	} else {

		/* expetided transfer */

		/* SCS - number of bytes specified */
		trData[0] = (UNSIGNED8)(CO_SDO_SCS_UPLOAD_INIT
			| ((4u - objSize) << 2)
			| CO_SDO_SCS_EXPEDITED | CO_SDO_SCS_LEN_INDICATED);
		pSdo->state = CO_SDO_STATE_FREE;

		retVal = icoOdGetObj(pSdo->pObjDesc, pSdo->subIndex, &trData[4],
			0u, 4u, CO_FALSE);
		if (retVal != RET_OK)  {
			return(retVal);
		}
	}

	/* transmit answer */
	{
		retVal = icoTransmitMessage(pSdo->trCob, &trData[0], 0u);
	}

	return(retVal);
}


#ifdef CO_SDO_SPLIT_INDICATION
/******************************************************************************/
/**
* \brief coSdoServerReadIndCont - continue SDO server Read indication
*
* This function has to be called,
* after the sdoServerReadInd function has returned RET_SDO_SPLIT_INDICATION	
* to continue and finish the SDO transfer
*
* The result parameter should contain the result for the transfer
*
* \return RET_T
*
*/
RET_T coSdoServerReadIndCont(
		UNSIGNED8		sdoNr,		/**< sdo number */
		RET_T			result		/**< result for transfer */
	)
{
CO_SDO_SERVER_T	*pSdo;
RET_T		retVal;

	pSdo = icoSdoServerPtr((UNSIGNED16)sdoNr);
	if (pSdo->split != CO_TRUE)  {
		return(RET_INTERNAL_ERROR);
	}

	if (result != RET_OK)  {
		icoSdoServerAbort(pSdo, result, CO_FALSE);
		return(RET_OK);
	}

	pSdo->split = CO_FALSE;

	retVal = sdoServerReadIndCont(pSdo);
	if (retVal != RET_OK)  {
		icoSdoServerAbort(pSdo, result, CO_FALSE);
		return(RET_OK);
	}

	return(retVal);
}
#endif /* CO_SDO_SPLIT_INDICATION */


/***************************************************************************/
/**
* \internal
*
* \brief sdoServerReadSegment
*
* \return RET_T
*/
static RET_T sdoServerReadSegment(
		CO_SDO_SERVER_T			*pSdo,		/* pointer to sdo */
		CO_CONST CO_CAN_REC_MSG_T	*pRecData	/* pointer to received data */
	)
{
RET_T	retVal;
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */
UNSIGNED32	size;
UNSIGNED8	toggle;
UNSIGNED8	dataOffs;

	if (pSdo->state != CO_SDO_STATE_UPLOAD_SEGMENT)  {
		return(RET_SDO_UNKNOWN_CCS);
	}

	memset(&trData[1], 0, CO_CAN_MAX_DATA_LEN - 1u);

	toggle = pRecData->data[0] & CO_SDO_CCS_TOGGLE_BIT;
	if (pSdo->toggle != toggle)  {
		return(RET_TOGGLE_MISMATCH);
	}
	if (toggle == 0u)  {
		pSdo->toggle = CO_SDO_CCS_TOGGLE_BIT;
	} else {
		pSdo->toggle = 0u;
	}

#ifdef CO_EVENT_SSDO_DOMAIN_READ
	/* domain transfer ? */
	if (pSdo->domainTransfer == CO_TRUE) {
		/* size reached or last segment */
		if (((pSdo->transferedSize % (7u * CO_SSDO_DOMAIN_CNT)) == 0u)
		 && (pSdo->transferedSize > 0u)) {
			/* domain indication */
			retVal = icoSdoDomainUserReadInd(pSdo);
			/* reset domain pointer */
			pSdo->transferedSize = 0u;
# ifdef CO_SDO_SPLIT_INDICATION
			pSdo->split = CO_FALSE;
			if (retVal == RET_SDO_SPLIT_INDICATION) {
				pSdo->split = CO_TRUE;
				return(RET_OK);
			}
# endif /* CO_SDO_SPLIT_INDICATION */
		}
	}
#endif /* CO_EVENT_SSDO_DOMAIN_READ  */

	trData[0] = CO_SDO_SCS_UPLOAD_SEGMENT;
	trData[0] |= toggle;

	{
		/* last transfer ? */
		if (pSdo->objSize > 7u)  {
			size = 7u;
		} else {
			/* last transfer */
			size = pSdo->objSize;
			trData[0] |= (UNSIGNED8)(CO_SDO_SCS_CONT_FLAG | ((7u - size) << 1));

			pSdo->state = CO_SDO_STATE_FREE;
		}
		dataOffs = 1u;
	}

	retVal = icoOdGetObj(pSdo->pObjDesc, pSdo->subIndex, &trData[dataOffs], 
			pSdo->transferedSize, size, CO_FALSE);
	if (retVal != RET_OK)  {
		return(retVal);
	}

	pSdo->objSize -= size;
	pSdo->transferedSize += size;

#ifdef CO_EVENT_SSDO_DOMAIN_READ
	pSdo->domainTransferedSize += size;
#endif /* CO_EVENT_SSDO_DOMAIN_READ */

	/* transmit answer */
	{
		retVal = icoTransmitMessage(pSdo->trCob, &trData[0], 0u);
	}

	return(retVal);
}


/***************************************************************************/
/**
* \internal
*
* \brief sdoServerWriteInit
*
* \return RET_T
*
*/
static RET_T sdoServerWriteInit(
		CO_SDO_SERVER_T		*pSdo,		/* pointer to sdo */
		const CO_CAN_REC_MSG_T	*pRecData	/* pointer to receive data */
	)
{
RET_T	retVal;
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */
UNSIGNED16	objAttr;
UNSIGNED32	len = 0u;
BOOL_T		segmented = CO_FALSE;
UNSIGNED8	u8;
UNSIGNED8	dataStartIdx = 4u;

	/* transfer already actice ? */
	if (pSdo->state != CO_SDO_STATE_FREE)  {
		return(RET_SERVICE_BUSY);
	}

	/* save multiplexer */
	icoSdoDeCodeMultiplexer(&pRecData->data[1], pSdo);

	/* check index/subindex/attribute/size/limits */
	retVal = coOdGetObjDescPtr(pSdo->index, pSdo->subIndex, &pSdo->pObjDesc);
	if (retVal != RET_OK)  {
		return(retVal);
	}
	pSdo->objSize = coOdGetObjMaxSize(pSdo->pObjDesc);

	/* check attribute */
	objAttr = coOdGetObjAttribute(pSdo->pObjDesc);
	if ((objAttr & CO_ATTR_WRITE) == 0u)  {
		return(RET_NO_WRITE_PERM);
	}

	/* check data size */
	switch (pRecData->data[0] & CO_SDO_CCS_WRITE_TYPE_MASK)  {
		case 1u:		/* count in byte 5..8 */
			(void)coNumMemcpyPack(&len, &pRecData->data[4], 4u, CO_ATTR_NUM, 0u);
			segmented = CO_TRUE;
			break;
		case 3u:		/* count = 4-n */
			u8 = (pRecData->data[0] & CO_SDO_CCS_LEN_MASK) >> 2;
			len = 4ul - u8;
			break;
		case 2u:		/* size unspecified */
			len = pSdo->objSize;
			break;

		default:
			return(RET_SDO_UNKNOWN_CCS);
	}

	/* len doesnt match */
	if (len != pSdo->objSize)  {
		/* not enough memory ? */
		if (len > pSdo->objSize)  {
			return(RET_SDO_DATA_TYPE_NOT_MATCH);
		} else {
			if ((objAttr & CO_ATTR_NUM) != 0u)   {
				return(RET_SDO_DATA_TYPE_NOT_MATCH);
			}
		}
	}

	/* check limits */
	retVal = icoOdCheckObjLimits(pSdo->pObjDesc, &pRecData->data[dataStartIdx]);
	if (retVal != RET_OK)  {
		return(retVal);
	}

	/* call all user indications */
	retVal = icoSdoCheckUserCheckWriteInd(pSdo->sdoNr, pSdo->index,
			pSdo->subIndex, &pRecData->data[dataStartIdx], len);
	if (retVal != RET_OK)  {
		/* return error */
		return(retVal);
	}

	/* segmented transfer ? */
	if (segmented == CO_TRUE)  {
		trData[0] = CO_SDO_SCS_DOWNLOAD_INIT;
		trData[1] = (UNSIGNED8)(pSdo->index & 0xffu);
		trData[2] = (UNSIGNED8)(pSdo->index >> 8u);
		trData[3] = pSdo->subIndex;
		memset(&trData[4], 0, 4u);

		/* transmit answer */
		{
			retVal = icoTransmitMessage(pSdo->trCob, &trData[0], 0u);
		}

		pSdo->transferedSize = 0u;
#ifdef CO_EVENT_SSDO_DOMAIN_WRITE
		pSdo->domainTransferedSize = 0u;
#endif /* CO_EVENT_SSDO_DOMAIN_WRITE */
		pSdo->toggle = 0u;

		pSdo->state = CO_SDO_STATE_DOWNLOAD_SEGMENT;

#ifdef CO_EVENT_SSDO_DOMAIN_WRITE
		if (pSdo->pObjDesc->dType == CO_DTYPE_DOMAIN)  {	
			pSdo->domainTransfer = CO_TRUE;
		} else {
			pSdo->domainTransfer = CO_FALSE;
		}
#endif /* CO_EVENT_SSDO_DOMAIN_WRITE */

		return(retVal);
	}

	/* expedited transfer */

	/* save orginal value */
	if (pSdo->objSize <= MAX_SAVE_DATA)  {
		retVal = icoOdGetObj(pSdo->pObjDesc, pSdo->subIndex,
			&pSdo->saveData[0], 0u, MAX_SAVE_DATA, CO_TRUE);
	}

	if (retVal == RET_OK)  {
		/* put new value */
		retVal = icoOdPutObj(pSdo->pObjDesc, pSdo->subIndex,
			&pRecData->data[dataStartIdx], 0u, len, CO_FALSE, &pSdo->changed);
	}
	if (retVal != RET_OK)  {
		/* return error */
		return(retVal);
	}

#ifdef CO_EVENT_SSDO_DOMAIN_WRITE
	if (pSdo->pObjDesc->dType == CO_DTYPE_DOMAIN)  {	
		pSdo->transferedSize = len;
		pSdo->domainTransferedSize = len;
		retVal = icoSdoDomainUserWriteInd(pSdo);
	}
#endif /* CO_EVENT_SSDO_DOMAIN_WRITE */

	/* call user indication */
	if (retVal == RET_OK)  {
		(void)coOdSetObjSize(pSdo->pObjDesc, len);
		retVal = icoSdoCheckUserWriteInd(pSdo);
	}
	if (retVal != RET_OK)  {
		if (retVal == RET_SDO_SPLIT_INDICATION)  {
			pSdo->state = CO_SDO_STATE_WR_SPLIT_INDICATION;
			return(RET_OK);
		}

		/* restore data, if user ind returns error */
		/* restore original entry */
		if (pSdo->objSize <= MAX_SAVE_DATA)  {
			(void)icoOdPutObj(pSdo->pObjDesc, pSdo->subIndex,
				&pSdo->saveData[0], 0u, MAX_SAVE_DATA, CO_TRUE, &pSdo->changed);
		}

		(void)coOdSetObjSize(pSdo->pObjDesc, pSdo->objSize);

		/* return error */
		return(retVal);
	}

	pSdo->state = CO_SDO_STATE_WR_SPLIT_INDICATION;
	retVal = sdoServerWriteIndCont(pSdo);

	return(retVal);
}


#ifdef CO_SDO_SPLIT_INDICATION
/***************************************************************************/
/**
*
* \brief coSdoServerWriteIndCont - continue SDO server Write indication
*
* This function has to be called,
* after the sdoServerWriteInd function has returned RET_SDO_SPLIT_INDICATION	
* to continue and finish the SDO transfer
*
* The result parameter should contain the result for the transfer.
*
* \return RET_T
*
*/
RET_T coSdoServerWriteIndCont(
		UNSIGNED8	sdoNr,		/**< sdo number */
		RET_T		result		/**< result indication */
	)
{
CO_SDO_SERVER_T	*pSdo;
RET_T	retVal;

	pSdo = icoSdoServerPtr((UNSIGNED16)sdoNr);

	if ((pSdo->state != CO_SDO_STATE_WR_SPLIT_INDICATION)
	 && (pSdo->state != CO_SDO_STATE_WR_SEG_SPLIT_INDICATION)
	 && (pSdo->state != CO_SDO_STATE_WR_BLOCK_SPLIT_INDICATION))  {
		return(RET_INTERNAL_ERROR);
	}

	/* user request error ? */
	if (result != RET_OK)  {
		/* restore data, if user ind returns error */
		/* restore original entry */
		if (pSdo->objSize <= MAX_SAVE_DATA)  {
			(void)icoOdPutObj(pSdo->pObjDesc, pSdo->subIndex, &pSdo->saveData[0],
				0u, MAX_SAVE_DATA, CO_TRUE, &pSdo->changed);
		}

		(void)coOdSetObjSize(pSdo->pObjDesc, pSdo->objSize);
		icoSdoServerAbort(pSdo, result, CO_FALSE);
		return(RET_OK);
	}

	retVal = sdoServerWriteIndCont(pSdo);
	if (retVal != RET_OK)  {
		icoSdoServerAbort(pSdo, retVal, CO_FALSE);
	}

	return(retVal);
}
#endif /* CO_SDO_SPLIT_INDICATION */


/***************************************************************************/
/**
* \internal
*
* \brief sdoServerWriteIndCont
*
* \return RET_T
*
*/
static RET_T sdoServerWriteIndCont(
		CO_SDO_SERVER_T	*pSdo
	)
{
RET_T	retVal;
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */
UNSIGNED8	toggle;

	/* call object change indication */
	retVal = icoEventObjectChanged(pSdo->pObjDesc,
		pSdo->index, pSdo->subIndex, pSdo->changed);
	if (retVal != RET_OK)  {
		/* return error */
		return(retVal);
	}

#ifdef CO_SDO_BLOCK
	if (pSdo->state == CO_SDO_STATE_WR_BLOCK_SPLIT_INDICATION)  {
		retVal = icoSdoServerBlockWriteConfirmation(pSdo);
		pSdo->state = CO_SDO_STATE_BLOCK_DOWNLOAD;
		return(retVal);
	}
#endif /* CO_SDO_BLOCK */

	if (pSdo->state == CO_SDO_STATE_WR_SPLIT_INDICATION)  {
		/* SCS - number of bytes specified */
		trData[0] = CO_SDO_SCS_DOWNLOAD_INIT;
		trData[1] = (UNSIGNED8)(pSdo->index & 0xffu);
		trData[2] = (UNSIGNED8)(pSdo->index >> 8u);
		trData[3] = pSdo->subIndex;

		memset(&trData[4], 0, 4u);
	} else {
		if (pSdo->toggle == 0u)  {
			toggle = CO_SDO_CCS_TOGGLE_BIT;
		} else {
			toggle = 0u;
		}
		trData[0] = CO_SDO_SCS_DOWNLOAD_SEGMENT | toggle;
		memset(&trData[1], 0, 7u);
	}

	/* transmit answer */
	{
		retVal = icoTransmitMessage(pSdo->trCob, &trData[0], 0u);
	}

	pSdo->state = CO_SDO_STATE_FREE;

	return(retVal);
}



/***************************************************************************/
/**
* \internal
*
* \brief sdoServerWriteSegment
*
* \return RET_T
*
*/
static RET_T sdoServerWriteSegment(
		CO_SDO_SERVER_T		*pSdo,		/* pointer to sdo */
		const CO_CAN_REC_MSG_T	*pRecData	/* pointer to received data */
	)
{
RET_T	retVal;
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */
UNSIGNED8	toggle;
UNSIGNED8	u8;
UNSIGNED32	size;
UNSIGNED8	dataOffs = 1u;

	/* transfer already actice ? */
	if ((pSdo->state != CO_SDO_STATE_DOWNLOAD_SEGMENT)
	 && (pSdo->state != CO_SDO_STATE_FD_DOWNLOAD_SEGMENT))  {
		return(RET_SERVICE_BUSY);
	}

	toggle = pRecData->data[0] & CO_SDO_CCS_TOGGLE_BIT;
	if (pSdo->toggle != toggle)  {
		return(RET_TOGGLE_MISMATCH);
	}
	if (toggle == 0u)  {
		pSdo->toggle = CO_SDO_CCS_TOGGLE_BIT;
	} else {
		pSdo->toggle = 0u;
	}

	u8 = (pRecData->data[0] & CO_SDO_CCS_LEN_MASK) >> 1;
	size = 7ul - u8;

	if (size > pSdo->objSize)  {
		return(RET_OUT_OF_MEMORY);
	}

	/* put data */
	retVal = icoOdPutObj(pSdo->pObjDesc, pSdo->subIndex,
		&pRecData->data[dataOffs], pSdo->transferedSize, size,
		CO_FALSE, &pSdo->changed);
	if (retVal != RET_OK)  {
		/* return error */
		return(retVal);
	}
	pSdo->objSize -= size;
	pSdo->transferedSize += size;

#ifdef CO_EVENT_SSDO_DOMAIN_WRITE
	pSdo->domainTransferedSize += size;

	/* domain transfer ? */
	if (pSdo->domainTransfer == CO_TRUE)  {
		/* size reached or last segment */
		if (((pSdo->transferedSize % (7u * CO_SSDO_DOMAIN_CNT)) == 0u)
		 ||	((pRecData->data[0] & CO_SDO_CCS_CONT_BIT) != 0u))  {
			/* domain indication */
			retVal = icoSdoDomainUserWriteInd(pSdo);
			if (retVal != RET_OK)  {
				return(retVal);
			}
			/* reset domain pointer */
			pSdo->transferedSize = 0u;
		}
	}
#endif /* CO_EVENT_SSDO_DOMAIN_WRITE */

	/* last transfer ? */
	if ((pRecData->data[0] & CO_SDO_CCS_CONT_BIT) != 0u)  {
		/* last segment */

#ifdef CO_EVENT_SSDO_DOMAIN_WRITE
		/* domain transfer ? */
		if (pSdo->domainTransfer == CO_TRUE)  {
			(void)coOdSetObjSize(pSdo->pObjDesc, pSdo->domainTransferedSize);
		} else {
			(void)coOdSetObjSize(pSdo->pObjDesc, pSdo->transferedSize);
		}
#else /* CO_EVENT_SSDO_DOMAIN_WRITE */
		(void)coOdSetObjSize(pSdo->pObjDesc, pSdo->transferedSize);
#endif /* CO_EVENT_SSDO_DOMAIN_WRITE */

		/* call user indication */
		retVal = icoSdoCheckUserWriteInd(pSdo);
		if (retVal != RET_OK)  {
			if (retVal == RET_SDO_SPLIT_INDICATION)  {
				pSdo->state = CO_SDO_STATE_WR_SEG_SPLIT_INDICATION;
				return(RET_OK);
			}
			/* return error */
			return(retVal);
		}

		pSdo->state = CO_SDO_STATE_WR_SEG_SPLIT_INDICATION;
		retVal = sdoServerWriteIndCont(pSdo);
		return(retVal);
	}

	
	trData[0] = CO_SDO_SCS_DOWNLOAD_SEGMENT | toggle;
	memset(&trData[1], 0, 7u);

	/* transmit answer */
	{
		retVal = icoTransmitMessage(pSdo->trCob, &trData[0], 0u);
	}

	return(retVal);
}



/***************************************************************************/
/**
*
* \brief icoSdoGetAbortCode - return sdo abort code
*
* This function returns the SDO Abort Code as 32bit value depending on given RET_T value
*
* \return abort code as U32
*
*/
UNSIGNED32 coSdoGetAbortCode(
		RET_T			errorReason		/**< error reason */
	)
{
UNSIGNED32	abortCode = 0x08000000UL;
UNSIGNED16	i;

	for (i = 0u; i < (sizeof(abortCodeTable) / sizeof(SDO_ABORT_CODE_TABLE_T)); i++) {
		if (abortCodeTable[i].reason == errorReason)  {
			abortCode = abortCodeTable[i].abortCode;
			break;
		}
	}

	return(abortCode);
}


/***************************************************************************/
/**
*
* \brief coSdoGetAbortReason - return sdo abort Reason
*
* This function returns the abort reason as RET_T from given SDO error code
*
* \return 
* 	RET_T
*
*/
RET_T coSdoGetAbortReason(
		UNSIGNED32			errorCode		/**< error code */
	)
{
RET_T	abortReason = RET_OK;
UNSIGNED16	i;

	for (i = 0u; i < (sizeof(abortCodeTable) / sizeof(SDO_ABORT_CODE_TABLE_T)); i++) {
		if (abortCodeTable[i].abortCode == errorCode)  {
			abortReason = abortCodeTable[i].reason;
			break;
		}
	}

	return(abortReason);
}


/***************************************************************************/
/**
* \internal
*
* \brief icoSdoServerAbort - abort sdo transfer
*
* \return none
*
*/
void icoSdoServerAbort(
		CO_SDO_SERVER_T	*pSdo,			/* pointer to sdo */
		RET_T			errorReason,	/* error reason */
		BOOL_T			fromClient		/* called from client */
    )
{
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */
UNSIGNED32	abortCode;

	if (fromClient == CO_TRUE)  {
		/* user indication */




	} else {
		abortCode = coSdoGetAbortCode(errorReason);

		trData[0] = CO_SDO_CS_ABORT;
		sdoCodeMultiplexer(pSdo, &trData[1]);
		coNumMemcpyUnpack(&trData[4], &abortCode, 4u, CO_ATTR_NUM, 0u);

		{
			(void) icoTransmitMessage(pSdo->trCob, &trData[0], 0u);
		}
	}

	pSdo->state = CO_SDO_STATE_FREE;
}


/******************************************************************************/
/**
* \internal
*
* \brief icoSearchSdoServer - search for sdo server
*
*
* \return index
*
*/
CO_SDO_SERVER_T	*icoSdoServerPtr(
		UNSIGNED16		sdoNr
	)
{
UNSIGNED16	cnt;
CO_SDO_SERVER_T	*pSdo;

	for (cnt = 0u; cnt < sdoServerCnt; cnt++)  {
		pSdo = getSdoServer(cnt);
		if (pSdo != NULL)  {
			if (sdoNr == pSdo->sdoNr)  {
				return(pSdo);
			}
		}
	}

	return(NULL);
}


#ifdef CO_APPL_SDO_HANDLER
/***************************************************************************/
/**
*
* \brief applSdoAnswer - answer from application SDO handler
*
* Transmit SDO answer from application SDO handler
*
* \return RET_T
*
*/
RET_T coSdoServerApplAnswer(
		UNSIGNED16		sdoSpec,		/**< spec (given by applSdoHandler)*/
		CO_CAN_REC_MSG_T	*pCanMsg		/**< pointer to transmit data */
	)
{
RET_T	retVal;
CO_SDO_SERVER_T	*pSdo;
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */

	pSdo = getSdoServer(sdoSpec);

	memcpy(&trData[0], &pCanMsg->data[0], 8);
	retVal = icoTransmitMessage(pSdo->trCob, &trData[0], 0u);

	return(retVal);
}
#endif /* CO_APPL_SDO_HANDLER */


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
#ifdef CO_EVENT_SDO_SERVER_READ_CNT
/***************************************************************************/
/**
* \brief coEventRegister_SdoServer - register SDO server event
*
* This function registers a sdo server indication function,
* which is called before a SDO read request is executed,
* so the application can update the data before the response is
* sent.
*
* \return RET_T
*
*/
RET_T coEventRegister_SDO_SERVER_READ(
		CO_EVENT_SDO_SERVER_T pFunction		/**< pointer to function */
    )
{
	if (sdoServerReadTableCnt >= CO_EVENT_SDO_SERVER_READ_CNT) {
		return(RET_EVENT_NO_RESSOURCE);
	}

	/* set new indication function as first at the list */
	sdoServerReadTable[sdoServerReadTableCnt] = pFunction;
	sdoServerReadTableCnt++;

	return(RET_OK);
}
#endif /* CO_EVENT_SDO_SERVER_READ_CNT */


#ifdef CO_EVENT_SDO_SERVER_CHECK_WRITE_CNT
/***************************************************************************/
/**
* \brief coEventRegister_SdoServerCheckWrite - register SDO server write event
*
* This function register a sdo server indication function,
* which is called before SDO write access is executed,
* so the application can reject an SDO write access. 
*
*
* \return RET_T
*
*/
RET_T coEventRegister_SDO_SERVER_CHECK_WRITE(
		CO_EVENT_SDO_SERVER_CHECK_WRITE_T pFunction /**< pointer to function */
    )
{
	if (sdoServerCheckWriteTableCnt >= CO_EVENT_SDO_SERVER_CHECK_WRITE_CNT) {
		return(RET_EVENT_NO_RESSOURCE);
	}

	sdoServerCheckWriteTable[sdoServerCheckWriteTableCnt] = pFunction;
	sdoServerCheckWriteTableCnt++;

	return(RET_OK);
}
#endif /*  CO_EVENT_SDO_SERVER_CHECK_WRITE_CNT */


#ifdef CO_EVENT_SDO_SERVER_WRITE_CNT
/***************************************************************************/
/**
* \brief coEventRegister_SdoServerWrite - register SDO server write event
*
* This function registers a SDO server write indication function.
* It is called, after a SDO write access was finished.
*
* \return RET_T
*
*/
RET_T coEventRegister_SDO_SERVER_WRITE(
		CO_EVENT_SDO_SERVER_T	pFunction	/**< pointer to function */
    )
{
	if (sdoServerWriteTableCnt >= CO_EVENT_SDO_SERVER_WRITE_CNT) {
		return(RET_EVENT_NO_RESSOURCE);
	}

	sdoServerWriteTable[sdoServerWriteTableCnt] = pFunction;
	sdoServerWriteTableCnt++;

	return(RET_OK);
}
#endif /* CO_EVENT_SDO_SERVER_WRITE_CNT */


#ifdef CO_EVENT_SSDO_DOMAIN_WRITE_CNT
/***************************************************************************/
/**
* \brief coEventRegister_SDO_SERVER_DOMAIN_WRITE - register SDO server domain write event
*
* This function registers a SDO server write domain indication function.
* It is called, after a specified portion of bytes at a domain transfer
* was receiced.
* After the function was called, the domain pointer start address is refreshed,
* so new data is saved at start of the domain again.
* The application can use this function to save may be the data at flash.
*
* If an abort from client was received, the indication is called 
* with parameter domainBufSize=0 and transferSize=0
*
* \return RET_T
*
*/
RET_T coEventRegister_SDO_SERVER_DOMAIN_WRITE(
		CO_EVENT_SDO_SERVER_DOMAIN_WRITE_T	pFunction	/**< pointer to function */
    )
{
	if (sdoServerDomainWriteTableCnt >= CO_EVENT_SSDO_DOMAIN_WRITE_CNT) {
		return(RET_EVENT_NO_RESSOURCE);
	}

	sdoServerDomainWriteTable[sdoServerDomainWriteTableCnt] = pFunction;
	sdoServerDomainWriteTableCnt++;

	return(RET_OK);
}
#endif /* CO_EVENT_SSDO_DOMAIN_WRITE_CNT */


#ifdef CO_EVENT_SSDO_DOMAIN_READ_CNT
/***************************************************************************/
/**
* \brief coEventRegister_SDO_SERVER_DOMAIN_READ - register SDO server domain read event
*
* This function registers a SDO server read domain indication function.
* It is called, after a specified portion of bytes at a domain transfer
* was transfered.
* After the function was called, the domain pointer start address is refreshed,
* so new data is read at start of the domain again.
* The application can use this function to reload the particular portion of the domain.
*
*
* \return RET_T
*
*/
RET_T coEventRegister_SDO_SERVER_DOMAIN_READ(
        CO_EVENT_SDO_SERVER_DOMAIN_READ_T    pFunction    /**< pointer to function */
    )
{
	if (sdoServerDomainReadTableCnt >= CO_EVENT_SSDO_DOMAIN_READ_CNT) {
		return(RET_EVENT_NO_RESSOURCE);
	}

	sdoServerDomainReadTable[sdoServerDomainReadTableCnt] = pFunction;
	sdoServerDomainReadTableCnt++;

	return(RET_OK);
}
#endif /* CO_EVENT_SSDO_DOMAIN_READ_CNT */


/***************************************************************************/
/**
* \internal
*
* \brief icoSdoCheckUserReadInd - check user indication
*
* \return RET_T
*
*/
RET_T icoSdoCheckUserReadInd(
		UNSIGNED8		sdoNr,		/* sdo number */
		UNSIGNED16		index,		/* index */
		UNSIGNED8		subIndex	/* subindex */
	)
{
#ifdef CO_EVENT_SDO_SERVER_READ
RET_T	retVal = RET_OK;
UNSIGNED8	splitInd = 0u;
# ifdef CO_EVENT_SDO_SERVER_READ_CNT
UNSIGNED8	cnt;

	/* check all indications for correct return value */
	cnt = sdoServerReadTableCnt;
	while (cnt > 0u)  {
		cnt--;

		/* check user indication */
		retVal = sdoServerReadTable[cnt](CO_FALSE, sdoNr, index, subIndex);
		if (retVal != RET_OK)  {
			if (retVal == RET_SDO_SPLIT_INDICATION)  {
				splitInd++;
			} else {
				return(retVal);
			}
		}
	}
# endif /* CO_EVENT_SDO_SERVER_READ_CNT */

# ifdef CO_EVENT_STATIC_SDO_SERVER_READ
	retVal = coEventSdoServerReadInd(CO_FALSE, sdoNr, index, subIndex);
	/* return, if not ok */
	if (retVal != RET_OK)  {
		if (retVal != RET_SDO_SPLIT_INDICATION)  {
			return(retVal);
		} else {
			splitInd++;
		}
	}
# endif /* CO_EVENT_STATIC_SDO_SERVER_READ */

# ifdef CO_EVENT_SDO_SERVER_READ_CNT
	/* call indication to execute */
	cnt = sdoServerReadTableCnt;
	while (cnt > 0u)  {
		cnt--;
		/* call user indication */
		(void)sdoServerReadTable[cnt](CO_TRUE, sdoNr, index, subIndex);
	}
# endif /* CO_EVENT_SDO_SERVER_READ_CNT */

# ifdef CO_EVENT_STATIC_SDO_SERVER_READ
	(void)coEventSdoServerReadInd(CO_TRUE, sdoNr, index, subIndex);
# endif /* CO_EVENT_STATIC_SDO_SERVER_READ */

	if (splitInd != 0u)  {
		return(RET_SDO_SPLIT_INDICATION);
	}
#else /* CO_EVENT_SDO_SERVER_READ */
	(void)sdoNr;
	(void)index;
	(void)subIndex;
#endif /* CO_EVENT_SDO_SERVER_READ */

	return(RET_OK);
}


/***************************************************************************/
/**
* \internal
*
* \brief icoSdoCheckUserCheckWriteInd - check user write indication
*
* \return RET_T
*
*/
RET_T icoSdoCheckUserCheckWriteInd(
		UNSIGNED8		sdoNr,		/* sdo number */
		UNSIGNED16		index,		/* index */
		UNSIGNED8		subIndex,	/* subindex */
		const UNSIGNED8	*pData,		/* pointer to received data */
		UNSIGNED32		size		/* size */
	)
{
	/* check all indications for correct return value */
RET_T	retVal = RET_OK;
#ifdef CO_EVENT_SDO_SERVER_CHECK_WRITE_CNT
UNSIGNED8	cnt;

	/* check all indications for correct return value */
	cnt = sdoServerCheckWriteTableCnt;
	while (cnt > 0u)  {
		cnt--;

		/* check user indication */
#ifdef CO_SRV_CHECK_WRITE_IND_SIZE
		retVal = sdoServerCheckWriteTable[cnt](CO_FALSE, sdoNr,
			index, subIndex, pData, size);
#else /* CO_SRV_CHECK_WRITE_IND_SIZE */
		retVal = sdoServerCheckWriteTable[cnt](CO_FALSE, sdoNr,
			index, subIndex, pData);
#endif /* CO_SRV_CHECK_WRITE_IND_SIZE */
		if (retVal != RET_OK)  {
			break;
		}
	}

	/* return, if not ok */
	if (retVal != RET_OK)  {
		return(retVal);
	}
#endif /* CO_EVENT_SDO_SERVER_CHECK_WRITE_CNT */

#ifdef CO_EVENT_STATIC_SDO_SERVER_CHECK_WRITE
# ifdef CO_SRV_CHECK_WRITE_IND_SIZE
	retVal = coEventSdoServerCheckWriteInd(CO_FALSE, sdoNr,
			index, subIndex, pData, size);
# else /* CO_SRV_CHECK_WRITE_IND_SIZE */
	retVal = coEventSdoServerCheckWriteInd(CO_FALSE, sdoNr,
			index, subIndex, pData);
# endif /* CO_SRV_CHECK_WRITE_IND_SIZE */
	/* return, if not ok */
	if (retVal != RET_OK)  {
		return(retVal);
	}
#endif /* CO_EVENT_STATIC_SDO_SERVER_CHECK_WRITE */

#ifdef CO_EVENT_SDO_SERVER_CHECK_WRITE_CNT
	/* call indication to execute */
	cnt = sdoServerCheckWriteTableCnt;
	while (cnt > 0u)  {
		cnt--;
		/* call user indication */
#ifdef CO_SRV_CHECK_WRITE_IND_SIZE
		(void)sdoServerCheckWriteTable[cnt](CO_TRUE, sdoNr,
				index, subIndex, pData, size);
#else /* CO_SRV_CHECK_WRITE_IND_SIZE */
		(void)sdoServerCheckWriteTable[cnt](CO_TRUE, sdoNr,
				index, subIndex, pData);
#endif /* CO_SRV_CHECK_WRITE_IND_SIZE */
	}
#endif /* CO_EVENT_SDO_SERVER_CHECK_WRITE_CNT */

#ifdef CO_EVENT_STATIC_SDO_SERVER_CHECK_WRITE
# ifdef CO_SRV_CHECK_WRITE_IND_SIZE
	(void)coEventSdoServerCheckWriteInd(CO_TRUE, sdoNr,
			index, subIndex, pData, size);
# else /* CO_SRV_CHECK_WRITE_IND_SIZE */
	(void)coEventSdoServerCheckWriteInd(CO_TRUE, sdoNr,
			index, subIndex, pData);
# endif /* CO_SRV_CHECK_WRITE_IND_SIZE */
#endif /* CO_EVENT_STATIC_SDO_SERVER_CHECK_WRITE */

#ifdef CO_SRV_CHECK_WRITE_IND_SIZE
#else /* CO_SRV_CHECK_WRITE_IND_SIZE */
(void)size;
#endif /* CO_SRV_CHECK_WRITE_IND_SIZE */

	(void)sdoNr;
	(void)index;
	(void)subIndex;
	(void)pData;

	return(retVal);
}


/***************************************************************************/
/**
* \internal
*
* \brief icoSdoCheckUserWriteInd - check user write indication
*
* \return RET_T
*
*/
RET_T icoSdoCheckUserWriteInd(
		const CO_SDO_SERVER_T	*pSdo		/* pointer to sdo */
	)
{
RET_T	retVal = RET_OK;
UNSIGNED8	splitInd = 0u;

#ifdef CO_EVENT_SDO_SERVER_WRITE_CNT
UNSIGNED8	cnt;

	/* check all indications for correct return value */
	cnt = sdoServerWriteTableCnt;
	while (cnt > 0u)  {
		cnt--;

		/* check user indication */
		retVal = sdoServerWriteTable[cnt](CO_FALSE, pSdo->sdoNr,
				pSdo->index, pSdo->subIndex);
		if (retVal != RET_OK)  {
			if (retVal == RET_SDO_SPLIT_INDICATION)  {
				splitInd++;
			} else {
				return(retVal);
			}
		}
	}
#endif /* CO_EVENT_SDO_SERVER_WRITE_CNT */

#ifdef CO_EVENT_STATIC_SDO_SERVER_WRITE
	retVal = coEventSdoServerWriteInd(CO_FALSE, pSdo->sdoNr,
			pSdo->index, pSdo->subIndex);
	if (retVal != RET_OK) {
		if (retVal == RET_SDO_SPLIT_INDICATION)  {
			splitInd++;
		} else {
			return(retVal);
		}
	}
#endif /* CO_EVENT_STATIC_SDO_SERVER_WRITE */

#ifdef CO_EVENT_SDO_SERVER_WRITE_CNT
	/* call indication to execute */
	cnt = sdoServerWriteTableCnt;
	while (cnt > 0u)  {
		cnt--;
		/* call user indication */
		(void)sdoServerWriteTable[cnt](CO_TRUE, pSdo->sdoNr,
				pSdo->index, pSdo->subIndex);
	}
#endif /* CO_EVENT_SDO_SERVER_WRITE_CNT */

#ifdef CO_EVENT_STATIC_SDO_SERVER_WRITE
	(void)coEventSdoServerWriteInd(CO_TRUE, pSdo->sdoNr, pSdo->index, pSdo->subIndex);
#endif /* CO_EVENT_STATIC_SDO_SERVER_WRITE */

	if (splitInd != 0u)  {
		return(RET_SDO_SPLIT_INDICATION);
	}

	return(retVal);
}


/***************************************************************************/
/**
* \internal
*
* \brief icoSdoDomainUserWriteInd - user domain write indication
*
* \return RET_T
*
*/
RET_T icoSdoDomainUserWriteInd(
		const CO_SDO_SERVER_T	*pSdo		/* pointer to sdo */
	)
{
RET_T retVal = RET_OK;

#if (defined(CO_EVENT_SSDO_DOMAIN_WRITE_CNT) || defined(CO_EVENT_STATIC_SSDO_DOMAIN_WRITE))
#else /* (defined(CO_EVENT_SSDO_DOMAIN_WRITE_CNT) || defined(CO_EVENT_STATIC_SSDO_DOMAIN_WRITE)) */
(void)pSdo;
#endif /* (defined(CO_EVENT_SSDO_DOMAIN_WRITE_CNT) || defined(CO_EVENT_STATIC_SSDO_DOMAIN_WRITE)) */

#ifdef CO_EVENT_SSDO_DOMAIN_WRITE_CNT
UNSIGNED8	cnt;

	/* check all indications for correct return value */
	cnt = sdoServerDomainWriteTableCnt;
	while (cnt > 0u)  {
		cnt--;

		retVal = sdoServerDomainWriteTable[cnt](pSdo->index, pSdo->subIndex,
				pSdo->transferedSize, pSdo->domainTransferedSize);
		if (retVal != RET_OK) {
			break;
		}
	}
#endif /* CO_EVENT_SSDO_DOMAIN_WRITE_CNT */

#ifdef CO_EVENT_STATIC_SSDO_DOMAIN_WRITE
	coEventSdoServerDomainWriteInd(pSdo->index, pSdo->subIndex,
				pSdo->transferedSize, pSdo->domainTransferedSize);
#endif /* CO_EVENT_STATIC_SSDO_DOMAIN_WRITE */


	return(retVal);
}


/***************************************************************************/
/**
* \internal
*
* \brief icoSdoDomainUserReadInd - user domain read indication
*
* \return RET_T
*
*/
RET_T icoSdoDomainUserReadInd(
		const CO_SDO_SERVER_T	*pSdo		/* pointer to sdo */
    )
{
RET_T retVal = RET_OK;
#ifdef CO_EVENT_SSDO_DOMAIN_READ_CNT
UNSIGNED8    cnt;
#endif /* CO_EVENT_SSDO_DOMAIN_READ_CNT */

#if (defined(CO_EVENT_SSDO_DOMAIN_READ_CNT) || defined(CO_EVENT_STATIC_SSDO_DOMAIN_READ))
UNSIGNED32		domainBufSize;	/* domain buffer size */

	if ((pSdo->objSize != 0u) && (pSdo->objSize < (7u * CO_SSDO_DOMAIN_CNT)))  {
		domainBufSize = pSdo->objSize;
	} else {
		domainBufSize = 7u * CO_SSDO_DOMAIN_CNT;
	}
#else /* (defined(CO_EVENT_SSDO_DOMAIN_READ_CNT) || defined(CO_EVENT_STATIC_SSDO_DOMAIN_READ)) */
(void)pSdo;
#endif /* (defined(CO_EVENT_SSDO_DOMAIN_READ_CNT) || defined(CO_EVENT_STATIC_SSDO_DOMAIN_READ)) */


#ifdef CO_EVENT_SSDO_DOMAIN_READ_CNT
	/* check all indications for correct return value */
	cnt = sdoServerDomainReadTableCnt;
	while (cnt > 0u)  {
		cnt--;
#ifdef CO_SDO_SPLIT_INDICATION
		retVal = sdoServerDomainReadTable[cnt](pSdo->index, pSdo->subIndex,
			domainBufSize, pSdo->domainTransferedSize);
		if (retVal != RET_OK)  {
			return(retVal);
		}
#else /* CO_SDO_SPLIT_INDICATION*/
		sdoServerDomainReadTable[cnt](pSdo->index, pSdo->subIndex,
			domainBufSize, pSdo->domainTransferedSize);
#endif /* CO_SDO_SPLIT_INDICATION*/
	}
#endif /* CO_EVENT_SSDO_DOMAIN_READ_CNT */

#ifdef CO_EVENT_STATIC_SSDO_DOMAIN_READ
# ifdef CO_SDO_SPLIT_INDICATION
	retVal = coEventSdoServerDomainReadInd(pSdo->index, pSdo->subIndex,
		domainBufSize, pSdo->domainTransferedSize);
	if (retVal != RET_OK)  {
		return(retVal);
	}
# else /* CO_SDO_SPLIT_INDICATION */
	coEventSdoServerDomainReadInd(pSdo->index, pSdo->subIndex,
		domainBufSize, pSdo->domainTransferedSize);
# endif /* CO_SDO_SPLIT_INDICATION */
#endif /* CO_EVENT_STATIC_SSDO_DOMAIN_READ */


    return(retVal);
}


/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/**
* \internal
*
* \brief icoSdoGetObjectAddr - get sdo server object address
*
* \return pointer to address
*
*/
void *icoSdoGetObjectAddr(
		UNSIGNED16		sdoNr,			/* sdo number */
		UNSIGNED8		subIndex		/* subindex */
	)
{
void	*pAddr = NULL;
CO_SDO_SERVER_T	*pSdo;

	pSdo = icoSdoServerPtr(sdoNr);
	if (pSdo == NULL)  {
		return(NULL);
	}

	switch (subIndex)  {
		case 1:
			/* copy from cob handler */
			pAddr = (void *)&pSdo->recCobId;
			break;
		case 2:
			/* copy from cob handler */
			pAddr = (void *)&pSdo->trCobId;
			break;
		case 3:
			pAddr = (void *)&pSdo->node;
			break;
		default:
			break;
	}

	return(pAddr);
}


/******************************************************************************/
/**
* \internal
*
* \brief icoSdoCheckObjLimitNode - check object limits node entry
*
*
* \return RET_T
*/
RET_T icoSdoCheckObjLimitNode(
		UNSIGNED16		sdoNr		/* sdo number */
	)
{
RET_T	retVal = RET_OK;
CO_SDO_SERVER_T	*pSdo;

	pSdo = icoSdoServerPtr(sdoNr);
	if (pSdo == NULL)  {
		return(RET_SDO_TRANSFER_NOT_SUPPORTED);
	}

	return(retVal);
}


/******************************************************************************/
/**
* \internal
*
* \brief icoSdoCheckObjLimitCobId - check cob-id
*
*
* \return RET_T
*/
RET_T icoSdoCheckObjLimitCobId(
		UNSIGNED16		sdoNr,		/* sdo number */
		UNSIGNED8		subIndex,	/* sub index */
		UNSIGNED32		canId		/* pointer to receive data */
	)
{
RET_T	retVal = RET_OK;
CO_SDO_SERVER_T	*pSdo;

	pSdo = icoSdoServerPtr(sdoNr);
	if (pSdo == NULL)  {
		return(RET_SDO_TRANSFER_NOT_SUPPORTED);
	}

	if (subIndex == 1u)  {
		if ((canId & CO_COB_VALID_MASK) == 0u)  {
			/* new cobid is valid, only allowed if cob was disabled before*/
			if ((pSdo->recCobId & CO_COB_VALID_MASK) != CO_COB_INVALID)  {
				return(RET_SDO_INVALID_VALUE);
			}
			if (icoCheckRestrictedCobs(canId, 0x600ul, 0x600ul) == CO_TRUE)  {
				return(RET_SDO_INVALID_VALUE);
			}
		}
	} else 
	if (subIndex == 2u)  {
		if ((canId & CO_COB_VALID_MASK) == 0u)  {
			/* new cobid is valid, only allowed if cob was disabled before*/
			if ((pSdo->trCobId & CO_COB_VALID_MASK) != CO_COB_INVALID)  {
				return(RET_SDO_INVALID_VALUE);
			}
			if (icoCheckRestrictedCobs(canId, 0x580ul, 0x580ul) == CO_TRUE)  {
				return(RET_SDO_INVALID_VALUE);
			}
		}
	} else {
		retVal = RET_SUBIDX_NOT_FOUND;
	}

	return(retVal);
}


/******************************************************************************/
/**
* \internal
*
* \brief icoSdoObjChanged - sdo object was changed
*
*/
RET_T icoSdoObjChanged(
		UNSIGNED16		sdoNr,		/* sdo number */
		UNSIGNED8		subIndex	/* subindex */
	)
{
RET_T		retVal = RET_OK;
CO_SDO_SERVER_T	*pSdo;

/*printf("sdo object changed indication %x:%d\n", 0x1200 + sdoNr- 1, subIndex); */
	pSdo = icoSdoServerPtr(sdoNr);
	if (pSdo == NULL)  {
		return(RET_SDO_TRANSFER_NOT_SUPPORTED);
	}

	switch (subIndex)  {
		case 1u:
			retVal = icoCobSet(pSdo->recCob, pSdo->recCobId,
				CO_COB_RTR_NONE, 8u);
			break;
		case 2u:
			retVal = icoCobSet(pSdo->trCob, pSdo->trCobId,
				CO_COB_RTR_NONE, 8u);
			break;
		default:
			break;
	}

	return(retVal);
}


/******************************************************************************/
/**
* \internal
*
* \brief getSdoServer - get SDO server pointer
*
* \return none
*
*/
static CO_SDO_SERVER_T	*getSdoServer(
		UNSIGNED16		sdoIdx		/* index at sdo server list */
	)
{
CO_SDO_SERVER_T	*pSdo;

	if (sdoIdx > sdoServerCnt)  {
		return(NULL);
	}

	pSdo = &sdoServer[sdoIdx];

	return (pSdo);
}


/******************************************************************************/
/**
* \internal
*
* \brief coResetSdoServer - reset comm for sdo server
*
* \return none
*
*/
void icoSdoServerReset(
		void	/* no parameter */
	)
{
UNSIGNED16	cnt;
CO_SDO_SERVER_T	*pSdo;

	for (cnt = 0u; cnt < sdoServerCnt; cnt++)  {
		pSdo = getSdoServer(cnt);
		if (pSdo == NULL)  {
			break;
		}

		pSdo->state = CO_SDO_STATE_FREE;

		/* ignore temporary cob-ids */
		if ((pSdo->recCobId & CO_SDO_DYN_BIT) != 0ul)  {
			pSdo->recCobId = CO_COB_INVALID;
			pSdo->trCobId = CO_COB_INVALID;
		}

		(void)icoCobSet(pSdo->recCob, pSdo->recCobId,
				CO_COB_RTR_NONE, 8u);
		(void)icoCobSet(pSdo->trCob, pSdo->trCobId,
				CO_COB_RTR_NONE, 8u);
	}
}


#ifdef CANOPEN_SUPPORTED
/******************************************************************************/
/**
* \internal
*
* \brief coSdoServerSetDefault - reset comm for sdo server
*
* \return none
*
*/
void icoSdoServerSetDefaultValue(
		void	/* no parameter */
	)
{
UNSIGNED16	cnt;
CO_SDO_SERVER_T	*pSdo;

	for (cnt = 0u; cnt < sdoServerCnt; cnt++)  {
		pSdo = getSdoServer(cnt);
		if (pSdo == NULL)  {
			break;
		}
		if (pSdo->sdoNr == 1u)  {
			/* calculate cobs for 1. server sdo */
			pSdo->recCobId = 0x600ul + coNmtGetNodeId();
			pSdo->trCobId = 0x580ul + coNmtGetNodeId();
		} else {
			/* disable sdo */
			pSdo->recCobId |= CO_COB_INVALID;
			pSdo->trCobId |= CO_COB_INVALID;
		}
	}
}
#endif /* CANOPEN_SUPPORTED */


/******************************************************************************/
/*
* \brief icoSdoServerVarInit - init sdo server variables
*
*/
void icoSdoServerVarInit(
		CO_CONST UNSIGNED8	*pList		/* line counts */
	)
{
(void)pList;

	{
		sdoServerCnt = 0u;
	}

	memset(&sdoServer[0], 0, sizeof(sdoServer));

#ifdef CO_EVENT_SDO_SERVER_READ_CNT
	sdoServerReadTableCnt = 0u;
#endif /* CO_EVENT_SDO_SERVER_READ_CNT */

#ifdef CO_EVENT_SDO_SERVER_WRITE_CNT
	sdoServerWriteTableCnt = 0u;
#endif /* CO_EVENT_SDO_SERVER_WRITE_CNT */

#ifdef CO_EVENT_SDO_SERVER_CHECK_WRITE_CNT
	sdoServerCheckWriteTableCnt = 0u;
#endif /* CO_EVENT_SDO_SERVER_CHECK_WRITE_CNT */

#ifdef CO_EVENT_SSDO_DOMAIN_WRITE_CNT
	sdoServerDomainWriteTableCnt = 0u;
#endif /* CO_EVENT_SSDO_DOMAIN_WRITE_CNT */

#ifdef CO_EVENT_SSDO_DOMAIN_READ_CNT
    sdoServerDomainReadTableCnt = 0u;
#endif /* CO_EVENT_SSDO_DOMAIN_READ_CNT */
}


/******************************************************************************/
/**
* \brief coInitSdoServer - init sdo server functionality
*
* This function initializes the given sdo server.
* If the sdo number = 1,
* then the default COB-IDs are set for this SDO.
*
* \return RET_T
*/
RET_T coSdoServerInit(
		UNSIGNED8		sdoServerNr		/**< sdo server number */
	)
{
CO_SDO_SERVER_T	*pSdo;

	if (sdoServerCnt >= CO_SDO_SERVER_CNT)  {
		return(RET_INVALID_PARAMETER);
	}

	pSdo = getSdoServer((UNSIGNED16)sdoServerCnt);
	if (pSdo == NULL)  {
		return(RET_INVALID_PARAMETER);
	}

	pSdo->sdoNr = sdoServerNr;

	pSdo->trCob =
		icoCobCreate(CO_COB_TYPE_TRANSMIT, CO_SERVICE_SDO_SERVER,
			(UNSIGNED16)sdoServerCnt);
	if (pSdo->trCob == 0xffffu)  {
		return(RET_NO_COB_AVAILABLE);
	}
	pSdo->recCob =
		icoCobCreate(CO_COB_TYPE_RECEIVE, CO_SERVICE_SDO_SERVER,
			(UNSIGNED16)sdoServerCnt);
	if (pSdo->recCob == 0xffffu)  {
		return(RET_NO_COB_AVAILABLE);
	}

	sdoServerCnt++;

	return(RET_OK);
}

#endif /* CO_SDO_SERVER_CNT */
