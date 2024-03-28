/*
* co_sdoblockserver.c - contains sdo block routines for server
*
* Copyright (c) 2013-2022 emotas embedded communication GmbH
*-------------------------------------------------------------------------------
* $Id: co_sdoblockserver.c 40827 2022-05-30 12:59:49Z boe $
*
*
*-------------------------------------------------------------------------------
*
*
*/

/******************************************************************************/
/**
* \brief sdo block routines
*
* \file co_sdoblockserver.c
* contains sdo block transfer routines for server
*
*/


/* header of standard C - libraries
------------------------------------------------------------------------------*/
#include <string.h>

/* header of project specific types
------------------------------------------------------------------------------*/
#include <gen_define.h>

#ifdef CO_SDO_BLOCK
#include <co_odaccess.h>
#include <co_sdo.h>
#include <co_drv.h>
#include <co_timer.h>
#include <co_nmt.h>
#include "ico_cobhandler.h"
#include "ico_queue.h"
#include "ico_odaccess.h"
#include "ico_nmt.h"
#include "ico_event.h"
#include "ico_indication.h"
#include "ico_sdoserver.h"
#include "ico_sdo.h"

/* constant definitions
------------------------------------------------------------------------------*/

/* local defined data types
------------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
------------------------------------------------------------------------------*/

/* list of global defined functions
------------------------------------------------------------------------------*/

/* list of local defined functions
------------------------------------------------------------------------------*/
static void sdoServerBlockTransmit(void *pData);
static RET_T sdoServerBlockReadEnd(CO_SDO_SERVER_T *pSdo);


/* external variables
------------------------------------------------------------------------------*/

/* global variables
------------------------------------------------------------------------------*/

/* local defined variables
------------------------------------------------------------------------------*/



/******************************************************************************/
/**
* \internal
*
* \brief icoSdoServerBlockReadInit
*
* \return RET_T
*
*/
RET_T icoSdoServerBlockReadInit(
		CO_SDO_SERVER_T	*pSdo,			/* pointer to sdo */
		const CO_CAN_REC_MSG_T	*pRecData	/* pointer to received data */
	)
{
RET_T	retVal;
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */
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

	/* save blocksize */
	pSdo->blockSize = pRecData->data[4];
	if (pSdo->blockSize > 127u)  {
		return(RET_SDO_WRONG_BLOCKSIZE);
	}

	/* call all user indications */
	retVal = icoSdoCheckUserReadInd(pSdo->sdoNr, pSdo->index, pSdo->subIndex);
#ifdef CO_SDO_SPLIT_INDICATION
	pSdo->split = CO_FALSE;
	if (retVal == RET_SDO_SPLIT_INDICATION) {
		/* split indikation ? */

		pSdo->split = CO_TRUE;
		retVal = RET_OK;
	}
#endif /* CO_SDO_SPLIT_INDICATION */

	if (retVal != RET_OK)  {
		return(retVal);
	}

	pSdo->objSize = coOdGetObjSize(pSdo->pObjDesc);
	
	trData[1] = (UNSIGNED8)(pSdo->index & 0xffu);
	trData[2] = (UNSIGNED8)(pSdo->index >> 8);
	trData[3] = pSdo->subIndex;

	memset(&trData[4], 0, 4u);

	/* segmented transfer */
	trData[0] = CO_SDO_SCS_BLOCK_UPLOAD | CO_SDO_SCS_BLOCK_UL_SIZE;
# ifdef CO_SDO_BLOCK_CRC
	/* with CRC ? */
	if ((pRecData->data[0] & CO_SDO_CCS_BLOCK_UL_CRC) != 0u)  {
		trData[0] |= CO_SDO_SCS_BLOCK_UL_CRC;
		pSdo->blockCrcUsed = CO_TRUE;
		pSdo->blockCrc = 0u;
		pSdo->blockCrcSize = 0u;
	} else {
		pSdo->blockCrcUsed = CO_FALSE;
	}
# endif /* CO_SDO_BLOCK_CRC */

	coNumMemcpyUnpack(&trData[4], &pSdo->objSize, 4u, CO_ATTR_NUM, 0u);
	pSdo->transferedSize = 0u;

	/* transmit answer */
	retVal = icoTransmitMessage(pSdo->trCob, &trData[0], 0u);
	if (retVal == RET_OK)  {
		pSdo->state = CO_SDO_STATE_BLOCK_UPLOAD_INIT;
	}

#ifdef CO_EVENT_SSDO_DOMAIN_READ
	if (pSdo->pObjDesc->dType == CO_DTYPE_DOMAIN) {
		pSdo->domainTransfer = CO_TRUE;
		pSdo->domainTransferedSize = 0u;
		/* domain indication */
		retVal = icoSdoDomainUserReadInd(pSdo);
#ifdef CO_SDO_SPLIT_INDICATION
		if (retVal == RET_SDO_SPLIT_INDICATION)  {
			pSdo->split = CO_TRUE;
			retVal = RET_OK;
		}
#endif /* CO_SDO_SPLIT_INDICATION */
	}
	else {
		pSdo->domainTransfer = CO_FALSE;
	}
#endif /* CO_EVENT_SSDO_DOMAIN_READ */

	return(retVal);
}


