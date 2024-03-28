/*
* co_sdoblockclient.c - contains sdo block routines for client
*
* Copyright (c) 2013-2022 emotas embedded communication GmbH
*-------------------------------------------------------------------
* $Id: co_sdoblockclient.c 39658 2022-02-17 10:15:29Z boe $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \brief sdo block routines
*
* \file co_sdoblockclient.c
* contains sdo block transfer routines for client
*
*/


/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <string.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <gen_define.h>

#ifdef CO_SDO_CLIENT_CNT
# ifdef CO_SDO_BLOCK
#include <co_datatype.h>
#include <co_timer.h>
#include <co_sdo.h>
#include <co_drv.h>
#include <co_odaccess.h>
#include <co_nmt.h>
#include "ico_cobhandler.h"
#include "ico_queue.h"
#include "ico_odaccess.h"
#include "ico_nmt.h"
#include "ico_event.h"
#ifdef CO_SDO_NETWORKING
# include "ico_indication.h"
# include "ico_sdoserver.h"
#endif /* CO_SDO_NETWORKING */
#include "ico_sdo.h"
#include "ico_sdoclient.h"

/* constant definitions
---------------------------------------------------------------------------*/

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
static void sdoClientReadBlockconfirmation(CO_SDO_CLIENT_T *pSdo);
static void sdoClientWriteBlockEnd(CO_SDO_CLIENT_T *pSdo);
static void	sdoClientWriteBlock(void *pData);

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/


/***************************************************************************/
/**
* \internal
*
* \brief  icoSdoClientReadBlockInit
*
*
* \return none
*
*/
void icoSdoClientReadBlockInit(
		CO_SDO_CLIENT_T	*pSdo,				/* pointer to SDO */
		const CO_CAN_REC_MSG_T	*pRecData	/* pointer to receive data */
	)
{
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */
UNSIGNED32	len = 0u;

	if (pSdo->state != CO_SDO_CLIENT_STATE_BLK_UL_INIT)  {
		/* ignore message */
		return;
	}

	/* delete timer */
	(void)coTimerStop(&pSdo->timer);

#ifdef CO_SDO_BLOCK_CRC
	/* use CRC ? */
	if ((pRecData->data[0] & CO_SDO_SCS_BLOCK_UL_CRC) != 0u)  {
		pSdo->blockCrcUsed = CO_TRUE;
		pSdo->blockCrc = 0u;
	} else {
		pSdo->blockCrcUsed = CO_FALSE;
	}
#endif /* CO_SDO_BLOCK_CRC */

	/* size indicated ? */
	if ((pRecData->data[0] & CO_SDO_SCS_BLOCK_UL_SIZE) != 0u)  {
		/* segmented transfer, len in byte 4..7 */
		(void)coNumMemcpyPack(&len, &pRecData->data[4], 4u, CO_ATTR_NUM, 0u);
	} else {
		/* unspecified data length */
		len = pSdo->size;
	}

	/* received len > as internal buffer size */
	if (len > pSdo->size)  {
		/* send abort */
		icoSdoClientAbort(pSdo, RET_DATA_TYPE_MISMATCH);
		return;
	}

	trData[0] = CO_SDO_CCS_BLOCK_UPLOAD | CO_SDO_CCS_BLOCK_SC_UL_BLK;
	memset(&trData[1], 0, 7u);

	/* transmit request */
	(void)icoTransmitMessage(pSdo->trCob, &trData[0], 0u);

	/* start timer */
	(void)coTimerStart(&pSdo->timer, pSdo->timeOutVal,
			icoSdoClientTimeOut, pSdo, CO_TIMER_ATTR_ROUNDUP); /*lint !e960 */
	/* Derogation MisraC2004 R.16.9 function identifier used without '&'
	 * or parenthesized parameter */

	pSdo->size = len;
	pSdo->restSize = len;
	pSdo->seqNr = 1u;
	pSdo->state = CO_SDO_CLIENT_STATE_BLK_UL_BLK;
}


