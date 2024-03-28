/*
* co_queue.c - contains functions for queue handling
*
* Copyright (c) 2012-2022 emotas embedded communication GmbH
*-------------------------------------------------------------------
* $Id: co_queue.c 41218 2022-07-12 10:35:19Z boe $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \brief Queue handling
*
* \file co_queue.c
* contains functions for queue handling
*
*/

/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <string.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <gen_define.h>
#include <co_datatype.h>
#include <co_timer.h>
#include <co_drv.h>
#include <co_commtask.h>
#include "ico_common.h"
#include "ico_indication.h"
#include "ico_cobhandler.h"
#include "ico_queue.h"
#ifdef ISOTP_SUPPORTED
# include "iiso_tp.h"
#endif

/* constant definitions
---------------------------------------------------------------------------*/
#define CO_CONFIG_REC_BUFFER_SIZE (CO_CONFIG_REC_BUFFER_CNT * (sizeof(CO_RECBUF_HDR_T) + CO_CAN_MAX_DATA_LEN))

/* local defined data types
---------------------------------------------------------------------------*/
typedef enum {
	CO_TR_STATE_FREE = 0,		/* buffer is free */
	CO_TR_STATE_WAITING,		/* buffer waits for inhibit elapsed */
	CO_TR_STATE_TO_TRANSMIT,	/* buffer should be transmitted */
	CO_TR_STATE_ACTIVE,			/* buffer is transmitting */
	CO_TR_STATE_TRANSMITTED		/* buffer was transmitted */
} CO_TR_STATE_T;

struct CO_TRANS_QUEUE {
	CO_CAN_TR_MSG_T	msg;		/* can message */
	COB_REFERENZ_T	cobRef;		/* cob reference */
	CO_TR_STATE_T	state;		/* status transmit message (written from ISR */
	struct CO_TRANS_QUEUE *pNext;	/* index to next buffer */
};
typedef struct CO_TRANS_QUEUE CO_TRANS_QUEUE_T;


typedef struct {
	UNSIGNED32			canId;		/* CAN identifier */
#ifdef CO_CAN_TIMESTAMP_SUPPORTED
	CO_CAN_TIMESTAMP_T	timestamp;	/* timestamp */
#endif /* CO_CAN_TIMESTAMP_SUPPORTED */
	UNSIGNED8			dataLen;	/* CAN msg len */
	UNSIGNED8			flags;		/* flags (rtr, extended, enabled, ... */
} CO_RECBUF_HDR_T;

typedef struct {
	CO_RECBUF_HDR_T	bufHdr;
	UNSIGNED16 		hdrStart;		/* offset start header */
	UNSIGNED16 		lastData;		/* offset last data byte */
	BOOL_T			swap;
	UNSIGNED8		tmpDataBuf[CO_CAN_MAX_DATA_LEN];
} CO_RECBUF_DATA_T;


#ifdef CO_SYNC_SUPPORTED
typedef enum {
	CO_SYNCBUF_STATE_EMPTY,
	CO_SYNCBUF_STATE_SYNC_REQ,
	CO_SYNCBUF_STATE_SYNC_AVAIL,
	CO_SYNCBUF_STATE_SYNC_CNT_REQ,
	CO_SYNCBUF_STATE_SYNC_CNT_AVAIL
} CO_SYNCBUF_STATE_T;

typedef struct {
	UNSIGNED32	canId;
	UNSIGNED8	syncCounter;
	CO_SYNCBUF_STATE_T	state;
	UNSIGNED8	flags;
} CO_SYNC_BUFFER_T;
#endif /* CO_SYNC_SUPPORTED */

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
static CO_TRANS_QUEUE_T	*searchLastMessage(UNSIGNED16 cobRef, BOOL_T all);
static CO_TRANS_QUEUE_T	*getNextTransBuf(void);
static void inhibitTimer(void *pData);
static void addToInhibitList(CO_TRANS_QUEUE_T *pTrBuf);
static void addToTransmitList(CO_TRANS_QUEUE_T *pTrBuf);
#ifdef CO_SYNC_SUPPORTED
static void addToTransmitListFirst(CO_TRANS_QUEUE_T *pTrBuf);
#endif /* CO_SYNC_SUPPORTED */
static CO_TRANS_QUEUE_T *moveInhibitToTransmitList(CO_CONST CO_COB_T *pCob);

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/
#ifdef CO_EVAL_MODE
UNSIGNED32 demoTimerEnable = 0u;
#endif /* CO_EVAL_MODE */

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef OLD_FIXED_BUFFER
static CO_REC_DATA_T	recDataBuffer[CO_CONFIG_REC_BUFFER_CNT];
#else /* OLD_FIXED_BUFFER */
static UNSIGNED8		recBuffer[CO_CONFIG_REC_BUFFER_SIZE];
static UNSIGNED8		*pRecBuffer;
static UNSIGNED16		recBufferSize;
static CO_RECBUF_DATA_T	recBufData;
#endif /* OLD_FIXED_BUFFER */
static UNSIGNED16		recBufferWrCnt = { 0u };
static UNSIGNED16		recBufferRdCnt = { 0u };
#ifdef OLD_FIXED_BUFFER
static CO_TRANS_QUEUE_T	trDataBuffer[CO_CONFIG_TRANS_BUFFER_CNT];
#else /* OLD_FIXED_BUFFER */
static CO_TRANS_QUEUE_T	trBuffer[CO_CONFIG_TRANS_BUFFER_CNT];
#endif /* OLD_FIXED_BUFFER */
static BOOL_T			drvBufAccess = { CO_TRUE };
static CO_TRANS_QUEUE_T	*pInhibitList = { NULL };
static CO_TRANS_QUEUE_T	*pTransmitList = { NULL };
static BOOL_T			recBufFull = { CO_FALSE };
#ifdef CO_EVENT_SLEEP
static BOOL_T			queueDisabled = { CO_FALSE };
#endif /* CO_EVENT_SLEEP */
#ifdef CO_GATEWAY_BUFFER
static CO_REC_DATA_T	gwRecDataBuffer[CO_CONFIG_REC_BUFFER_CNT];
static UNSIGNED16		gwRecBufferWrCnt = 0u;
static UNSIGNED16		gwRecBufferRdCnt = 0u;
#endif /* CO_GATEWAY_BUFFER */
#ifdef CO_SYNC_SUPPORTED
static CO_SYNC_BUFFER_T syncBuf;
#endif /* CO_SYNC_SUPPORTED */

#ifdef CO_EVAL_MODE
#warning "Compiling for EVAL mode"
static CO_TIMER_T	demoTimer;
static UNSIGNED32	demoTrCnt = 0;
#define CO_DEMO_TIMER_VALUE	320100	/* in usec */
static void demoTimerFct(void * pData);
#endif /* CO_EVAL_MODE */