/******************************************************************************/
/**
* \internal
*
* \brief icoSdoServerBlockRead
*
* \return RET_T
*
*/
RET_T icoSdoServerBlockRead(
		CO_SDO_SERVER_T	*pSdo			/* pointer to sdo */
	)
{
RET_T	retVal;
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */
UNSIGNED32	size;

	/* don't start it yet if buffer is 80% filled ... */
	if (icoQueueGetTransBufFillState() > 80u)  {
		retVal = icoEventStart(&pSdo->blockEvent,
			sdoServerBlockTransmit, pSdo); /*lint !e960 */
		/* Derogation MisraC2004 R.16.9 function identifier used without '&'
		 * or parenthesized parameter */

		return(retVal);
	}

	/* first transfer for subblock ? */
	if (pSdo->state == CO_SDO_STATE_BLOCK_UPLOAD_INIT)  {
		/* yes, reset counter */
		pSdo->seqNr = 1u;
		pSdo->state = CO_SDO_STATE_BLOCK_UPLOAD;

		/* BLOCK_TEST U1 Start */
		/* pSdo->seqNr = 2; */
		/* BLOCK_TEST U1 End */
	}
	if (pSdo->state != CO_SDO_STATE_BLOCK_UPLOAD)  {
		return(RET_OK);
	}
#ifdef CO_SDO_SPLIT_INDICATION
	if (pSdo->split == CO_TRUE)  {
		return(icoEventStart(&pSdo->blockEvent, sdoServerBlockTransmit, pSdo));	/*lint !e960 function identifier used without '&' or parenthesized parameter list */
	}
#endif /* CO_SDO_SPLIT_INDICATION */

	trData[0] = pSdo->seqNr;
	memset(&trData[1], 0, 7u);

	/* last transfer ? */
	if (pSdo->objSize > 7u)  {
		size = 7u;
	} else {
		/* last transfer */
		size = pSdo->objSize;
		trData[0] |= CO_SDO_SCS_BLOCK_UL_LAST;
		pSdo->state = CO_SDO_STATE_BLOCK_UPLOAD_LAST;
	}
	pSdo->objSize -= size;

	/* get data */
	if (size > 0u)  {
		retVal = icoOdGetObj(pSdo->pObjDesc, pSdo->subIndex, &trData[1],
			pSdo->transferedSize, size, CO_FALSE);
		if (retVal != RET_OK) {
			return(retVal);
		}
	}

	/* transmit answer */
	retVal = icoTransmitMessage(pSdo->trCob, &trData[0], 0u);
	if (retVal != RET_OK)  {
		/* transmit buffer full - try again */
		retVal = icoEventStart(&pSdo->blockEvent,
			sdoServerBlockTransmit, pSdo); /*lint !e960 */
		/* Derogation MisraC2004 R.16.9 function identifier used without '&'
		 * or parenthesized parameter */

		return(retVal);
	}

# ifdef CO_SDO_BLOCK_CRC
	/* calculate CRC, but ignore repeated seq blocks */
	if ((pSdo->blockCrcUsed == CO_TRUE)
	 && (pSdo->transferedSize >= pSdo->blockCrcSize))  {
		pSdo->blockCrc = icoSdoCrcCalc(pSdo->blockCrc, &trData[1], size);
		pSdo->blockCrcSize += size;
	}
# endif /* CO_SDO_BLOCK_CRC */

	/* apply size information */
	pSdo->transferedSize += size;
	pSdo->seqNr++;

#ifdef CO_EVENT_SSDO_DOMAIN_READ
	pSdo->domainTransferedSize += size;

	/* domain transfer ? */
	if ((pSdo->domainTransfer == CO_TRUE)
		&& ((pSdo->transferedSize % (7u * CO_SSDO_DOMAIN_CNT)) == 0u)) {
		/* domain indication */
		retVal = icoSdoDomainUserReadInd(pSdo);
		/* reset domain pointer */
		pSdo->transferedSize = 0u;
# ifdef CO_SDO_BLOCK_CRC
		pSdo->blockCrcSize = 0u;
# endif /* CO_SDO_BLOCK_CRC */
# ifdef CO_SDO_SPLIT_INDICATION
		pSdo->split = CO_FALSE;
		if (retVal == RET_SDO_SPLIT_INDICATION)  {
			pSdo->split = CO_TRUE;
			retVal = RET_OK;
		}
# endif /* CO_SDO_SPLIT_INDICATION */
	}
#endif /* CO_EVENT_SSDO_DOMAIN_READ */

	/* is last transfer */
	if ((trData[0] & CO_SDO_SCS_BLOCK_UL_LAST) != 0u)  {
		pSdo->state = CO_SDO_STATE_BLOCK_UPLOAD_LAST;
		return(RET_OK);
	}

	/* block count reached ? */
	if (pSdo->seqNr > pSdo->blockSize)  {
		/* await acq from client */
		pSdo->state = CO_SDO_STATE_BLOCK_UPLOAD_RESP;
	} else {
		/* transmit next message */
		retVal = icoEventStart(&pSdo->blockEvent,
			sdoServerBlockTransmit, pSdo); /*lint !e960 */
		/* Derogation MisraC2004 R.16.9 function identifier used without '&'
		 * or parenthesized parameter */
	}

	return(retVal);
}