/***************************************************************************/
/**
* \internal
*
* \brief  icoSdoClientReadBlock
*
*
* \return none
*
*/
void icoSdoClientReadBlock(
		CO_SDO_CLIENT_T	*pSdo,				/* pointer to SDO */
		const CO_CAN_REC_MSG_T	*pRecData	/* pointer to receive data */
	)
{
UNSIGNED32	len;
UNSIGNED8	packOffset = 0u;

	/* delete timer */
	(void)coTimerStop(&pSdo->timer);

	/* check for next correct block number */
	if ((pRecData->data[0] & (UNSIGNED8)~(UNSIGNED8)CO_SDO_SCS_BLOCK_UL_LAST)
				== pSdo->seqNr)  {

		/* block cnt reached ? */
		if (pSdo->seqNr <= pSdo->blockSize)  {
			pSdo->seqNr++;
		}

		/* last message ? */
		if ((pRecData->data[0] & CO_SDO_SCS_BLOCK_UL_LAST) != 0u)  {
			/* yes */
			/* save data temporary until block end was received */
			memcpy(&pSdo->saveBlockData[0], &pRecData->data[1], 7u);

			/* send response to server */
			sdoClientReadBlockconfirmation(pSdo);

			/* continue with block end sequence */
			pSdo->state = CO_SDO_CLIENT_STATE_BLK_UL_END;

			return;
		}

		/* save data */
		if (pSdo->restSize > 7u) {
			len = 7u;
		} else {
			len = pSdo->restSize;
		}

		/* received len > as internal buffer size */
		if (len > pSdo->restSize)  {
			/* send abort */
			icoSdoClientAbort(pSdo, RET_DATA_TYPE_MISMATCH);
			return;
		}

#ifdef CO_CPU_DSP
		if (pSdo->domain == CO_TRUE)  {
			packOffset = pSdo->size % CO_CPU_DSP_BYTESIZE;
		}
#endif /* CO_CPU_DSP */
		(void)coNumMemcpyPack(pSdo->pData, &pRecData->data[1], len, pSdo->numeric,
			packOffset);
		pSdo->pData += len;
		pSdo->restSize -= len;

#ifdef CO_SDO_BLOCK_CRC
		if (pSdo->blockCrcUsed == CO_TRUE)  {
			pSdo->blockCrc = icoSdoCrcCalc(pSdo->blockCrc, &pRecData->data[1], len);
		}
#endif /* CO_SDO_BLOCK_CRC */
	}

	/* block cnt reached ? */
	if ((pRecData->data[0] & (UNSIGNED8)~(UNSIGNED8)CO_SDO_SCS_BLOCK_UL_LAST)
			== pSdo->blockSize)  {
		/* send response to server */
		sdoClientReadBlockconfirmation(pSdo);
	}

	/* start timer */
	(void)coTimerStart(&pSdo->timer, pSdo->timeOutVal,
			icoSdoClientTimeOut, pSdo,
			CO_TIMER_ATTR_ROUNDUP); /*lint !e960 */
	/* Derogation MisraC2004 R.16.9 function identifier used without '&'
	 * or parenthesized parameter */

}