/***************************************************************************/
/**
* \internal
*
* \brief icoGetReceiveMessage - get next receives message
*
*
* \retval CO_FALSE
*	no data available
* \retval CO_FALSE
*	data available
*/
BOOL_T icoQueueGetReceiveMessage(
		CO_REC_DATA_T	*pRecData		/* pointer to receive data */
	)
{
#ifdef OLD_FIXED_BUFFER
CO_REC_DATA_T	*pData = NULL;
#else /* OLD_FIXED_BUFFER */
CO_RECBUF_HDR_T	bufHdr;
UNSIGNED16		bufOffs;
#endif /* OLD_FIXED_BUFFER */
CO_COB_T		*pCob;
BOOL_T			cobFound = CO_FALSE;
BOOL_T			msgFound = CO_FALSE;
#ifdef CO_GATEWAY_BUFFER
UNSIGNED8		getGW = 1;
#endif /* CO_GATEWAY_BUFFER */

#ifdef CO_SYNC_SUPPORTED
	/* as first, check if SYNC message was received */
	if ((syncBuf.state == CO_SYNCBUF_STATE_SYNC_AVAIL)
	 || (syncBuf.state == CO_SYNCBUF_STATE_SYNC_CNT_AVAIL))  {
		pRecData->service = CO_SERVICE_SYNC_RECEIVE;
		pRecData->spec = 0u;
		pRecData->msg.canId = syncBuf.canId;

		if (syncBuf.state == CO_SYNCBUF_STATE_SYNC_CNT_AVAIL)  {
			pRecData->msg.len = 1u;
			pRecData->msg.data[0] = syncBuf.syncCounter;
		} else {
			pRecData->msg.len = 0u;
		}

		syncBuf.state = CO_SYNCBUF_STATE_EMPTY;

# ifdef CO_EVENT_SLEEP
		if (queueDisabled == CO_TRUE)  {
			return(CO_FALSE);
		}
# endif /* CO_EVENT_SLEEP */

# ifdef CUSTOMER_RECEIVE_MESSAGES_CALLBACK
		/* sometime the customer want to see all received messages,
		 * e.g. for USB Gateway.
		 *
		 * The customer require a function
		 * 
		 * BOOL_T fct( const CO_CAN_REC_MSG_T	*pRecData)
		 * Set CUSTOMER_RECEIVE_MESSAGES_CALLBACK to fct.
		 * (include co_drv.h for CO_CAN_REC_MSG_T)
		 */
		if (CUSTOMER_RECEIVE_MESSAGES_CALLBACK(&pRecData->msg)
				== CO_FALSE)  {
			return(CO_FALSE);
		}
# endif /* CUSTOMER_RECEIVE_MESSAGES_CALLBACK */

# ifdef CO_GATEWAY_BUFFER
				/* write data to gateway */
		coGatewayTransmitMessage(&pRecData->msg);
# endif /* CO_GATEWAY_BUFFER */

		return(CO_TRUE);
	}
#endif /* CO_SYNC_SUPPORTED */


	/* check queue for message and get it */
#ifdef CO_GATEWAY_BUFFER
	while ((recBufferWrCnt != recBufferRdCnt)
	    || (gwRecBufferWrCnt != gwRecBufferRdCnt)) {

		/* get message alternate from gw */
		if (getGW == 0u)  {
			getGW++;
		} else {
			getGW = 0u;
		}
		if (getGW != 0u)  {
			/* data from gw buffer */
			if (gwRecBufferWrCnt != gwRecBufferRdCnt)  {
				msgFound = CO_TRUE;

				gwRecBufferRdCnt++;
				if (gwRecBufferRdCnt >= CO_CONFIG_REC_BUFFER_CNT)  {
					gwRecBufferRdCnt = 0u;
				}
				pData = &gwRecDataBuffer[gwRecBufferRdCnt];
			}
		} else
#else /* CO_GATEWAY_BUFFER */
	while (recBufferWrCnt != recBufferRdCnt)  {
#endif /* CO_GATEWAY_BUFFER */
		{

			/* get data from CAN buffer */
			if (recBufferWrCnt != recBufferRdCnt)  {
				msgFound = CO_TRUE;

#ifdef OLD_FIXED_BUFFER
				recBufferRdCnt++;
				if (recBufferRdCnt >= CO_CONFIG_REC_BUFFER_CNT)  {
					recBufferRdCnt = 0u;
				}
				pData = &recDataBuffer[recBufferRdCnt];


#else /* OLD_FIXED_BUFFER */


				/* get header infos */
				bufOffs = recBufferRdCnt;
				/* if header doesn't fit completely at end of buffer,
				 * goto start of buffer*/
				if ((bufOffs + sizeof(CO_RECBUF_HDR_T)) > recBufferSize)  {
				/* printf("icoQueueGetReceiveMessage1: RdCnt: %d set to 0\n",
 					bufOffs);
				*/
					bufOffs = 0u;
					recBufferRdCnt = 0u;
				}
				memcpy(&bufHdr, &pRecBuffer[bufOffs],
					sizeof(CO_RECBUF_HDR_T));

				/* printf("icoQueueGetReceiveMessage: RdCnt: %d, datalen: %d, id: %x\n", bufOffs, bufHdr.dataLen, bufHdr.canId);
				*/
				/* and save it at given buffer */
				pRecData->msg.canId = bufHdr.canId;
				pRecData->msg.len = bufHdr.dataLen;
				pRecData->msg.flags = bufHdr.flags;
#ifdef CO_CAN_TIMESTAMP_SUPPORTED
				pRecData->msg.timestamp = bufHdr.timestamp;
#endif /* CO_CAN_TIMESTAMP_SUPPORTED */

				/* data swapped ? */
				bufOffs += (UNSIGNED16)sizeof(CO_RECBUF_HDR_T);
				if ((bufOffs + bufHdr.dataLen) > recBufferSize)  {
					/* swapped */
					UNSIGNED16	len1 = recBufferSize - bufOffs;
					UNSIGNED16	len2 = bufHdr.dataLen - len1;

					memcpy(&pRecData->msg.data[0], &pRecBuffer[bufOffs],
						(size_t)len1);
					memcpy(&pRecData->msg.data[len1], &pRecBuffer[0u],
						(size_t)len2);
				} else {
					memcpy(&pRecData->msg.data[0], &pRecBuffer[bufOffs],
						(size_t)bufHdr.dataLen);
				}

				recBufferRdCnt += (UNSIGNED16)(sizeof(CO_RECBUF_HDR_T) + bufHdr.dataLen);
				if (recBufferRdCnt >= recBufferSize)  {
					recBufferRdCnt -= recBufferSize;
				}

				/* printf("icoQueueGetReceiveMessageEnd: RdCnt: %d\n",
					recBufferRdCnt);
				*/
#endif /* OLD_FIXED_BUFFER */

				/* if buffer is empty, inform application */
				if (recBufferRdCnt == recBufferWrCnt)  {
					coCommStateEvent(CO_COMM_STATE_EVENT_REC_QUEUE_EMPTY);
				}

#ifdef CO_EVENT_SLEEP
				if (queueDisabled == CO_TRUE)  {
					return(CO_FALSE);
				}
#endif /* CO_EVENT_SLEEP */

#ifdef CUSTOMER_RECEIVE_MESSAGES_CALLBACK
				/* sometime the customer want to see all received messages,
				 * e.g. for USB Gateway.
				 *
				 * The customer require a function
				 * 
				 * void fct( const CO_CAN_REC_MSG_T	*pRecData)
				 * Set CUSTOMER_RECEIVE_MESSAGES_CALLBACK to fct.
				 * (include co_drv.h for CO_CAN_REC_MSG_T)
				 */
#ifdef OLD_FIXED_BUFFER
				if (CUSTOMER_RECEIVE_MESSAGES_CALLBACK(&pData->msg)
						== CO_FALSE)  {
					return(CO_FALSE);
				}
#else /* OLD_FIXED_BUFFER */
				if (CUSTOMER_RECEIVE_MESSAGES_CALLBACK(&pRecData->msg)
						== CO_FALSE)  {
					return(CO_FALSE);
				}
#endif /* OLD_FIXED_BUFFER */
#endif /* CUSTOMER_RECEIVE_MESSAGES_CALLBACK */

#ifdef CO_GATEWAY_BUFFER
				/* write data to gateway */
#ifdef OLD_FIXED_BUFFER
				coGatewayTransmitMessage(&pData->msg);
#else /* OLD_FIXED_BUFFER */
				coGatewayTransmitMessage(&pRecData->msg);
#endif /* OLD_FIXED_BUFFER */
#endif /* CO_GATEWAY_BUFFER */
			}
		}

#ifdef OLD_FIXED_BUFFER
		/* msg available (CAN or GW) */
		if ((msgFound == CO_TRUE) && (pData != NULL))  {

			cobFound = CO_FALSE;
			pCob = icoCobCheck(&pData->msg);

			if (pCob == NULL)  {
#ifdef CANOPEN_SUPPORTED
				/* check for bootup message */ 
				if (((pData->msg.canId & 0xFFFFFF80ul) == 0x700ul)
				 && ((pData->msg.flags & CO_COBFLAG_EXTENDED) == 0u))  {
					/* ok, bootup found */
					pData->service = CO_SERVICE_NMT;
					pData->spec = 0xffffu;
					cobFound = CO_TRUE;
				}
#endif /* CANOPEN_SUPPORTED */
			} else {
				pData->service = pCob->service;
				pData->spec = pCob->serviceNr;
				cobFound = CO_TRUE;
			}
		}

		if ((cobFound == CO_TRUE) && (pData != NULL))  {
			memcpy((void *)pRecData, (void *)pData, sizeof(CO_REC_DATA_T));
			return(CO_TRUE);
		}
#else /* OLD_FIXED_BUFFER */
		/* msg available (CAN or GW) */
		if (msgFound == CO_TRUE)  {

			cobFound = CO_FALSE;
			pCob = icoCobCheck(&pRecData->msg);
			if (pCob != NULL)  {
				pRecData->service = pCob->service;
				pRecData->spec = pCob->serviceNr;
				cobFound = CO_TRUE;
			}
#ifdef CANOPEN_SUPPORTED
			else {
				/* check for bootup message */ 
				if (((pRecData->msg.canId & 0xFFFFFF80ul) == 0x700ul)
				 && ((pRecData->msg.flags & CO_COBFLAG_EXTENDED) == 0u))  {
					/* ok, bootup found */
					pRecData->service = CO_SERVICE_NMT;
					pRecData->spec = 0xffffu;
					cobFound = CO_TRUE;
				}
			}
#endif /* CANOPEN_SUPPORTED */
		}

		if (cobFound == CO_TRUE)  {
			return(CO_TRUE);
		}
#endif /* OLD_FIXED_BUFFER */

	}

	return(CO_FALSE);
}


/***************************************************************************/
/**
*
* \brief coQueueReceiveMessageAvailable - receive messages available
*
* This functions checks the receive queue for new messages.
* Are new messages available, return CO_TRUE.
* Otherwise CO_FALSE
*
* \retval CO_FALSE
*	no data available
* \retval CO_TRUE
*	data available
*/
BOOL_T coQueueReceiveMessageAvailable(
		void	/* no parameter */
	)
{
#ifdef CO_EVENT_SLEEP
	if (queueDisabled == CO_TRUE)  {
		return(CO_FALSE);
	}
#endif /* CO_EVENT_SLEEP */

	/* check queue for message */
	if ((recBufferWrCnt != recBufferRdCnt)
#ifdef CO_SYNC_SUPPORTED
	 || (syncBuf.state == CO_SYNCBUF_STATE_SYNC_AVAIL)
	 || (syncBuf.state == CO_SYNCBUF_STATE_SYNC_CNT_AVAIL)
#endif /* CO_SYNC_SUPPORTED */
#ifdef CO_GATEWAY_BUFFER
	 || (gwRecBufferWrCnt != gwRecBufferRdCnt)
#endif /* CO_GATEWAY_BUFFER */
		) {
		return(CO_TRUE);
	} else {
		return(CO_FALSE);
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief coQueueGetReceiveBuffer - get pointer to receive buffer
*
* Check if buffer is available,
* save id and flags
* and return pointer to data buffer.
*
* If no buffer is necessary (0 databytes or RTR)
* return NULL and set buffer as filled.
*
* can be called at interrupt level
*
* \return pointer to save receive data
*
*/
UNSIGNED8 *coQueueGetReceiveBuffer(
		UNSIGNED32		canId,		/**< can ID */
		UNSIGNED8		dataLen,	/**< data len */
		UNSIGNED8		flags		/**< flags */
#ifdef CO_CAN_TIMESTAMP_SUPPORTED
		, CO_CAN_TIMESTAMP_T	timestamp
#endif /* CO_CAN_TIMESTAMP_SUPPORTED */
	)
{
	/* printf("coQueueGetReceiveBuffer: rdCnt: %d, wrCnt: %d\n",
		recBufferRdCnt, recBufferWrCnt);
	*/
#ifdef OLD_FIXED_BUFFER
UNSIGNED16	tmpCnt;

	/* get next receive buffer */
	tmpCnt = recBufferWrCnt + 1u;
	if (tmpCnt >= CO_CONFIG_REC_BUFFER_CNT)  {
		tmpCnt = 0u;
	}
	if (tmpCnt == recBufferRdCnt)  {
		/* save event, and signal it later outsite of the interrupt */
		recBufFull = CO_TRUE;

		return(NULL);
	}

	recDataBuffer[tmpCnt].msg.canId = canId;
	recDataBuffer[tmpCnt].msg.len = dataLen;
	recDataBuffer[tmpCnt].msg.flags = flags;
	return(&recDataBuffer[tmpCnt].msg.data[0]);

#else /* OLD_FIXED_BUFFER */

CO_RECBUF_DATA_T	*pBuf = &recBufData;
UNSIGNED16	bufLen;
UNSIGNED16	tmpWrCnt;

#ifdef CO_EVENT_SLEEP
	if (queueDisabled == CO_TRUE)  {
		return(NULL);
	}
#endif /* CO_EVENT_SLEEP */

#ifdef CO_SYNC_SUPPORTED
	/* SYNC message ? */
	if ((canId == syncBuf.canId)
	 && ((flags & (CO_COBFLAG_EXTENDED | CO_COBFLAG_RTR)) == syncBuf.flags))  {
		if (dataLen == 0u)  {
			syncBuf.state = CO_SYNCBUF_STATE_SYNC_REQ;
			coQueueReceiveBufferIsFilled();

			return(NULL);
		} else {
			if (dataLen == 1u)  {
				syncBuf.state = CO_SYNCBUF_STATE_SYNC_CNT_REQ;
				return(&syncBuf.syncCounter);
			}
		}
	}
#endif /* CO_SYNC_SUPPORTED */

	/* set swap to default value */
	pBuf->swap = CO_FALSE;

	/* set len for RTR frames to 0 */
	if ((flags & CO_COBFLAG_RTR) != 0u)  {
		dataLen = 0u;
	}

	/* save data at buffer */
	pBuf->bufHdr.canId = canId;
#ifdef CO_CAN_TIMESTAMP_SUPPORTED
	pBuf->bufHdr.timestamp = timestamp;
#endif /* CO_CAN_TIMESTAMP_SUPPORTED */
	pBuf->bufHdr.dataLen = dataLen;
	pBuf->bufHdr.flags = flags;
	bufLen = (UNSIGNED16)(sizeof(CO_RECBUF_HDR_T) + dataLen);

	/* calculate header start address */
	pBuf->hdrStart = recBufferWrCnt;
	/* if header doesn't fit completely at end of buffer, goto start of buffer*/
	if ((pBuf->hdrStart + sizeof(CO_RECBUF_HDR_T)) > recBufferSize)  {
		pBuf->hdrStart = 0u;
		/* calculate end of data */
		pBuf->lastData = bufLen - 1u;

		/* check, that hdr + data < rdCnt */
		if (pBuf->lastData >= recBufferRdCnt)  {
			/* no space at buffer */
			/* save event, and signal it later outsite of the interrupt */
			recBufFull = CO_TRUE;

			return(NULL);
		}
	} else {
		/* calculate end of data */
		pBuf->lastData = (pBuf->hdrStart + bufLen) - 1u;

		/* fit whole message before end of recBuffer ? */
		if (pBuf->lastData >= recBufferSize)  {
			/* data swapping */
			pBuf->lastData -= recBufferSize;

			if (recBufferWrCnt >= recBufferRdCnt)  {
				if (pBuf->lastData >= recBufferRdCnt)  {
					/* no space at buffer */
					/* save event, and signal it later outsite of interrupt */
					recBufFull = CO_TRUE;

					return(NULL);
				}
			} else {
				/* no space at buffer */
				/* save event, and signal it later outsite of interrupt */
				recBufFull = CO_TRUE;

				return(NULL);
			}
		} else {
			if (recBufferWrCnt < recBufferRdCnt)  {
				if (pBuf->lastData >= recBufferRdCnt)  {
					/* no space at buffer */
					/* save event, and signal it later outsite of interrupt */
					recBufFull = CO_TRUE;

					return(NULL);
				}
			}
		}
	}
	tmpWrCnt = pBuf->lastData + 1u;
	if (tmpWrCnt >= recBufferSize)  {
		tmpWrCnt = 0u;
	}
	if (tmpWrCnt == recBufferRdCnt)  {
		/* no space at buffer */
		/* save event, and signal it later outsite of interrupt */
		recBufFull = CO_TRUE;
		/* same state as empty, so leave it empty */
		return(NULL);
	}

	/* save header data */
	memcpy(&pRecBuffer[pBuf->hdrStart], &pBuf->bufHdr,
		sizeof(CO_RECBUF_HDR_T));
	if (dataLen == 0u)  {
		/* no data to copy */
		coQueueReceiveBufferIsFilled();

		return(NULL);
	}

	/* fit data after hdr? */
	if ((pBuf->hdrStart + bufLen) > recBufferSize)  {
		/* printf("coQueueGetReceiveBufferEnd: rdCnt: %d, wrCnt: %d, swap, bufLen: %d\n", recBufferRdCnt, recBufferWrCnt, bufLen);
		*/
		pBuf->swap = CO_TRUE;
		/* data swapping necessary */
		return(&pBuf->tmpDataBuf[0]);
	} else {
		pBuf->swap = CO_FALSE;
		/* printf("coQueueGetReceiveBufferEnd: rdCnt: %d, wrCnt: %d, data: %d\n", recBufferRdCnt, recBufferWrCnt, pBuf->hdrStart + sizeof(CO_RECBUF_HDR_T));
		*/
		return(&pRecBuffer[pBuf->hdrStart + sizeof(CO_RECBUF_HDR_T)]);
	}
#endif /* OLD_FIXED_BUFFER */
}


/***************************************************************************/
/**
*
* \internal
*
* \brief coQueueReceiveBufferIsFilled - given queue buffer is filled
*
* This function is called from the driver,
* if the given receive buffer was filled by a received CAN message.<br>
*
* It can be called at interrupt level.
*
*/
void coQueueReceiveBufferIsFilled(
		void	/* no parameter */
	)
{
#ifdef OLD_FIXED_BUFFER
UNSIGNED16	tmpCnt;
#else /* OLD_FIXED_BUFFER */
CO_RECBUF_DATA_T	*pBuf = &recBufData;
UNSIGNED16	len1;
#endif /* OLD_FIXED_BUFFER */

#ifdef CO_EVENT_SLEEP
	if (queueDisabled == CO_TRUE)  {
		return;
	}
#endif /* CO_EVENT_SLEEP */

#ifdef OLD_FIXED_BUFFER
	tmpCnt = recBufferWrCnt + 1u;
	if (tmpCnt >= CO_CONFIG_REC_BUFFER_CNT)  {
		tmpCnt = 0u;
	}
	if (tmpCnt == recBufferRdCnt)  {
		coCommStateEvent(CO_COMM_STATE_EVENT_REC_QUEUE_OVERFLOW);
		return;
	}

	recBufferWrCnt = tmpCnt;

#else /* OLD_FIXED_BUFFER */
# ifdef CO_SYNC_SUPPORTED
	if (syncBuf.state == CO_SYNCBUF_STATE_SYNC_REQ)  {
		syncBuf.state = CO_SYNCBUF_STATE_SYNC_AVAIL;
		coCommTaskSet(CO_COMMTASK_EVENT_MSG_AVAIL);
		return;
	}
	if (syncBuf.state == CO_SYNCBUF_STATE_SYNC_CNT_REQ)  {
		syncBuf.state = CO_SYNCBUF_STATE_SYNC_CNT_AVAIL;
		coCommTaskSet(CO_COMMTASK_EVENT_MSG_AVAIL);
		return;
	}
# endif /* CO_SYNC_SUPPORTED */

	/* data swap ? */
	if (pBuf->swap == CO_TRUE)  {
		len1 = (recBufferSize - pBuf->hdrStart) - (UNSIGNED16)sizeof(CO_RECBUF_HDR_T);
		/* printf("coQueueReceiveBufferIsFilledSwap: rdCnt: %d, wrCnt: %d, hdr: %d, len1: %d, lastData: %d\n", recBufferRdCnt, recBufferWrCnt, pBuf->hdrStart, len1, pBuf->lastData);
		*/
		memcpy(&pRecBuffer[pBuf->hdrStart + sizeof(CO_RECBUF_HDR_T)],
			&pBuf->tmpDataBuf[0], (size_t)len1);
		memcpy(&pRecBuffer[0], &pBuf->tmpDataBuf[len1],
			(size_t)pBuf->lastData + 1u);
	}

	recBufferWrCnt = pBuf->lastData + 1u;
	if (recBufferWrCnt >= recBufferSize)  {
		recBufferWrCnt = 0u;
	}
	/* printf("coQueueReceiveBufferIsFilled: rdCnt: %d, wrCnt: %d, hdr: %d, data: %d, swapped: %d\n", recBufferRdCnt, recBufferWrCnt, pBuf->hdrStart, pBuf->lastData, pBuf->swap);
	*/
#endif /* OLD_FIXED_BUFFER */

	coCommTaskSet(CO_COMMTASK_EVENT_MSG_AVAIL);

	/* last message buffer used ? */
/* 	coCommStateEvent(CO_COMM_STATE_EVENT_REC_QUEUE_FULL); */
}


/***************************************************************************/
/**
* \internal
*
* \brief icoTransmitMessage - save message in transmit queue
*
* flags:
* MSG_OVERWRITE - if the last message is not transmitted yet,
*	overwrite the last data with the new data
* MSG_RET_INHIBIT - check inhibit time
*	if inhibit time is active, return RET_INHIBIT_ACTIVE
*	otherwise save it at the inhibit queue
*	and send it after the inhibit time has been ellapsed
*
* \return RET_T
*
*/
RET_T icoTransmitMessage(
		COB_REFERENZ_T	cobRef,			/* cob reference */
		const UNSIGNED8	*pData,			/* pointer to transmit data */
		UNSIGNED8		flags			/* data handle flags */
	)
{
CO_COB_T	*pCob;
CO_TRANS_QUEUE_T *pTrBuf = NULL;
CO_TRANS_QUEUE_T *pLastBuf = NULL;
UNSIGNED16	i;
BOOL_T		inhibitActive;
BOOL_T		msgDoubled = CO_FALSE;
BOOL_T		msgOverwrite = CO_FALSE;

#ifdef CO_EVENT_SLEEP
	if (queueDisabled == CO_TRUE)  {
		return(RET_OK);
	}
#endif /* CO_EVENT_SLEEP */

	pCob = icoCobGetPointer(cobRef);
	if (pCob == NULL)  {
		return(RET_EVENT_NO_RESSOURCE);
	}

	/* if cob is disabled, return */
	if ((pCob->canCob.flags & CO_COBFLAG_ENABLED) == 0u)  {
		return(RET_COB_DISABLED);
	}

	/* if inhibit active ? */
	inhibitActive = coTimerIsActive(&pCob->inhibitTimer);
	/* inhibit is active, return RET_INHIBIT_ACTIVE */
	if ((flags & MSG_RET_INHIBIT) != 0u)  {
		/* inhibit timer active ? */
		if (inhibitActive == CO_TRUE)  {
			return(RET_INHIBIT_ACTIVE);
		}
		/* inhibit is set ?*/
		if (pCob->inhibit > 0u)  {
			/* in transmit buffer ? */
			pTrBuf = searchLastMessage(cobRef, CO_TRUE);
			if (pTrBuf != NULL)  {
				return(RET_INHIBIT_ACTIVE);
			}
		}
	}

#ifdef CUSTOMER_TRANSMIT_MESSAGES_CALLBACK
	/* sometime the customer want to modify transmit messages,
	 * like add checksum...
	 *
	 * The customer require a function
	 * 
	 * RET_T fct(canLine, id, dlc, *data)
	 */
	{
	RET_T retVal;
	UNSIGNED8 buffer[CO_CAN_MAX_DATA_LEN];

	if (pData != NULL)  {
		/* copy data to buffer */
		for (i = 0u; i < pCob->len; i++)  {
			buffer[i] = pData[i];
		}
	}
	
	retVal = CUSTOMER_TRANSMIT_MESSAGES_CALLBACK(pCob->canCob.canId,
		pCob->len, &buffer[0]);
	if (retVal != RET_OK)  {
		return(retVal);
	}
	pData = &buffer[0];
	}
#endif /* CUSTOMER_TRANSMIT_MESSAGES_CALLBACK */

#ifdef J1939_TESTCODE
	applInfoSend(pCob->canCob.canId, pData, pCob->len);
#endif /* J1939_TESTCODE */

	/* disable driver buffer access */
	drvBufAccess = CO_FALSE;

	/* should data at queue always overwritten ? */
	if ((flags & MSG_OVERWRITE) != 0u)  {
		/* search last message */
		pTrBuf = searchLastMessage(cobRef, CO_FALSE);
	} else {
		/* overwrite data only, if there has been changed
		 * Only valid for for PDO, EMCY and HB */
		if ((pCob->service == CO_SERVICE_ERRCTRL)
		 || (pCob->service == CO_SERVICE_GUARDING)
		 || (pCob->service == CO_SERVICE_EMCY_TRANSMIT)
		 || (pCob->service == CO_SERVICE_PDO_TRANSMIT))  {
			/* search last message */
			pTrBuf = searchLastMessage(cobRef, CO_FALSE);
			if (pTrBuf != NULL)  {
				/* message is in buffer, check for changed data */
				for (i = 0u; i < pTrBuf->msg.len; i++)  {
					if (pTrBuf->msg.data[i] != pData[i])  {
						/* data are changed... */
						pTrBuf = NULL;
						break;
					}
				}
				if (pTrBuf != NULL)  {
					msgDoubled = CO_TRUE;
				}
			}
		}
	}

	/* old buffer available ? */
	if (pTrBuf != NULL)  {
		msgOverwrite = CO_TRUE;
	} else {

		pTrBuf = getNextTransBuf();
		/* printf("No Overwrite, "); */
		if (pTrBuf == NULL)  {
			/* allow driver buffer access */
			drvBufAccess = CO_TRUE;

			/* start can transmission again */
			(void) codrvCanStartTransmission();

			/* inform application */
			coCommStateEvent(CO_COMM_STATE_EVENT_TR_QUEUE_OVERFLOW);
			return(RET_DRV_TRANS_BUFFER_FULL);
		}
	}

	/* if not the same message */
	if (msgDoubled == CO_FALSE)  {

		/* save data at transmit buffer */
		pTrBuf->cobRef = cobRef;
		pTrBuf->msg.flags = CO_COBFLAG_NONE;
		pTrBuf->msg.len = pCob->len;
		pTrBuf->msg.canId = pCob->canCob.canId;
		if ((pCob->canCob.flags & CO_COBFLAG_EXTENDED) != 0u)  {
			pTrBuf->msg.flags |= CO_COBFLAG_EXTENDED;
		}

		if (pCob->type == CO_COB_TYPE_RECEIVE)  {
			pTrBuf->msg.flags |= CO_COBFLAG_RTR;
		}
		pTrBuf->msg.canChan = pCob->canCob.canChan;

		if (pData != NULL)  {
			for (i = 0u; i < pCob->len; i++)  {
				pTrBuf->msg.data[i] = pData[i];
			}
		}

		/* should this message create a transmit event */
		if ((flags & MSG_INDICATION) != 0u)  {
			pTrBuf->msg.flags |= CO_COBFLAG_IND;
		}

		/* set state only for new messages */
		if (msgOverwrite == CO_FALSE)  {
			/* inhibit active */ 
			if (inhibitActive == CO_TRUE)  {
				/* the same message is already at buffer */
				/* printf("inhibit active\n"); */

				/* add to inhibit list */
				addToInhibitList(pTrBuf);

			} else {
				/* searchLastMessage incl tranmitted messages */
				pLastBuf = searchLastMessage(cobRef, CO_TRUE);
				/* if the same message already on the buffer
				 * and inhibit time != 0*/
				if ((pLastBuf != NULL) && (pCob->inhibit != 0u))  {
					/* yes */

					/* add to inhibit list */
					addToInhibitList(pTrBuf);
				} else {

					/* add to transmit list */
#ifdef CO_SYNC_SUPPORTED
					/* sync message ? */
					if (pCob->service == CO_SERVICE_SYNC_TRANSMIT)  {
						addToTransmitListFirst(pTrBuf);
					} else
#endif /* CO_SYNC_SUPPORTED */
					{
						addToTransmitList(pTrBuf);
					}

#ifdef CO_GATEWAY_BUFFER
					/* send it by gateway */
					coGatewayTransmitMessage(&pTrBuf->msg);
#endif /* CO_GATEWAY_BUFFER */
				}
			}
		}
	}

	/* allow driver buffer access */
	drvBufAccess = CO_TRUE;

	/* start can transmitting */
	(void) codrvCanStartTransmission();

#ifdef CO_EVAL_MODE
	if (demoTimerEnable != 0u)  {
		/* demo timer started ? */
		if (coTimerIsActive(&demoTimer) == CO_TRUE)  {
			/* check, if timer functionality isn't called */
			demoTrCnt++;
			/* more than 10 per msec ? */
			if (demoTrCnt > (CO_DEMO_TIMER_VALUE / 100))  {
				/* then the timer will not work... */
				demoTimerFct((void *)0x12735);
			}
		} else {
			/* Start timer */
			coTimerStart(&demoTimer, CO_DEMO_TIMER_VALUE, demoTimerFct,
				NULL, CO_TIMER_ATTR_ROUNDUP_CYCLIC); /*lint !e960 */
			/* Derogation MisraC2004 R.16.9 function identifier used without '&'
			 * or parenthesized parameter */
		}
	}
#endif /* CO_EVAL_MODE */

	return(RET_OK);
}


BOOL_T icoQueueInhibitActive(
		COB_REFERENZ_T	cobRef				/* cob reference */
	)
{
CO_COB_T	*pCob;
CO_TRANS_QUEUE_T *pLastBuf;
BOOL_T	inhibitActive;

	pCob = icoCobGetPointer(cobRef);
	if (pCob == NULL)  {
		return(CO_FALSE);
	}
	inhibitActive = coTimerIsActive(&pCob->inhibitTimer);
	pLastBuf = searchLastMessage(cobRef, CO_TRUE);

	if ((inhibitActive == CO_TRUE)
	 || ((pLastBuf != NULL) && (pCob->inhibit != 0u))) {
		return(CO_TRUE);
	} else {
		return(CO_FALSE);
	}
}


#ifdef CO_EVAL_MODE
/* fct is called every CO_DEMO_TIMER_VALUE msec
* wait about 1 h and then stop the program
*/
#if !defined(CO_EVAL_TIMEOUT)
/* ca 1 h       1.037 * 60 * 60 * 1000000ul */
# define CO_EVAL_TIMEOUT	(1.037 * 60 * 60 *1000000ul)
#endif /* !defined(CO_EVAL_TIMEOUT) */

static void demoTimerFct(
		void * pData
	)
{
static UNSIGNED32 demoFctCnt = 0;

	if (pData == NULL)  {
		demoTrCnt = 0;
	}

	demoFctCnt ++;

	if (demoFctCnt > (CO_EVAL_TIMEOUT / CO_DEMO_TIMER_VALUE))  {
/*		printf("**** Zeit ist um\n"); */
		pInhibitList = (void *)0x17835;
		pTransmitList = (void *)0x84653;
	}
}
#endif /* CO_EVAL_MODE */


/***************************************************************************/
/**
* \internal
*
* \brief addToInhibitList - add buffer to inhibit list
*
* Add an buffer entry to the inhibit list as the first element
*
*
*/
static void addToInhibitList(
		CO_TRANS_QUEUE_T	*pTrBuf
	)
{
CO_TRANS_QUEUE_T	*pList;

	/*
	printf("addToInhibitList: buf %ld - ", BUF_NUM(pTrBuf));
	pList = pInhibitList;
	while (pList != NULL)  {
		printf("%ld ", BUF_NUM(pList));
		pList = pList->pNext;
	}
	printf("\n");
	*/

	/* add it as first entry */
	/* as first save actual pointer */
	pList = pInhibitList;
	pInhibitList = pTrBuf;
	pTrBuf->pNext = pList;
	/* save buffer state */
	pTrBuf->state = CO_TR_STATE_WAITING;
	/* printf(" WAITING\n"); */
}


/***************************************************************************/
/**
* \internal
*
* \brief addToTransmitList - add an buffer to transmit list
*
* Add an buffer entry to the transmit list at the end of the list
*
* \return none
*
*/
static void addToTransmitList(
		CO_TRANS_QUEUE_T	*pTrBuf
	)
{
CO_TRANS_QUEUE_T	*pList;

	/* add it at last position */
	pList = pTransmitList;

	/* list empty ? */
	if (pList == NULL)  {
		/* save at first */
		pTransmitList = pTrBuf;
	} else {
		/* look for end of the list */
		while (pList->pNext != NULL)  {
			pList = pList->pNext;
		}
		pList->pNext = pTrBuf;
	}

	/* set next element to 0 */
	pTrBuf->pNext = NULL;
	/* set buffer state */
	pTrBuf->state = CO_TR_STATE_TO_TRANSMIT;
	/* printf(" TO TRANSMIT\n"); */
}


#ifdef CO_SYNC_SUPPORTED
/***************************************************************************/
/**
* \internal
*
* \brief addToTransmitListFirst - add an buffer to transmit list at first pos
*
* Add an buffer entry to the transmit list at the start of the list
*
* \return none
*
*/
static void addToTransmitListFirst(
		CO_TRANS_QUEUE_T	*pTrBuf
	)
{
CO_TRANS_QUEUE_T	*pList;

	/* add it at last position */
	pList = pTransmitList;

	/* list empty ? */
	if (pList == NULL)  {
		/* save at first */
		pTransmitList = pTrBuf;
		/* set next element to 0 */
		pTrBuf->pNext = NULL;
	} else {
		/* first entry to transmit ? */
		if (pList->state == CO_TR_STATE_TO_TRANSMIT)  {
			/* save as first element */
			pTrBuf->pNext = pList;
			pTransmitList = pTrBuf;
		} else {
			/* actual element is already transmitted */
			while (pList->pNext != NULL)  {
				if (pList->pNext->state == CO_TR_STATE_TO_TRANSMIT)  {
					break;
				}
				pList = pList->pNext;
			}

			/* save after actual buffer */
			pTrBuf->pNext = pList->pNext;
			pList->pNext = pTrBuf;
		}
	}

	/* set buffer state */
	pTrBuf->state = CO_TR_STATE_TO_TRANSMIT;
	/* printf(" TO TRANSMIT\n"); */
}
#endif /* CO_SYNC_SUPPORTED */


/***************************************************************************/
/**
* \internal
*
* \brief moveInhibitToTransmitList - move last entry to transmit list
*
* \return pointer to moved entry
*
*/
static CO_TRANS_QUEUE_T *moveInhibitToTransmitList(
		CO_CONST CO_COB_T	*pCob
	)
{
CO_TRANS_QUEUE_T	*pBuf;
CO_TRANS_QUEUE_T	*pTrData = NULL;
CO_COB_T	*cobPtr;

	pBuf = pInhibitList;

	/* look for last message at inhibit queue with this cobRef */
	while (pBuf != NULL)  {
		cobPtr = icoCobGetPointer(pBuf->cobRef);
		if (cobPtr == pCob)  {
			pTrData = pBuf;
		}

		/* search whole list - oldest message is at the end */
		pBuf = pBuf->pNext;
	}

	/* buffer found ? */
	if (pTrData != NULL)  {
		/* transmit data found - copy it to transmit list */

		/* disable driver buffer access */
		drvBufAccess = CO_FALSE;

		/* delete it from inhibit list */
		if (pInhibitList == pTrData)  {
			pInhibitList = pTrData->pNext;
		} else {
			pBuf = pInhibitList;
			while (pBuf->pNext != pTrData)  {
				pBuf = pBuf->pNext;
			}
			pBuf->pNext = pTrData->pNext;
		}

		/* and save it at transmit list */
		addToTransmitList(pTrData);
		/* printf("inhibitTimer: buf %ld TO TRANSMIT\n", BUF_NUM(pTrData)); */

#ifdef CO_GATEWAY_BUFFER
		/* send it by gateway */
		coGatewayTransmitMessage(&pTrData->msg);
#endif /* CO_GATEWAY_BUFFER */

		/* allow driver buffer access */
		drvBufAccess = CO_TRUE;

		/* start can transmission again */
		(void) codrvCanStartTransmission();
	}

	return(pTrData);
}


/***************************************************************************/
/**
* \internal
*
* \brief searchLastMessage - look for last message from same type
*
* look for message with same typ and status waiting
* Parameter all gibt an, ob auch aktuell schon versendete Nachrichten
* mit einbezogen werden sollen
*
* \return buffer index
*
*/
static CO_TRANS_QUEUE_T	*searchLastMessage(
		UNSIGNED16		cobRef,		/* cob reference */
		BOOL_T			all			/* use all messages incl. transmitted messages*/
	)
{
CO_TRANS_QUEUE_T	*pList;
CO_TRANS_QUEUE_T	*pLast = NULL;

	/* as first, look on inhibit list */
	pList = pInhibitList;
	while (pList != NULL)  {
		/* correct cob reference ? */
		if (pList->cobRef == cobRef)  {
			/* yes, return pointer */
			return(pList);
		}
		/* try until end of list */
		pList = pList->pNext;
	}

	/* no entry at inhibit list found - try the same at transmit list */
	pList = pTransmitList;
	while (pList != NULL)  {
		/* cobref found ? */
		if (pList->cobRef == cobRef)  {
			/* depending on parameter all */
			if (all == CO_TRUE)  {
				/* check state of the message */
				if (pList->state != CO_TR_STATE_FREE)  {
					pLast = pList;
				}
			} else {
				if (pList->state == CO_TR_STATE_TO_TRANSMIT)  {
					pLast = pList;
				}
			}

			/* if we have found an entry, return it */
		}
		pList = pList->pNext;
	}

	return(pLast);
}


/***************************************************************************/
/**
* \internal
*
* \brief icoQueueGetTransBufFillState - get filling state of transmit buffer
*
* get filling state of transmit buffer (0..100)%
* 0 - buffer empty
* 100 - buffer full
*
* \return buffer fill state
*
*/
UNSIGNED32 icoQueueGetTransBufFillState(
		void	/* no parameter */
	)
{
UNSIGNED32	cnt = 0u;
UNSIGNED32	i;

	/* look for next free buffer */
#ifdef OLD_FIXED_BUFFER
	for (i = 0u; i < CO_CONFIG_TRANS_BUFFER_CNT; i++)  {
		if (trDataBuffer[i].state == CO_TR_STATE_FREE)  {
			cnt++;
		}
	}

	return(cnt * 100u / CO_CONFIG_TRANS_BUFFER_CNT);
#else /* OLD_FIXED_BUFFER */
	for (i = 0u; i < CO_CONFIG_TRANS_BUFFER_CNT; i++)  {
		if (trBuffer[i].state == CO_TR_STATE_FREE)  {
			cnt++;
		}
	}

	cnt *= 100u;
	
	return(100u - (cnt / CO_CONFIG_TRANS_BUFFER_CNT));
#endif /* OLD_FIXED_BUFFER */
}


/***************************************************************************/
/**
* \internal
*
* \brief getNextTransBuf - get next transmit buffer
*
* get next transmit buffer depending on the last transmission time
*
* \return buffer index
*/
static CO_TRANS_QUEUE_T	*getNextTransBuf(
		void	/* no parameter */
	)
{
UNSIGNED16	i;

	/* look for next free buffer */
#ifdef OLD_FIXED_BUFFER
	for (i = 0u; i < CO_CONFIG_TRANS_BUFFER_CNT; i++)  {
		if (trDataBuffer[i].state == CO_TR_STATE_FREE)  {
			/* check for buffer full -
			 * it comes over only if we use the last buffer */
			if (i == (CO_CONFIG_TRANS_BUFFER_CNT - 1u))  {
				coCommStateEvent(CO_COMM_STATE_EVENT_TR_QUEUE_FULL);
			}
			return(&trDataBuffer[i]);
		}
	}
#else /* OLD_FIXED_BUFFER */
	for (i = 0u; i < CO_CONFIG_TRANS_BUFFER_CNT; i++)  {
		if (trBuffer[i].state == CO_TR_STATE_FREE)  {
			/* check for buffer full -
			 * it comes over only if we use the last buffer */
			if (i == (CO_CONFIG_TRANS_BUFFER_CNT - 1u))  {
				coCommStateEvent(CO_COMM_STATE_EVENT_TR_QUEUE_FULL);
			}
			return(&trBuffer[i]);
		}
	}
#endif /* OLD_FIXED_BUFFER */

	return(NULL);
}


/***************************************************************************/
/**
* \brief coQueueGetNextTransmitMessage - get next message to transmit
*
* This function returns the next available transmit message
* from the transmit queue.
* It increments also trBufferRdCnt.
*
* \return CO_CAN_TR_MSG_T* pointer to next tx message
* \retval !NULL
*	pointer to transmit queue entry
* \retval NULL
*	no message available
*
*/
CO_CAN_TR_MSG_T *coQueueGetNextTransmitMessage(
		void	/* no parameter */
	)
{
CO_TRANS_QUEUE_T *pBuf;

	/* access to buffer allowed ? */
	if (drvBufAccess != CO_TRUE)  {
		return(NULL);
	}

	/* start with first entry */
	pBuf = pTransmitList;
	while (pBuf != NULL)  {
		/* correct state ? */
		if (pBuf->state == CO_TR_STATE_TO_TRANSMIT)  {
			/* set state to active */
			pBuf->state = CO_TR_STATE_ACTIVE;
			/* save handle */
			pBuf->msg.handle = pBuf;
			return(&pBuf->msg);
		}

		pBuf = pBuf->pNext;
	}

	return(NULL);
}


/***************************************************************************/
/**
* \brief coQueueMsgTransmitted - message was transmitted
*
* This function is called after a message was succesfull transmitted.
*
* \return none
*
*/
void coQueueMsgTransmitted(
		const CO_CAN_TR_MSG_T *pBuf		/**< pointer to transmitted message */
	)
{
CO_TRANS_QUEUE_T	*pQueue;

	pQueue = (CO_TRANS_QUEUE_T	*)pBuf->handle;
	pQueue->state = CO_TR_STATE_TRANSMITTED;
}




/***************************************************************************/
/**
* \internal
*
* \brief icoQueueHandler
*
* \return none
*
*/
void icoQueueHandler(
		void	/* no parameter */
	)
{
CO_COB_T	*pCob;

/*
* trBufferRdCnt is getting changed in interrupts, is there additional guarding needed?
*/

	/* recBuffer was full ? */
	if (recBufFull == CO_TRUE)  {
		recBufFull = CO_FALSE;
		/* inform application, if buffer is full */
		coCommStateEvent(CO_COMM_STATE_EVENT_REC_QUEUE_OVERFLOW);
	}

	/* no entries at transmit list, return */
	if (pTransmitList == NULL)  {
		return;
	}

	/* check all buffer with state transmitted */
	while (pTransmitList->state == CO_TR_STATE_TRANSMITTED)  {
		/* delete transmitted messages ...... and save inhibit */

		/* get cob reference */
		pCob = icoCobGetPointer(pTransmitList->cobRef);
		if (pCob != NULL)  {
			/* if inhibit is != 0 */
			if (pCob->inhibit != 0u)  {
				/* start inhibit timer */
				(void)coTimerStart(&pCob->inhibitTimer,
						pCob->inhibit * 100ul,
						inhibitTimer, pCob, CO_TIMER_ATTR_ROUNDUP); /*lint !e960 */
				/* Derogation MisraC2004 R.16.9 function identifier used
				 * without '&' or parenthesized parameter */
			}
			/* should this message create a tx acknowledge */
			if ((pTransmitList->msg.flags & CO_COBFLAG_IND)
					== CO_COBFLAG_IND)  {

#ifdef ISOTP_CLIENT_CONNECTION_CNT
				if (pCob->service == CO_SERVICE_ISOTP_CLIENT)  {
					iisoTpClientTxAck(pTransmitList->cobRef, &pTransmitList->msg);
				}
#endif /* ISOTP_CLIENT_CONNECTION_CNT */
			}

		}
		/* set buffer state */
		pTransmitList->state = CO_TR_STATE_FREE;

		/* set transmit list to next buffer */
		pTransmitList = pTransmitList->pNext;
		if (pTransmitList == NULL)  {
			break;
		}
	}

	/* transmit list is empty */
	if (pTransmitList == NULL)  {
		/* inhibit list is also empty, signal it to eventHandler */
		if (pInhibitList == NULL)  {
			coCommStateEvent(CO_COMM_STATE_EVENT_TR_QUEUE_EMPTY);
		}
	} else {
		/* should buffer be transmitted? */
		if (pTransmitList->state == CO_TR_STATE_TO_TRANSMIT)  {
			(void) codrvCanStartTransmission();
		}
	}

}


/***************************************************************************/
/**
* \internal
*
* \brief icoQueueDeleteInhibit - delete inhibit entries of this cobRef
*
* \return none
*
*/
void icoQueueDeleteInhibit(
		COB_REFERENZ_T	cobRef			/* cob reference */
	)
{
CO_COB_T	*pCob;
CO_TRANS_QUEUE_T	*pData = NULL;

	pCob = icoCobGetPointer(cobRef);
	if (pCob == NULL)  {
		return;
	}

	/* delete inhibit until list is empty */
	do {
		pData = moveInhibitToTransmitList(pCob);
	} while (pData != NULL);
}


/***************************************************************************/
/**
* \internal
*
* \brief inhibitTimer - inhibit timer has been ellapsed
*
* \return none
*
*/
static void inhibitTimer(
		void			*pData
	)
{
CO_COB_T	*pCob;

	pCob = (CO_COB_T *)pData;

	(void)moveInhibitToTransmitList(pCob);
}


#ifdef CO_EVENT_SLEEP
/***************************************************************************/
/**
* \internal
*
* \brief icoQueueDisable - enable/disable message queues
*
* \return none
*
*/
void icoQueueDisable(
		BOOL_T	on
	)
{
	queueDisabled = on;

	/* delete all entries at the inhibit queue */
	pInhibitList = NULL;
}
#endif /* CO_EVENT_SLEEP */



/***************************************************************************/
/**
*
* \brief coQueueInit - (re)init queues
*
* This function clears the transmit and the receive queue
*
* \return none
*
*/
void coQueueInit(
		void	/* no parameter */
	)
{
UNSIGNED16	i;

	/* transmit queue */
#ifdef OLD_FIXED_BUFFER
	for (i = 0u; i < CO_CONFIG_TRANS_BUFFER_CNT; i++)  {
		trDataBuffer[i].state = CO_TR_STATE_FREE;
	}
#else /* OLD_FIXED_BUFFER */
	for (i = 0u; i < CO_CONFIG_TRANS_BUFFER_CNT; i++)  {
		trBuffer[i].state = CO_TR_STATE_FREE;
	}
#endif /* OLD_FIXED_BUFFER */

	drvBufAccess = CO_TRUE;
	pInhibitList = NULL;
	pTransmitList = NULL;

	/* receive queue */
	recBufferWrCnt = 0u;
	recBufferRdCnt = 0u;
	recBufFull = CO_FALSE;
#ifdef CO_EVENT_SLEEP
	queueDisabled = CO_FALSE;
#endif /* CO_EVENT_SLEEP */
}


#ifdef CO_SYNC_SUPPORTED
/***************************************************************************/
/**
*
* \internal
*
* \brief icoQueueSetSyncId - set sync cob-id 
*
* Set SYNC Cob-id for priority handling of sync messages
*
*
*/
void icoQueueSetSyncId(
		UNSIGNED32		syncId,			/* can id */
		UNSIGNED8		flags,			/* can flags */
		BOOL_T			enable			/* enable id */
	)
{
	if (enable == CO_TRUE)  {
		syncBuf.canId = syncId;
		syncBuf.flags = flags & (CO_COBFLAG_EXTENDED | CO_COBFLAG_RTR);
	} else {
		syncBuf.canId = 0xffffffffu;
		syncBuf.state = CO_SYNCBUF_STATE_EMPTY;
	}
}
#endif /* CO_SYNC_SUPPORTED */


#ifdef CO_GATEWAY_BUFFER
/***************************************************************************/
/**
*
* \brief coQueueRecMsgFromGw - receive message from gateway
*
* This function is called, if a new message is available on the gw
* The message is saved at the receive buffer and the transmit buffer.
* O it can be used from the CANopen stack and send out to the CAN.
*
* \return none
*
*/
void coQueueRecMsgFromGw(
		CO_CAN_REC_MSG_T	*pMsg		/* can message */
	)
{
UNSIGNED16	tmpCnt;
CO_TRANS_QUEUE_T	*pTrBuf;

	/* try to save at receive buffer */
	tmpCnt = gwRecBufferWrCnt + 1u;
	if (tmpCnt >= CO_CONFIG_REC_BUFFER_CNT)  {
		tmpCnt = 0u;
	}
	if (tmpCnt != gwRecBufferRdCnt)  {
		/* fill out standard values */
		pMsg->canCob.flags |= CO_COBFLAG_ENABLED;

		/* emty entry found */
		memcpy(&gwRecDataBuffer[tmpCnt].msg, pMsg, sizeof(CO_CAN_REC_MSG_T));

		gwRecBufferWrCnt = tmpCnt;

		coCommTaskSet(CO_COMMTASK_EVENT_MSG_AVAIL);
	} else {
		coCommStateEvent(CO_COMM_STATE_EVENT_REC_QUEUE_OVERFLOW);
	}


	/* transmit buffer */

	drvBufAccess = CO_FALSE;
	pTrBuf = getNextTransBuf();
	if (pTrBuf == NULL)  {
		/* allow driver buffer access */
		drvBufAccess = CO_TRUE;

		/* start can transmission again */
		(void) codrvCanStartTransmission();

		/* inform application */
		coCommStateEvent(CO_COMM_STATE_EVENT_TR_QUEUE_OVERFLOW);

		return;
	}

	memcpy(&pTrBuf->msg, pMsg, sizeof(CO_CAN_TR_MSG_T));
	pTrBuf->cobRef = 0xffff;

	addToTransmitList(pTrBuf);

	/* allow driver buffer access */
	drvBufAccess = CO_TRUE;

	/* start can transmission again */
	(void) codrvCanStartTransmission();
}
#endif /* CO_GATEWAY_BUFFER */


/***************************************************************************/
/**
* \internal
*
* \brief icoQueuVarInit - init lokal variables
*
* \return none
*
*/
void icoQueueVarInit(
		CO_CONST UNSIGNED16	*recQueueCnt,	/* receive queue cnt */
		CO_CONST UNSIGNED16	*trQueueCnt		/* transmit queue cnt */
	)
{
(void)trQueueCnt;

	{
#ifdef OLD_FIXED_BUFFER
#else /* OLD_FIXED_BUFFER */
		recBufferSize = (UNSIGNED16)(*recQueueCnt * (sizeof(CO_RECBUF_HDR_T) + CO_CAN_MAX_DATA_LEN));
		pRecBuffer = &recBuffer[0];
#endif /* OLD_FIXED_BUFFER */

		recBufferWrCnt = 0u;
		recBufferRdCnt= 0u;
		drvBufAccess = CO_TRUE;
		pInhibitList = NULL;
		pTransmitList = NULL;
		recBufFull = CO_FALSE;

#ifdef CO_EVENT_SLEEP
		queueDisabled = CO_FALSE;
#endif /* CO_EVENT_SLEEP */

#ifdef CO_SYNC_SUPPORTED
		syncBuf.canId = 0xffffffffu;
		syncBuf.state = CO_SYNCBUF_STATE_EMPTY;
#endif /* CO_SYNC_SUPPORTED */
	}

#ifdef CO_GATEWAY_BUFFER
	gwRecBufferWrCnt = 0u;
	gwRecBufferRdCnt = 0u;
#endif /* CO_GATEWAY_BUFFER */

#ifdef CO_EVAL_MODE
	demoTrCnt = 0;
# ifdef CO_EVAL_MODE_ENABLED
	demoTimerEnable = 1u;
# else /* CO_EVAL_MODE_ENABLED */
	demoTimerEnable = 0u;
# endif /* CO_EVAL_MODE_ENABLED */
#endif /* CO_EVAL_MODE */
}


#ifdef xxx
void printBufState(
	char 	*info
	)
{
UNSIGNED8	i;

	printf("%s: wr: %d, rd %d\n", info, trBufferWrCnt, trBufferRdCnt);

	for (i = 0; i < CO_CONFIG_TRANS_BUFFER_CNT; i++)  {
//CO_TRANS_QUEUE_T	trDataBuffer[CO_CONFIG_TRANS_BUFFER_CNT];
		printf("%d: state %d, id %x\n", i,
			trDataBuffer[i].state,
			trDataBuffer[i].cobRef);
	}
	printf("-----------------------------\n");
}


int coPrintTime()
{
#include <sys/time.h>
struct timeval tv;
struct timezone tz;

	tz.tz_minuteswest = 0;
	tz.tz_dsttime = 0;

	gettimeofday(&tv, &tz);
	printf("time %d:%06d", tv.tv_sec, tv.tv_usec);
}
#endif /* xxx */