/******************************************************************************/
/**
* \internal
*
* \brief sdoServerBlockTransmit
*
* \return RET_T
*
*/
static void sdoServerBlockTransmit(
		void			*pData
	)
{
CO_SDO_SERVER_T	*pSdo = pData;
RET_T	retVal;

	retVal = icoSdoServerBlockRead(pSdo);
	if (retVal != RET_OK)  {
		/* abort sdo transfer */
		icoSdoServerAbort(pSdo, retVal, CO_FALSE);
	}
}


/******************************************************************************/
/**
* \internal
*
* \brief icoSdoServerBlockRead
*
* \return RET_T
*
*/
RET_T icoSdoServerBlockReadCon(
		CO_SDO_SERVER_T	*pSdo,			/* pointer to sdo */
		const CO_CAN_REC_MSG_T	*pRecData	/* pointer to received data */
	)
{
UNSIGNED32	size;
RET_T	retVal = RET_OK;

	/* acq for which segment */
	if ((pRecData->data[1]) == pSdo->blockSize)  {
		/* all segments were received succesfully */
		pSdo->seqNr = 1u;

		/* all data transferred ? */
		if (pSdo->objSize == 0u)  {
			retVal = sdoServerBlockReadEnd(pSdo);
			return(retVal);
		}

	} else {
		/* acq == 0 ? */
		if ((pRecData->data[1]) == 0u)  {
			/* nothing was received, repeat complete last block */
			pSdo->seqNr = 1u;
			size = (pSdo->transferedSize / (pSdo->blockSize * 7ul)) * pSdo->blockSize;
		} else {
			/* wrong block number ? */
			if (pRecData->data[1] > 127u)  {
				return(RET_SDO_WRONG_SEQ_NR);
			}

			/* check, if all data was transmitted and seqNr fit them */
			if (pSdo->objSize == 0u)  {
				if (pRecData->data[1] == (pSdo->seqNr - 1u)) {
					retVal = sdoServerBlockReadEnd(pSdo);
					return(retVal);
				}
			}

			/* some blocks were received but not all, repeat them */
			pSdo->seqNr = pRecData->data[1] + 1u;
			size = ((pSdo->transferedSize / (pSdo->blockSize * 7ul)) * pSdo->blockSize)
				+ (pSdo->seqNr * 7ul);
		}
		pSdo->transferedSize = size;
	}

	pSdo->blockSize = pRecData->data[2];

	pSdo->state = CO_SDO_STATE_BLOCK_UPLOAD;
	retVal = icoSdoServerBlockRead(pSdo);


	return(retVal);
}