/***************************************************************************/
/**
* \internal
*
* \brief  sdoClientReadBlockConfirmation
*
*
* \return none
*
*/
static void sdoClientReadBlockconfirmation(
		CO_SDO_CLIENT_T	*pSdo
	)
{
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */

	trData[0] = CO_SDO_CCS_BLOCK_UPLOAD | CO_SDO_CCS_BLOCK_SC_UL_CON;
	trData[1] = pSdo->seqNr - 1u;
	trData[2] = pSdo->blockSize;
	memset(&trData[3], 0, 5u);

	/* transmit request */
	(void)icoTransmitMessage(pSdo->trCob, &trData[0], 0u);

	if ((pSdo->seqNr - 1u) == pSdo->blockSize)  {
		pSdo->seqNr = 1u;
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief  icoSdoClientReadBlockEnd
*
*
* \return none
*
*/
void icoSdoClientReadBlockEnd(
		CO_SDO_CLIENT_T	*pSdo,				/* pointer to SDO */
		const CO_CAN_REC_MSG_T	*pRecData	/* pointer to receive data */
	)
{
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */
UNSIGNED8	packOffset = 0u;
UNSIGNED8	len;
#ifdef CO_SDO_BLOCK_CRC
UNSIGNED16	crc;
#endif /* CO_SDO_BLOCK_CRC */

	/* delete timer */
	(void)coTimerStop(&pSdo->timer);

	/* valid data from last transfer */
	len = 7u - ((pRecData->data[0] >> 2) & 0x7u);

#ifdef CO_CPU_DSP
	if (pSdo->domain == CO_TRUE)  {
		packOffset = pSdo->size % CO_CPU_DSP_BYTESIZE;
	}
#endif /* CO_DSP32 */
	(void)coNumMemcpyPack(pSdo->pData, &pSdo->saveBlockData[0], (UNSIGNED32)len,
			pSdo->numeric, packOffset);

#ifdef CO_SDO_BLOCK_CRC
	if (pSdo->blockCrcUsed == CO_TRUE)  {
		pSdo->blockCrc = icoSdoCrcCalc(pSdo->blockCrc, &pSdo->saveBlockData[0],
			(UNSIGNED32)len);
		crc = pRecData->data[1] | (UNSIGNED16)((UNSIGNED16)pRecData->data[2] << 8u);
		/* printf("crc %x %x\n", pSdo->blockCrc, crc); */
		if (pSdo->blockCrc != crc)  {
			/* wrong CRC, abort */
			icoSdoClientAbort(pSdo, RET_SDO_CRC_ERROR);
			return;
		}
	}
#endif /* CO_SDO_BLOCK_CRC */

	trData[0] = CO_SDO_CCS_BLOCK_UPLOAD | CO_SDO_CCS_BLOCK_SC_UL_END;
	memset(&trData[1], 0, 7u);

	/* transmit request */
	(void)icoTransmitMessage(pSdo->trCob, &trData[0], 0u);

	pSdo->state = CO_SDO_CLIENT_STATE_FREE;

	icoSdoClientUserInd(pSdo, CO_SDO_CLIENT_STATE_BLK_UL_END, 0u);
}


/***************************************************************************/
/**
* \internal
*
* \brief  icoSdoClientWriteBlockInit
*
*
* \return none
*
*/
void icoSdoClientWriteBlockInit(
		CO_SDO_CLIENT_T		*pSdo,			/* pointer to SDO */
		const CO_CAN_REC_MSG_T	*pRecData	/* pointer to receive data */
	)
{
	if (pSdo->state != CO_SDO_CLIENT_STATE_BLK_DL_INIT)  {
		/* ignore message */
		return;
	}

	/* delete timer */
	(void)coTimerStop(&pSdo->timer);

#ifdef CO_SDO_BLOCK_CRC
	/* use CRC ? */
	if ((pRecData->data[0] & CO_SDO_SCS_BLOCK_DL_CRC) != 0u)  {
		pSdo->blockCrcUsed = CO_TRUE;
		pSdo->blockCrc = icoSdoCrcCalc(0u, pSdo->pData, pSdo->size);
	} else {
		pSdo->blockCrcUsed = CO_FALSE;
	}
#endif /* CO_SDO_BLOCK_CRC */

	pSdo->blockSize = pRecData->data[4];
	if ((pSdo->blockSize == 0u) || (pSdo->blockSize > 127u))  {
		icoSdoClientAbort(pSdo, RET_SDO_WRONG_BLOCKSIZE);
		return;
	}

	pSdo->seqNr = 1u;

	sdoClientWriteBlock(pSdo);
}


/***************************************************************************/
/**
* \internal
*
* \brief  sdoClientWriteBlock
*
*
* \return none
*
*/
static void sdoClientWriteBlock(
		void			*pData
	)
{
CO_SDO_CLIENT_T	*pSdo = pData;
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */
UNSIGNED32	size;
UNSIGNED8	packOffset = 0u;
RET_T		retVal;

	trData[0] = pSdo->seqNr;
	memset(&trData[1], 0, 7u);

	/* last transfer ? */
	if ((pSdo->size - pSdo->restSize) > 7u)  {
		size = 7u;
	} else {
		/* last transfer */
		size = pSdo->size - pSdo->restSize;
		trData[0] |= CO_SDO_SCS_BLOCK_DL_LAST;

		pSdo->state = CO_SDO_CLIENT_STATE_BLK_DL_ACQ;
	}

# ifdef CO_CPU_DSP
	if (pSdo->domain == CO_TRUE)  {
		packOffset = pSdo->restSize % CO_CPU_DSP_BYTESIZE;
	}
# endif /* CO_CPU_DSP */

	/* get data */
	coNumMemcpyUnpack(&trData[1], pSdo->pData, (UNSIGNED32)size,
		pSdo->numeric, packOffset);

	/* transmit message */
	retVal = icoTransmitMessage(pSdo->trCob, &trData[0], 0u);
	
	if (retVal == RET_DRV_TRANS_BUFFER_FULL)  {
		/* transmit next message */
		(void)icoEventStart(&pSdo->blockEvent, sdoClientWriteBlock,
			(void *)pSdo); /*lint !e960 */
		/* Derogation MisraC2004 R.16.9 function identifier used without '&'
		 * or parenthesized parameter */

		return;
	}

	/* apply size information */
	pSdo->restSize += size;
	pSdo->pData += size;

#ifdef CO_EVENT_CSDO_DOMAIN_WRITE
	/* splitted domain transfer ? */
	if (pSdo->split == CO_TRUE)  {
		/* size reached or last segment */
		if (((pSdo->restSize % (7u * pSdo->msgCnt)) == 0u)
			/*|| ((pRecData->data[0] & CO_SDO_CCS_CONT_BIT) != 0u)*/)  {
			/* domain indication */
			if (pSdo->pFunction != NULL)  {
				retVal = pSdo->pFunction(pSdo->sdoNr, pSdo->index,
					pSdo->subIndex, pSdo->restSize, pSdo->pFctData);
				pSdo->pData -= (7u * pSdo->msgCnt);

# ifdef CO_SDO_CLIENT_DOMAIN_SPLIT_INDICATION
				/* split indication ? */
				if (retVal == RET_SDO_SPLIT_INDICATION)  {
					/* stop timeout timer */
					(void)coTimerStop(&pSdo->timer);
					/* save transmit data */
					memcpy(&pSdo->saveData[0], &trData[0], 8);
					pSdo->splitIndication = CO_TRUE;
					return;
				}
				pSdo->splitIndication = CO_FALSE;
# endif /* CO_SDO_CLIENT_DOMAIN_SPLIT_INDICATION */

				if (retVal != RET_OK) {
					icoSdoClientAbort(pSdo, retVal);
					return;
				}
			}
		}
	}
#endif /* CO_EVENT_CSDO_DOMAIN_WRITE */

	icoSdoClientDomainWriteBlockIndCont(pSdo);
}


/******************************************************************************/
/**
* \internal
*
* \brief sdoClientDomainWriteBlockIndCont - continue SDO Client Domain Write indication
*
*
*/
void icoSdoClientDomainWriteBlockIndCont(
		CO_SDO_CLIENT_T	*pSdo
	)
{
	/* block count reached or last block ? */
	if ((pSdo->seqNr >= pSdo->blockSize) || (pSdo->restSize == pSdo->size))  {
		/* await acq from server */
		pSdo->state = CO_SDO_CLIENT_STATE_BLK_DL_ACQ;

		/* start timer */
		(void)coTimerStart(&pSdo->timer, pSdo->timeOutVal,
				icoSdoClientTimeOut, pSdo, CO_TIMER_ATTR_ROUNDUP); /*lint !e960 */
		/* Derogation MisraC2004 R.16.9 function identifier used without '&'
		 * or parenthesized parameter */
	} else {
		pSdo->seqNr++;
		/* transmit next message */
		(void)icoEventStart(&pSdo->blockEvent, sdoClientWriteBlock,
			(void *)pSdo); /*lint !e960 */
		/* Derogation MisraC2004 R.16.9 function identifier used without '&'
		 * or parenthesized parameter */
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief  icoSdoClientWriteBlockAcq
*
*
* \return none
*
*/
void icoSdoClientWriteBlockAcq(
		CO_SDO_CLIENT_T		*pSdo,
		const CO_CAN_REC_MSG_T	*pRecData
	)
{
UNSIGNED8	blk;
UNSIGNED32	size;

	if (pSdo->state != CO_SDO_CLIENT_STATE_BLK_DL_ACQ)  {
		/* ignore message */
		return;
	}

	/* delete timer */
	(void)coTimerStop(&pSdo->timer);

	/* check last seqNr */
	if (pRecData->data[1] == pSdo->seqNr)  {
		pSdo->seqNr = 1u;
	} else {
		/* wrong sequence number ? */
		if (pRecData->data[1] > 127u)  {
			icoSdoClientAbort(pSdo, RET_SDO_WRONG_SEQ_NR);
			return;
		}

		pSdo->seqNr = pRecData->data[1] + 1u;

		/* calculate blocks to be repeated */
		blk = pSdo->blockSize - pRecData->data[1];
		if (pSdo->size == pSdo->restSize)  {
			/* all blocks already sent - calculate last block byte cnt */
			/* size of last block */
			size = (pSdo->size % (pSdo->blockSize * 7ul))
					- (pRecData->data[1] * 7ul);
		} else {
			/* not all blocks sent */
			size = blk * 7ul;
		}
		pSdo->pData -= size;
		pSdo->restSize -= size;
	}

	pSdo->blockSize = pRecData->data[2];
	if ((pSdo->blockSize < 1u) || (pSdo->blockSize > 127u)) {
		icoSdoClientAbort(pSdo, RET_SDO_WRONG_BLOCKSIZE);
		return;
	}

	/* all data transfered ? */
	if ((pSdo->size - pSdo->restSize) == 0u)  {
		/* send block end */
		sdoClientWriteBlockEnd(pSdo);
	} else {
		/* start next block */
		sdoClientWriteBlock(pSdo);
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief  sdoClientWriteBlockEnd
*
*
* \return none
*
*/
static void sdoClientWriteBlockEnd(
		CO_SDO_CLIENT_T	*pSdo
	)
{
UNSIGNED8 size;
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */

	trData[0] = CO_SDO_CCS_BLOCK_DOWNLOAD | CO_SDO_CCS_BLOCK_CS_DL_END;
	memset(&trData[1], 0, 7u);

# ifdef CO_SDO_BLOCK_CRC
	if (pSdo->blockCrcUsed == CO_TRUE)  {
		/* BLOCK_TEST D6 Start */
		/* pSdo->blockCrc = 0; */
		/* BLOCK_TEST D6 End */

		trData[1] = (UNSIGNED8)(pSdo->blockCrc & 0xffu);
		trData[2] = (UNSIGNED8)((pSdo->blockCrc >> 8) & 0xffu);
	}
# endif /* CO_SDO_BLOCK_CRC */

	/* calculate the unused byte in the last segment */
	size = (UNSIGNED8)(pSdo->size % 7u);
	
	if (size != 0u) {
		trData[0] |= (UNSIGNED8)((7u - size) << 2);
	}

	/* transmit answer */
	(void)icoTransmitMessage(pSdo->trCob, &trData[0], 0u);

	pSdo->state = CO_SDO_CLIENT_STATE_BLK_DL_END;

	/* start timer */
	(void)coTimerStart(&pSdo->timer, pSdo->timeOutVal,
			icoSdoClientTimeOut, pSdo, CO_TIMER_ATTR_ROUNDUP); /*lint !e960 */
	/* Derogation MisraC2004 R.16.9 function identifier used without '&'
	 * or parenthesized parameter */
}

# endif /* CO_SDO_BLOCK_CLIENT */
#endif /* CO_SDO_CLIENT_CNT */