/******************************************************************************/
/**
* \internal
*
* \brief sdoServerBlockReadEnd
*
* \return RET_T
*
*/
static RET_T sdoServerBlockReadEnd(
		CO_SDO_SERVER_T	*pSdo			/* pointer to sdo */
	)
{
RET_T	retVal;
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */

	trData[0] = CO_SDO_SCS_BLOCK_UPLOAD | CO_SDO_SCS_BLOCK_SS_UL_END;
	
	if ((UNSIGNED8)(pSdo->transferedSize % 7u) > 0u)  {
		trData[0] |= ((7u - (UNSIGNED8)(pSdo->transferedSize % 7u)) << 2);
	}

	memset(&trData[1], 0, 7u);

# ifdef CO_SDO_BLOCK_CRC
	if (pSdo->blockCrcUsed == CO_TRUE) {
		/* BLOCK_TEST U2 Start */
		/* pSdo->blockCrc = 0; */
		/* BLOCK_TEST U2 End */

		trData[1] = (UNSIGNED8)(pSdo->blockCrc & 0xffu);
		trData[2] = (UNSIGNED8)((pSdo->blockCrc >> 8) & 0xffu);
	}
# endif /* CO_SDO_BLOCK_CRC */

	/* transmit answer */
	retVal = icoTransmitMessage(pSdo->trCob, &trData[0], 0u);
	if (retVal != RET_OK)  {
		return(retVal);
	}

	pSdo->state = CO_SDO_STATE_FREE;

	return(retVal);
}


/******************************************************************************/
/******************************************************************************/
/**
* \internal
*
* \brief icoSdoServerBlockWriteInit
*
* \return RET_T
*
*/
RET_T icoSdoServerBlockWriteInit(
		CO_SDO_SERVER_T	*pSdo,			/* pointer to sdo */
		const CO_CAN_REC_MSG_T	*pRecData	/* pointer to received data */
	)
{
RET_T	retVal;
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */
UNSIGNED16	objAttr;
UNSIGNED32	len = 0u;

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

	/* size indicated ? */
	if ((pRecData->data[0] & CO_SDO_CCS_BLOCK_DL_SIZE) != 0u)  {
		(void)coNumMemcpyPack(&len, &pRecData->data[4], 4u, CO_ATTR_NUM, 0u);
	} else {
		len = pSdo->objSize;
	}

#ifdef CO_SDO_SPLIT_INDICATION
	pSdo->split = CO_FALSE;
#endif /* CO_SDO_SPLIT_INDICATION */

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
	retVal = icoOdCheckObjLimits(pSdo->pObjDesc, &pRecData->data[4]);
	if (retVal != RET_OK)  {
		return(retVal);
	}

	/* call all user indications */
	retVal = icoSdoCheckUserCheckWriteInd(pSdo->sdoNr, pSdo->index,
			pSdo->subIndex, &pRecData->data[4], len);
	if (retVal != RET_OK)  {
		/* return error */
		return(retVal);
	}

	trData[0] = CO_SDO_SCS_BLOCK_DOWNLOAD /* | CO_SDO_SCS_BLOCK_SS_DL_INIT*/;

#ifdef CO_SDO_BLOCK_CRC
	/* use CRC ? */
	if ((pRecData->data[0] & CO_SDO_CCS_BLOCK_DL_CRC) != 0u)  {
		trData[0] |= CO_SDO_SCS_BLOCK_DL_CRC;
		pSdo->blockCrcUsed = CO_TRUE;
		pSdo->blockCrc = 0u;
	} else {
		pSdo->blockCrcUsed = CO_FALSE;
	}
#endif /* CO_SDO_BLOCK_CRC */

	trData[1] = (UNSIGNED8)(pSdo->index & 0xffu);
	trData[2] = (UNSIGNED8)(pSdo->index >> 8u);
	trData[3] = pSdo->subIndex;

	pSdo->blockSize = CO_SDO_BLOCK_SIZE;

	/* BLOCK_TEST D1 Start */
	/* pSdo->blockSize = 0x81u; */
	/* BLOCK_TEST D1 End */

	trData[4] = pSdo->blockSize;
	memset(&trData[5], 0, 3u);

	/* transmit answer */
	retVal = icoTransmitMessage(pSdo->trCob, &trData[0], 0u);

	pSdo->state = CO_SDO_STATE_BLOCK_DOWNLOAD;
	pSdo->transferedSize = 0u;
	pSdo->seqNr = 1u;

#ifdef CO_EVENT_SSDO_DOMAIN_WRITE
	if (pSdo->pObjDesc->dType == CO_DTYPE_DOMAIN)  {	
		pSdo->domainTransfer = CO_TRUE;
		pSdo->domainTransferedSize = 0u;
	} else {
		pSdo->domainTransfer = CO_FALSE;
	}
#endif /* CO_EVENT_SSDO_DOMAIN_WRITE */

	return(retVal);
}


/******************************************************************************/
/**
* \internal
*
* \brief icoSdoServerBlockWrite
*
* \return RET_T
*
*/
RET_T icoSdoServerBlockWrite(
		CO_SDO_SERVER_T	*pSdo,				/* pointer to sdo */
		const CO_CAN_REC_MSG_T	*pRecData		/* pointer to received data */
	)
{
RET_T	retVal;
BOOL_T	changed;

	/* check for next correct block number */
	if ((pRecData->data[0] & (UNSIGNED8)~(UNSIGNED8)CO_SDO_CCS_BLOCK_DL_LAST)
			== pSdo->seqNr)  {

		pSdo->seqNr++;

		/* last message ? */
		if ((pRecData->data[0] & CO_SDO_CCS_BLOCK_DL_LAST) != 0u)  {
			/* yes */
			/* save data temporary until block end was received */
			memcpy(&pSdo->blockSaveData[0], &pRecData->data[1], 7u);

			/* send response to server */
			retVal = icoSdoServerBlockWriteConfirmation(pSdo);

			/* continue with block end sequence */
			pSdo->state = CO_SDO_STATE_BLOCK_DOWNLOAD_END;

			return(retVal);
		}

		/* received len > as internal buffer size */
		if ((pSdo->transferedSize + 7u) > pSdo->objSize)  {
			/* send abort */
			icoSdoServerAbort(pSdo, RET_OUT_OF_MEMORY, CO_TRUE);
			return(RET_OK);
		}

		/* put data */
		retVal = icoOdPutObj(pSdo->pObjDesc, pSdo->subIndex, &pRecData->data[1],
			pSdo->transferedSize, 7u, CO_FALSE, &changed);
		if (retVal != RET_OK)  {
			/* return error */
			return(retVal);
		}
		pSdo->transferedSize += 7u;

#ifdef CO_EVENT_SSDO_DOMAIN_WRITE
		pSdo->domainTransferedSize += 7u;

		/* domain transfer ? */
		if ((pSdo->domainTransfer == CO_TRUE)
		 && ((pSdo->transferedSize % (7u * CO_SSDO_DOMAIN_CNT)) == 0u))  {
			/* domain indication */
			retVal = icoSdoDomainUserWriteInd(pSdo);
			/* reset domain pointer */
			pSdo->transferedSize = 0u;
		}
#endif /* CO_EVENT_SSDO_DOMAIN_WRITE */

#ifdef CO_SDO_BLOCK_CRC
		/* calculate CRC */
		if (pSdo->blockCrcUsed == CO_TRUE)  {
			pSdo->blockCrc = icoSdoCrcCalc(pSdo->blockCrc, &pRecData->data[1], 7u);
		}
#endif /*  CO_SDO_BLOCK_CRC */
	} else {
		retVal = RET_OK;
	}

	/* block cnt reached ? */
	if ((pRecData->data[0] & (UNSIGNED8)~(UNSIGNED8)CO_SDO_CCS_BLOCK_DL_LAST)
			== pSdo->blockSize)  {
		/* send response to server */
		if (retVal == RET_SDO_SPLIT_INDICATION) {
			pSdo->state = CO_SDO_STATE_WR_BLOCK_SPLIT_INDICATION;
		} else {
			retVal = icoSdoServerBlockWriteConfirmation(pSdo);
		}

	}

	return(retVal);
}


/***************************************************************************/
/**
* \internal
*
* \brief icoSdoServerBlockWriteInit
*
* \return RET_T
*
*/
RET_T icoSdoServerBlockWriteConfirmation(
		CO_SDO_SERVER_T		*pSdo		/* pointer to sdo */
	)
{
RET_T	retVal;
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */

	trData[0] = CO_SDO_SCS_BLOCK_DOWNLOAD | CO_SDO_SCS_BLOCK_SS_DL_ACQ;
	trData[1] = pSdo->seqNr - 1u;
	trData[2] = pSdo->blockSize;
	memset(&trData[3], 0, 5u);

	/* BLOCK_TEST D2 Start */
	/* trData.data[1] = 0; */
	/* BLOCK_TEST D2 End */

	/* BLOCK_TEST D3 Start */
	/* trData.data[1] = 1; */
	/* BLOCK_TEST D3 End */

	/* BLOCK_TEST D4 Start */
	/* trData.data[1] = 0x80; */
	/* BLOCK_TEST D4 End */

	/* BLOCK_TEST D5 Start */
	/* trData.data[2] = 0x81; */
	/* BLOCK_TEST D5 End */

	/* transmit answer */
	retVal = icoTransmitMessage(pSdo->trCob, &trData[0], 0u);

	/* reset seqNr */
	if ((pSdo->seqNr - 1u) == pSdo->blockSize)  {
		pSdo->seqNr = 1u;

		/* call user indication */
/*		sdoServerDomainIndication(pSdo);*/
	}

	return(retVal);
}


/***************************************************************************/
/**
* \internal
*
* \brief icoSdoServerBlockWriteEnd
*
* \return RET_T
*
*/
RET_T icoSdoServerBlockWriteEnd(
		CO_SDO_SERVER_T	*pSdo,				/* pointer to sdo */
		const CO_CAN_REC_MSG_T	*pRecData		/* pointer to received data */
	)
{
RET_T	retVal;
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */
UNSIGNED8	len;
#ifdef CO_SDO_BLOCK_CRC
UNSIGNED16	clCrc;
#endif /* CO_SDO_BLOCK_CRC */
BOOL_T		changed;

	/* check for correct state */
	if (pSdo->state != CO_SDO_STATE_BLOCK_DOWNLOAD_END)  {
		return(RET_SERVICE_BUSY);
	}

	/* save data from last transfer */
	len = 7u - ((pRecData->data[0] >> 2) & 7u);
	retVal = icoOdPutObj(pSdo->pObjDesc, pSdo->subIndex, &pSdo->blockSaveData[0], 
			pSdo->transferedSize, (UNSIGNED32)len, CO_FALSE, &changed);
	if (retVal != RET_OK)  {
		return(retVal);
	}

#ifdef CO_SDO_BLOCK_CRC
	/* calculate CRC */
	if (pSdo->blockCrcUsed == CO_TRUE)  {
		pSdo->blockCrc = icoSdoCrcCalc(pSdo->blockCrc, &pSdo->blockSaveData[0],
			(UNSIGNED32)len);

		clCrc = (UNSIGNED16)((UNSIGNED16)pRecData->data[2] << 8) | pRecData->data[1];
		/* printf("CRC %x %x \n", pSdo->blockCrc, clCrc); */
		if (pSdo->blockCrc != clCrc)  {
			return(RET_SDO_CRC_ERROR);
		}
	}
#endif /*  CO_SDO_BLOCK_CRC */

	/* send answer */
	trData[0] = CO_SDO_SCS_BLOCK_DOWNLOAD | CO_SDO_SCS_BLOCK_SS_DL_END;
	memset(&trData[1], 0, 7u);

	/* transmit answer */
	retVal = icoTransmitMessage(pSdo->trCob, &trData[0], 0u);
	if (retVal != RET_OK)  {
		return(retVal);
	}

	pSdo->transferedSize += len;

#ifdef CO_EVENT_SSDO_DOMAIN_WRITE
	pSdo->domainTransferedSize += len;

	/* domain transfer ? */
	if (pSdo->domainTransfer == CO_TRUE)  {
		/* domain indication */
		(void)icoSdoDomainUserWriteInd(pSdo);

		/* set new size */
		(void)coOdSetObjSize(pSdo->pObjDesc, pSdo->domainTransferedSize);
	} else {
		(void)coOdSetObjSize(pSdo->pObjDesc, pSdo->transferedSize);
	}
#else /* CO_EVENT_SSDO_DOMAIN_WRITE */
	(void)coOdSetObjSize(pSdo->pObjDesc, pSdo->transferedSize);
#endif /* CO_EVENT_SSDO_DOMAIN_WRITE */

	/* user indication */
	retVal = icoSdoCheckUserWriteInd(pSdo);
	if (retVal == RET_OK)  {
		retVal = icoEventObjectChanged(pSdo->pObjDesc,
			pSdo->index, pSdo->subIndex, changed); /*lint !e960 */
		/* Derogation MisraC2004 R.16.9 function identifier used without '&'
		 * or parenthesized parameter */
	}

	/* transfer ready */
	pSdo->state = CO_SDO_STATE_FREE;

	return(retVal);
}


#ifdef CO_SDO_BLOCK_CRC
/***************************************************************************/
/**
* \internal
*
* \brief icoSdoCrcCalc - calculate CRC
*
*
* CRC CCITT: x16 + x12 + x5 + 1
* \return
*	CRC
*
*/
UNSIGNED16 icoSdoCrcCalc(
		UNSIGNED16	crc,			/* start value for CRC */
		const UNSIGNED8	*pData,		/* pointer to data for CRC */
		UNSIGNED32	len				/* number of bytes for calculation */
	)
{
static const UNSIGNED16 crc_tabccitt[256] = {
 0x0000u, 0x1021u, 0x2042u, 0x3063u, 0x4084u, 0x50a5u, 0x60c6u, 0x70e7u,
 0x8108u, 0x9129u, 0xa14au, 0xb16bu, 0xc18cu, 0xd1adu, 0xe1ceu, 0xf1efu,
 0x1231u, 0x0210u, 0x3273u, 0x2252u, 0x52b5u, 0x4294u, 0x72f7u, 0x62d6u,
 0x9339u, 0x8318u, 0xb37bu, 0xa35au, 0xd3bdu, 0xc39cu, 0xf3ffu, 0xe3deu,
 0x2462u, 0x3443u, 0x0420u, 0x1401u, 0x64e6u, 0x74c7u, 0x44a4u, 0x5485u,
 0xa56au, 0xb54bu, 0x8528u, 0x9509u, 0xe5eeu, 0xf5cfu, 0xc5acu, 0xd58du,
 0x3653u, 0x2672u, 0x1611u, 0x0630u, 0x76d7u, 0x66f6u, 0x5695u, 0x46b4u,
 0xb75bu, 0xa77au, 0x9719u, 0x8738u, 0xf7dfu, 0xe7feu, 0xd79du, 0xc7bcu,
 0x48c4u, 0x58e5u, 0x6886u, 0x78a7u, 0x0840u, 0x1861u, 0x2802u, 0x3823u,
 0xc9ccu, 0xd9edu, 0xe98eu, 0xf9afu, 0x8948u, 0x9969u, 0xa90au, 0xb92bu,
 0x5af5u, 0x4ad4u, 0x7ab7u, 0x6a96u, 0x1a71u, 0x0a50u, 0x3a33u, 0x2a12u,
 0xdbfdu, 0xcbdcu, 0xfbbfu, 0xeb9eu, 0x9b79u, 0x8b58u, 0xbb3bu, 0xab1au,
 0x6ca6u, 0x7c87u, 0x4ce4u, 0x5cc5u, 0x2c22u, 0x3c03u, 0x0c60u, 0x1c41u,
 0xedaeu, 0xfd8fu, 0xcdecu, 0xddcdu, 0xad2au, 0xbd0bu, 0x8d68u, 0x9d49u,
 0x7e97u, 0x6eb6u, 0x5ed5u, 0x4ef4u, 0x3e13u, 0x2e32u, 0x1e51u, 0x0e70u,
 0xff9fu, 0xefbeu, 0xdfddu, 0xcffcu, 0xbf1bu, 0xaf3au, 0x9f59u, 0x8f78u,
 0x9188u, 0x81a9u, 0xb1cau, 0xa1ebu, 0xd10cu, 0xc12du, 0xf14eu, 0xe16fu,
 0x1080u, 0x00a1u, 0x30c2u, 0x20e3u, 0x5004u, 0x4025u, 0x7046u, 0x6067u,
 0x83b9u, 0x9398u, 0xa3fbu, 0xb3dau, 0xc33du, 0xd31cu, 0xe37fu, 0xf35eu,
 0x02b1u, 0x1290u, 0x22f3u, 0x32d2u, 0x4235u, 0x5214u, 0x6277u, 0x7256u,
 0xb5eau, 0xa5cbu, 0x95a8u, 0x8589u, 0xf56eu, 0xe54fu, 0xd52cu, 0xc50du,
 0x34e2u, 0x24c3u, 0x14a0u, 0x0481u, 0x7466u, 0x6447u, 0x5424u, 0x4405u,
 0xa7dbu, 0xb7fau, 0x8799u, 0x97b8u, 0xe75fu, 0xf77eu, 0xc71du, 0xd73cu,
 0x26d3u, 0x36f2u, 0x0691u, 0x16b0u, 0x6657u, 0x7676u, 0x4615u, 0x5634u,
 0xd94cu, 0xc96du, 0xf90eu, 0xe92fu, 0x99c8u, 0x89e9u, 0xb98au, 0xa9abu,
 0x5844u, 0x4865u, 0x7806u, 0x6827u, 0x18c0u, 0x08e1u, 0x3882u, 0x28a3u,
 0xcb7du, 0xdb5cu, 0xeb3fu, 0xfb1eu, 0x8bf9u, 0x9bd8u, 0xabbbu, 0xbb9au,
 0x4a75u, 0x5a54u, 0x6a37u, 0x7a16u, 0x0af1u, 0x1ad0u, 0x2ab3u, 0x3a92u,
 0xfd2eu, 0xed0fu, 0xdd6cu, 0xcd4du, 0xbdaau, 0xad8bu, 0x9de8u, 0x8dc9u,
 0x7c26u, 0x6c07u, 0x5c64u, 0x4c45u, 0x3ca2u, 0x2c83u, 0x1ce0u, 0x0cc1u,
 0xef1fu, 0xff3eu, 0xcf5du, 0xdf7cu, 0xaf9bu, 0xbfbau, 0x8fd9u, 0x9ff8u,
 0x6e17u, 0x7e36u, 0x4e55u, 0x5e74u, 0x2e93u, 0x3eb2u, 0x0ed1u, 0x1ef0u
};
UNSIGNED16 tmp, x;
UNSIGNED32		i;

	i = 0u;
	while (i < len)  {
		x = *pData;
		x &= 0xffu;

		tmp = (crc >> 8) ^ x;
		crc = (UNSIGNED16)((crc & 0xffu) << 8) ^ crc_tabccitt[tmp];

		i++;
		pData++;
	}

	return(crc);
}
#endif /* CO_SDO_BLOCK_CRC */


#ifdef xxx
static void init_crcccitt_tab(void) {
int i, j;
UNSIGNED16 crc, c;
UNSIGNED16 crc_tabccitt[256];
#define	P_CCITT	0x1021

	for (i = 0; i < 256; i++) {
		crc = 0;
		c = ((UNSIGNED16)i) << 8;

		for (j=0; j < 8; j++) {

			if ((crc ^ c) & 0x8000 ) {
				crc = ( crc << 1 ) ^ P_CCITT;
            }  else  {
				crc = crc << 1;
			}
			c = c << 1;
		}
		crc_tabccitt[i] = crc;
	}

	for (i = 0; i < 16; i++)  {
		for (j = 0; j < 16; j++)  {
			printf(" 0x%04x,", crc_tabccitt[i * 8 + j]);
			if (j == 7)  printf("\n");
		}
		printf("\n");
	}
}
#endif /* xxx */


#ifdef xxx

main()
{
short int crc = 0;
char test[9];
int i;

	for (i = 0; i < 9; i++)  {
		test[i] = i + 0x31;
		crc = update_crc_ccitt(crc, test[i]);
printf("%x\n", crc);
	}

	printf("crc: %x\n", crc);
}
#endif /* xxx */

#endif /* CO_SDO_BLOCK */
