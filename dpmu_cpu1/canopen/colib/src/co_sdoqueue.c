/*
* co_sdoqueue.c - sdo queue
*
* Copyright (c) 2013-2020 emotas embedded communication GmbH
*-------------------------------------------------------------------
* $Id: co_sdoqueue.c 34711 2021-01-28 10:28:17Z boe $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \brief SDO handling with queuing
*
* \file co_sdoqueue.c
* Functions for queuing SDO requests.
*
* Functions are only available in Classical CAN mode
*/


/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <gen_define.h>
#include "ico_indication.h"

#ifdef CO_SDO_QUEUE
#include <co_commtask.h>
#include <co_drv.h>
#include <co_timer.h>
#include <co_sdo.h>
#include "ico_cobhandler.h"
#include "ico_queue.h"
#include "ico_event.h"
#include "ico_sdoclient.h"
#include "co_odaccess.h"
#include "ico_odaccess.h"
#ifdef CO_SDO_NETWORKING
# include "ico_sdoserver.h"
#endif /* CO_SDO_NETWORKING */
#include "ico_sdo.h"

/* constant definitions
---------------------------------------------------------------------------*/
typedef enum {
	CO_SDO_QUEUE_STATE_NEW,
	CO_SDO_QUEUE_STATE_TRANSMITTING,
	CO_SDO_QUEUE_STATE_FINISHED
} CO_SDO_QUEUE_STATE_T;


/* SDO queue entry */
typedef struct {
	UNSIGNED32	dataLen;			/* datalen */
	UNSIGNED32	timeOut;			/* timeout in msec */
	UNSIGNED8	*pData;				/* pointer of data */
	void		*pFctPara;			/* function parameter */
	CO_SDO_QUEUE_IND_T	pFct;		/* pointer to function */
	UNSIGNED16	remoteIndex;		/* remote index */
	UNSIGNED16	localIndex;			/* local index */
	UNSIGNED16	numeric;			/* numeric transfer (with byte swapping) */
	UNSIGNED8	data[4];			/* data (saved for write access) */
	UNSIGNED8	sdoNr;				/* sdo number */
	UNSIGNED8	remoteSubIndex;		/* remote subindex */
	UNSIGNED8	localSubIndex;		/* local subindex */
	CO_SDO_QUEUE_STATE_T state;		/* internal state */
	BOOL_T		write;				/* read/write */
#ifdef CO_SDO_NETWORKING 
	UNSIGNED16	network;			/* remote network */
	UNSIGNED8	node;				/* remote node */
#endif /* CO_SDO_NETWORKING */

} CO_SDO_QUEUE_T;


/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
static CO_SDO_QUEUE_T	*getSdoBuf(UNSIGNED16 idx);
static RET_T sdoQueueAddTransfer(BOOL_T write, UNSIGNED8 sdoNr,
#ifdef CO_SDO_NETWORKING 
		UNSIGNED16	networkNr, UNSIGNED8	nodeNr,
#endif /* CO_SDO_NETWORKING */
		UNSIGNED16	remoteIndex, UNSIGNED8	remoteSubIndex,
		UNSIGNED16	localIndex,	UNSIGNED8	localSubIndex,
		UNSIGNED8	*pData, UNSIGNED32	dataLen, UNSIGNED16	numeric,
		CO_SDO_QUEUE_IND_T	pFct, void	*pFctPara,
		UNSIGNED32	timeOut);
static void startSdoTransfer(void *ptr);
#ifdef CO_SDOQUEUE_INTERNAL_ACCESS
static RET_T internalAccess(CO_SDO_QUEUE_T	*pSdoBuf);
static void internalSdoFinished(void *pData);
#endif /* CO_SDOQUEUE_INTERNAL_ACCESS */

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
static CO_SDO_QUEUE_T	sdoBuf[CO_SDO_QUEUE_LEN];
static UNSIGNED16	rdIdx = { 0u };
static UNSIGNED16	wrIdx = { 0u };
static CO_EVENT_T	startSdoEvent;
static UNSIGNED32	sdoReadSize = { 0u };




/***************************************************************************/
/**
*
* \brief coSdoQueueAddTransfer - add sdo transfer to sdo queue handler
*
* This function is only available in classical CAN mode.
*
* The data for SDO write accesses are internal buffered
* for transfers up to 4 bytes, so the data pointer can be released
* after calling this function.
* For larger transfers, the data pointer has to be valid until the transfer
* is finished.
*
* \return
*	RET_T
*/
RET_T coSdoQueueAddTransfer(
		BOOL_T		write,				/**< write/read access */
		UNSIGNED8	sdoNr,				/**< sdo number */
		UNSIGNED16	index,				/**< index */
		UNSIGNED8	subIndex,			/**< subIndex */
		UNSIGNED8	*pData,				/**< pointer to transfer data */
		UNSIGNED32	dataLen,			/**< len of transfer data */
		CO_SDO_QUEUE_IND_T	pFct,		/**< pointer to finish function */
		void		*pFctPara,			/**< pointer to data field for finish function */
		UNSIGNED32	timeOut				/**< timeout in msec */
	)
{
RET_T	retVal;

	retVal = sdoQueueAddTransfer(write, sdoNr,
#ifdef CO_SDO_NETWORKING 
		0u, 0u,
#endif /* CO_SDO_NETWORKING */
		index, subIndex, 0u, 0u,
		pData, dataLen, 1u, pFct, pFctPara, timeOut);

	return(retVal);
}


/***************************************************************************/
/**
*
* \brief coSdoQueueAddStringTransfer - add string sdo transfer to sdo queue handler
*
* This function is only available in classical CAN mode.
*
* The data are handled as string - without any changes to CANopen little endian transfer format
*
* The data for SDO write accesses are internal buffered for transfers up to 4 bytes,
* so the data pointer can be released after calling this function.
* For larger transfers, the data pointer has to be valid until the transfer
* is finished.
*
* \return
*	RET_T
*/
RET_T coSdoQueueAddStringTransfer(
		BOOL_T		write,				/**< write/read access */
		UNSIGNED8	sdoNr,				/**< sdo number */
		UNSIGNED16	index,				/**< index */
		UNSIGNED8	subIndex,			/**< subIndex */
		UNSIGNED8	*pData,				/**< pointer to transfer data */
		UNSIGNED32	dataLen,			/**< len of transfer data */
		CO_SDO_QUEUE_IND_T	pFct,		/**< pointer to finish function */
		void		*pFctPara,			/**< pointer to data field for finish function */
		UNSIGNED32	timeOut				/**< timeout in msec */
	)
{
RET_T	retVal;

	retVal = sdoQueueAddTransfer(write, sdoNr,
#ifdef CO_SDO_NETWORKING 
		0u, 0u,
#endif /* CO_SDO_NETWORKING */
		index, subIndex, 0u, 0u,
		pData, dataLen, 0u, pFct, pFctPara, timeOut);

	return(retVal);
}


/***************************************************************************/
/**
*
* \brief coSdoQueueAddOdTransfer - add sdo transfer to sdo queue handler
*
* This function is only available in classical CAN mode.
*
* This function can be used to add sdo transfers 
* from local object dicationary to remote dictionary to a queue.
* If a tranfer was finished, the next will start automatically.
* After each transfer, the given function with the parameter are called.
*
* \return
*	RET_T
*/
RET_T coSdoQueueAddOdTransfer(
		BOOL_T		write,			/**< write/read access */
		UNSIGNED8	sdoNr,			/**< sdo number */
		UNSIGNED16	remoteIndex,	/**< remote index */
		UNSIGNED8	remoteSubIndex,	/**< remote subIndex */
		UNSIGNED16	localIndex,		/**< local index */
		UNSIGNED8	localSubIndex,	/**< local subindex */
		CO_SDO_QUEUE_IND_T	pFct,	/**< pointer to finish function */
		void		*pFctPara,		/**< pointer to data field for finish function */
		UNSIGNED32	timeOut			/**< timeout in msec */
	)
{
RET_T	retVal;
UNSIGNED8	*pData;
UNSIGNED32	dataLen;
CO_CONST CO_OBJECT_DESC_T *pDescPtr;

	pData = coOdGetObjAddr(localIndex, localSubIndex);
	if (pData == NULL)  {
		return(RET_OD_ACCESS_ERROR);
	}

	retVal = coOdGetObjDescPtr(localIndex, localSubIndex, &pDescPtr);
	if (retVal != RET_OK)  {
		return(retVal);
	}

	/* only main datatypes are supported */
	switch(pDescPtr->dType)  {
		case CO_DTYPE_BOOL_VAR:
		case CO_DTYPE_BOOL_PTR:
		case CO_DTYPE_U8_VAR:
		case CO_DTYPE_U8_PTR:
		case CO_DTYPE_U16_VAR:
		case CO_DTYPE_U16_PTR:
		case CO_DTYPE_U32_VAR:
		case CO_DTYPE_U32_PTR:
		case CO_DTYPE_I8_VAR:
		case CO_DTYPE_I8_PTR:
		case CO_DTYPE_I16_VAR:
		case CO_DTYPE_I16_PTR:
		case CO_DTYPE_I32_VAR:
		case CO_DTYPE_I32_PTR:
			break;
		default:
			return(RET_PARAMETER_INCOMPATIBLE);
	}

	dataLen = coOdGetObjSize(pDescPtr);

	retVal = sdoQueueAddTransfer(write, sdoNr,
#ifdef CO_SDO_NETWORKING 
		0u, 0u,
#endif /* CO_SDO_NETWORKING */
		remoteIndex, remoteSubIndex,
		localIndex, localSubIndex,
		pData, dataLen, 1u, pFct, pFctPara, timeOut);

	return(retVal);
}


#ifdef CO_SDO_NETWORKING 
/***************************************************************************/
/**
*
* \brief coSdoQueueAddTransfer - add sdo transfer to sdo queue handler
*
* This function is only available in classical CAN mode.
*
* This function can be used to add sdo transfers to a queue.
* If a transfer was finished, the next will start automatically.
* After each transfer, the given function with the parameter are called.
*
* The data for SDO write accesses are internal buffered
* for transfers up to 4 bytes, so the data pointer can be released
* after calling this function.
* For larger transfers, the data pointer has to be valid until the transfer
* is finished.
*
*
* \return
*	RET_T
*/
RET_T coSdoQueueAddNetworkTransfer(
		BOOL_T		write,				/**< write/read access */
		UNSIGNED8	sdoNr,				/**< sdo number */
		UNSIGNED16	networkNr,			/**< network number */
		UNSIGNED8	nodeNr,				/**< node number */
		UNSIGNED16	index,				/**< index */
		UNSIGNED8	subIndex,			/**< subIndex */
		UNSIGNED8	*pData,				/**< pointer to transfer data */
		UNSIGNED32	dataLen,			/**< len of transfer data */
		CO_SDO_QUEUE_IND_T	pFct,		/**< pointer to finish function */
		void		*pFctPara,			/**< pointer to data field for finish function */
		UNSIGNED32	timeOut				/**< timeout in msec */
	)
{
RET_T	retVal;

	retVal = sdoQueueAddTransfer(write, sdoNr,
		networkNr, nodeNr, index, subIndex, 0u, 0u,
		pData, dataLen, 1u, pFct, pFctPara, timeOut);

	return(retVal);
}
#endif /* CO_SDO_NETWORKING */


/***************************************************************************/
/**
* \internal
*
* \brief sdoQueueAddTransfer - add sdo transfer to sdo queue handler
*
* \return
*	RET_T
*/
static RET_T sdoQueueAddTransfer(
		BOOL_T		write,				/* write/read access */
		UNSIGNED8	sdoNr,				/* sdo number */
#ifdef CO_SDO_NETWORKING 
		UNSIGNED16	networkNr,			/* network number */
		UNSIGNED8	nodeNr,				/* node number */
#endif /* CO_SDO_NETWORKING */
		UNSIGNED16	remoteIndex,		/* index */
		UNSIGNED8	remoteSubIndex,		/* subIndex */
		UNSIGNED16	localIndex,			/* index */
		UNSIGNED8	localSubIndex,		/* subIndex */
		UNSIGNED8	*pData,				/* pointer to transfer data */
		UNSIGNED32	dataLen,			/* len of transfer data */
		UNSIGNED16	numeric,			/* numeric flag */
		CO_SDO_QUEUE_IND_T	pFct,		/* pointer to finish function */
		void		*pFctPara,			/* pointer to data field for finish function */
		UNSIGNED32	timeOut				/* timeout in msec */
	)
{
CO_SDO_QUEUE_T	*pSdoBuf;
UNSIGNED16	idx;
#ifdef CO_SDOQUEUE_INTERNAL_ACCESS
#else /* CO_SDOQUEUE_INTERNAL_ACCESS */
UNSIGNED32   cobId;
RET_T		retVal;
#endif /* CO_SDOQUEUE_INTERNAL_ACCESS */

	CO_DEBUG4("coSdoAddTransfer: write: %d, sdoNr: %d, idx: %x:%d, ",
		write, sdoNr, index, subIndex);
	CO_DEBUG4("data %x %x %x %x\n",
		pData[0], pData[1], pData[2], pData[3]);
	CO_DEBUG2("coSdoAddTransfer start: rd: %d wr:%d\n", rdIdx, wrIdx);

#ifdef CO_SDOQUEUE_INTERNAL_ACCESS
	/* allow access to local OD */
	if (sdoNr > 128u)  {
		return(RET_INVALID_PARAMETER);
	}
#else /* CO_SDOQUEUE_INTERNAL_ACCESS */
	if ((sdoNr < 1u) || (sdoNr > 128u)) {
		return(RET_INVALID_PARAMETER);
	}

	/* check, if sdo is enabled */
	retVal = coOdGetObj_u32(sdoNr + (0x1280u - 1u), 1u, &cobId);
	if (retVal != RET_OK) {
		return(retVal);
	}
	if ((cobId & CO_COB_INVALID) != 0u) {
		return(RET_COB_DISABLED);
	}
	retVal = coOdGetObj_u32(sdoNr + (0x1280u - 1u), 2u, &cobId);
	if (retVal != RET_OK) {
		return(retVal);
	}
	if ((cobId & CO_COB_INVALID) != 0u) {
		return(RET_COB_DISABLED);
	}
#endif /* CO_SDOQUEUE_INTERNAL_ACCESS */


	idx = wrIdx + 1u;
	if (idx == CO_SDO_QUEUE_LEN)  {
		idx = 0u;
	}
	if (idx == rdIdx)  {
		return(RET_OUT_OF_MEMORY);
	}

	pSdoBuf = getSdoBuf(wrIdx);
	pSdoBuf->write = write;
	pSdoBuf->sdoNr = sdoNr;
	pSdoBuf->numeric = numeric;
#ifdef CO_SDO_NETWORKING 
	pSdoBuf->network = networkNr;
	pSdoBuf->node = nodeNr;
#endif /* CO_SDO_NETWORKING */

	pSdoBuf->remoteIndex = remoteIndex;
	pSdoBuf->remoteSubIndex = remoteSubIndex;
	pSdoBuf->localIndex = localIndex;
	pSdoBuf->localSubIndex = localSubIndex;
	pSdoBuf->timeOut = timeOut;
	if (write == CO_TRUE)  {
		/* use internal buffer for transfer up to 4 bytes */
		if (dataLen > 4u)  {
			pSdoBuf->pData = pData;
		} else {
			(void)coNumMemcpy(&pSdoBuf->data[0], (CO_CONST void *)pData,
				(UNSIGNED32)dataLen, 1u);
			pSdoBuf->pData = pSdoBuf->data;
		}
	} else {
		if (localIndex != 0u)  {
			pSdoBuf->pData = &pSdoBuf->data[0];
		} else {
			pSdoBuf->pData = pData;
		}
	}
	pSdoBuf->dataLen = dataLen;
	pSdoBuf->pFct = pFct;
	pSdoBuf->pFctPara = pFctPara;
	pSdoBuf->state = CO_SDO_QUEUE_STATE_NEW;

	wrIdx = idx;

	CO_DEBUG2("ebSdoAddTransfer end: rd: %d wr:%d\n", rdIdx, wrIdx);

	/* try to start transfer */
	startSdoTransfer(NULL);

	return(RET_OK);
}


/***************************************************************************/
/**
* \internal
*
* \brief startSdoTransfer - try to start next sdo transfer
*
*
* \return
*	void
*/
static void startSdoTransfer(
		void	*ptr
	)
{
RET_T	retVal;
CO_SDO_QUEUE_T	*pSdoBuf;
(void)ptr;

#ifdef ___DEBUG__
{
UNSIGNED8	rd = rdIdx, wr = wrIdx;
CO_SDO_QUEUE_T	*pSdoBuf;


	while (rd != wr)  {
		pSdoBuf = getSdoBuf(rd);
		EB_DEBUG("buf %d, state %d, idx %x:%d\n", rd,
			pSdoBuf->state,
			pSdoBuf->remoteIndex,
			pSdoBuf->remoteSubIndex);
		rd++;
		if (rd == CO_SDO_QUEUE_LEN)  {
			rd = 0u;
		}
	}
}
#endif /* __DEBUG__ */

	CO_DEBUG2("startSdoTransfer start: rd: %d wr:%d\n", rdIdx, wrIdx);

	/* data to transmit ? */
	if (rdIdx != wrIdx)  {
		/* buffer waiting ? */
		pSdoBuf = getSdoBuf(rdIdx);
		if (pSdoBuf->state == CO_SDO_QUEUE_STATE_NEW)  {

#ifdef CO_SDOQUEUE_INTERNAL_ACCESS
			if (pSdoBuf->sdoNr == 0u)  {
				retVal = internalAccess(pSdoBuf);
			} else
#endif /* CO_SDOQUEUE_INTERNAL_ACCESS */
			if (pSdoBuf->write == CO_TRUE)  {
				/* write */
				CO_DEBUG3("startSdoTransfer: write sdoNr: %d, idx: %x:%d",
					pSdoBuf->sdoNr, pSdoBuf->remoteIndex,
					pSdoBuf->remoteSubIndex);
				CO_DEBUG4(", data %x %x %x %x\n",
					pSdoBuf->pData[0], pSdoBuf->pData[1],
					pSdoBuf->pData[2], pSdoBuf->pData[3]);

#ifdef CO_SDO_NETWORKING 
				if (pSdoBuf->network != 0u)  {
					retVal = coSdoNetworkWrite(pSdoBuf->sdoNr,
						pSdoBuf->network, pSdoBuf->node,
						pSdoBuf->remoteIndex, pSdoBuf->remoteSubIndex,
						pSdoBuf->pData, pSdoBuf->dataLen, pSdoBuf->numeric,
						pSdoBuf->timeOut);
				} else
#endif /* CO_SDO_NETWORKING */
				{
					retVal = coSdoWrite(pSdoBuf->sdoNr,
						pSdoBuf->remoteIndex, pSdoBuf->remoteSubIndex,
						pSdoBuf->pData, pSdoBuf->dataLen, pSdoBuf->numeric,
						pSdoBuf->timeOut);
				}
			} else {
				/* read */
				CO_DEBUG3("startSdoTransfer: read sdoNr: %d, idx: %x:%d\n",
					pSdoBuf->sdoNr, pSdoBuf->remoteIndex,
					pSdoBuf->remoteSubIndex);
				/* check for internal transfers */

#ifdef CO_SDO_NETWORKING 
				if (pSdoBuf->network != 0u)  {
					retVal = coSdoNetworkRead(pSdoBuf->sdoNr,
						pSdoBuf->network, pSdoBuf->node,
						pSdoBuf->remoteIndex, pSdoBuf->remoteSubIndex,
						pSdoBuf->pData, pSdoBuf->dataLen, pSdoBuf->numeric, pSdoBuf->timeOut);
				} else
#endif /* CO_SDO_NETWORKING */
				{
					retVal = coSdoRead(pSdoBuf->sdoNr,
						pSdoBuf->remoteIndex, pSdoBuf->remoteSubIndex,
						pSdoBuf->pData, pSdoBuf->dataLen, pSdoBuf->numeric, pSdoBuf->timeOut);
				}
			}
			if (retVal != RET_OK)  {
				if (retVal != RET_SERVICE_BUSY)  {
					/* abort transfer */
					if (pSdoBuf->pFct != NULL)  {
						pSdoBuf->pFct(pSdoBuf->pFctPara, 0x06010000uL);
					}

					pSdoBuf->state = CO_SDO_QUEUE_STATE_FINISHED;
				}

				/* start function again */
				(void)icoEventStart(&startSdoEvent,
						startSdoTransfer, NULL); /*lint !e960 */
				/* Derogation MisraC2004 R.16.9 function identifier used
				 * without '&' or parenthesized parameter */
				return;
			}
			pSdoBuf->state = CO_SDO_QUEUE_STATE_TRANSMITTING;
		}
	}

	CO_DEBUG2("startSdoTransfer end: rd: %d wr:%d\n", rdIdx, wrIdx);
}


/***************************************************************************/
/**
* \internal
*
* \brief icoSdoClientQueueInd
*
*
* \return
*	void
*/
void icoSdoClientQueueInd(
		UNSIGNED8		sdoNr,			/* sdo number */
		UNSIGNED16		index,			/* index */
		UNSIGNED8		subIndex,		/* subindex */
		UNSIGNED32		size,			/* size of read transfer */
		UNSIGNED32		result			/* result of transfer */
	)
{
CO_CONST CO_OBJECT_DESC_T *pDescPtr;
CO_SDO_QUEUE_T	*pSdoBuf;
UNSIGNED16	u16;
UNSIGNED32	u32;
INTEGER16	i16;
INTEGER32	i32;

	pSdoBuf = getSdoBuf(rdIdx);

	CO_DEBUG2("coSdoClientQueueInd start: rd: %d wr:%d\n", rdIdx, wrIdx);
	CO_DEBUG4("coSdoClientQueueInd: sdoNr: %d, idx: %x:%d, result: %x\n",
		sdoNr, index, subIndex, result);

	CO_DEBUG4("coSdoClientQueueInd: sdoBuf[%d].sdoNr: %d sdoBuf[%d].index: %x\n",
		rdIdx, pSdoBuf->sdoNr, rdIdx, pSdoBuf->remoteIndex);

	if ((sdoNr == pSdoBuf->sdoNr)
	 && (index == pSdoBuf->remoteIndex)
	 && (subIndex == pSdoBuf->remoteSubIndex))  {
		CO_DEBUG1("coSdoClientWriteInd: state: %d\n", pSdoBuf->state);

		/* save size for use inside indication function */
		sdoReadSize = size;

		if (pSdoBuf->state == CO_SDO_QUEUE_STATE_TRANSMITTING)  {
			/* only for read access to local objects */
			if ((result == 0u)
			 && (pSdoBuf->write == CO_FALSE)
			 && (pSdoBuf->localIndex != 0u))  {
				/* for coPutObj we need type of data for signed or unsigned */
				(void)coOdGetObjDescPtr(
					pSdoBuf->localIndex, pSdoBuf->localSubIndex, &pDescPtr);

				/* putObj() depends on data type */
				/* so the object indication is called also */
				switch(pDescPtr->dType)  {
					case CO_DTYPE_BOOL_VAR:
					case CO_DTYPE_BOOL_PTR:
					case CO_DTYPE_U8_VAR:
					case CO_DTYPE_U8_PTR:
						(void)coOdPutObj_u8(pSdoBuf->localIndex,
							pSdoBuf->localSubIndex, pSdoBuf->pData[0]);
						break;
					case CO_DTYPE_U16_VAR:
					case CO_DTYPE_U16_PTR:
						(void)coNumMemcpy(&u16, &pSdoBuf->pData[0], 2u, 1u);
						(void)coOdPutObj_u16(pSdoBuf->localIndex,
							pSdoBuf->localSubIndex, u16);
						break;
					case CO_DTYPE_U32_VAR:
					case CO_DTYPE_U32_PTR:
						(void)coNumMemcpy(&u32, &pSdoBuf->pData[0], 4u, 1u);
						(void)coOdPutObj_u32(pSdoBuf->localIndex,
							pSdoBuf->localSubIndex, u32);
						break;
					case CO_DTYPE_I8_VAR:
					case CO_DTYPE_I8_PTR:
						(void)coOdPutObj_i8(pSdoBuf->localIndex,
							pSdoBuf->localSubIndex, (INTEGER8)pSdoBuf->pData[0]);
						break;
					case CO_DTYPE_I16_VAR:
					case CO_DTYPE_I16_PTR:
						(void)coNumMemcpy(&i16, &pSdoBuf->pData[0], 2u, 1u);
						(void)coOdPutObj_i16(pSdoBuf->localIndex,
							pSdoBuf->localSubIndex, i16);
						break;
					case CO_DTYPE_I32_VAR:
					case CO_DTYPE_I32_PTR:
						(void)coNumMemcpy(&i32, &pSdoBuf->pData[0], 4u, 1u);
						(void)coOdPutObj_i32(pSdoBuf->localIndex,
							pSdoBuf->localSubIndex, i32);
						break;
					default:
						result = 0xffff000u;
				}
			}

			pSdoBuf->state = CO_SDO_QUEUE_STATE_FINISHED;

			/* call indication */
			if (pSdoBuf->pFct != NULL)  {
				pSdoBuf->pFct(pSdoBuf->pFctPara, result);
			}

			rdIdx++;
			if (rdIdx == CO_SDO_QUEUE_LEN)  {
				rdIdx = 0u;
			}
		}
	}
	CO_DEBUG2("coSdoClientQueueInd end: rd: %d wr:%d\n", rdIdx, wrIdx);

	/* start next transfer */
	startSdoTransfer(NULL);
}


#ifdef CO_SDOQUEUE_INTERNAL_ACCESS
/***************************************************************************/
/*
* \internal
*
* \brief internalAccess - internal SDO read/write access
*
* Only one access possible
* Start only event cycle
* Data are read/written there, inclusive the indication function
*
*/
static RET_T internalAccess(
		CO_SDO_QUEUE_T	*pSdoBuf
	)
{
static CO_EVENT_T	internalSdoEvent; 

	/* start event for answer - only one transfer possible */
	if (icoEventIsActive(&internalSdoEvent) == CO_TRUE)  { /*lint !e738 */
		/* Symbol 'internalSdoEvent' not explicitly initialized */

		return(RET_SERVICE_BUSY);
	}

    (void)icoEventStart(&internalSdoEvent, internalSdoFinished,
			pSdoBuf); /*lint !e960 */
	/* Derogation MisraC2004 R.16.9 function identifier used without '&'
	 * or parenthesized parameter */

	return(RET_OK);
}


static void internalSdoFinished(
		void *pData
	)
{
CO_SDO_QUEUE_T	*pSdoBuf = (CO_SDO_QUEUE_T *)pData;
CO_CONST CO_OBJECT_DESC_T *pDesc;
RET_T	retVal;
UNSIGNED32	result = 0uL;
BOOL_T	changed;

	retVal = coOdGetObjDescPtr(pSdoBuf->remoteIndex, pSdoBuf->remoteSubIndex, &pDesc);
	if (retVal != RET_OK)  {
		result = 0x06020000uL;
	} else {
		if (pSdoBuf->write == CO_TRUE)  {
			retVal = icoOdPutObj(pDesc, pSdoBuf->remoteSubIndex,
				pData, 0u, pSdoBuf->dataLen, CO_TRUE, &changed);
		} else {
			retVal = icoOdGetObj(pDesc, pSdoBuf->remoteSubIndex,
				pData, 0u, pSdoBuf->dataLen, CO_TRUE);
		}
		if (retVal != RET_OK)  {
			result = 0x06010000uL;
		}
	}

	/* call indication */
	icoSdoClientQueueInd(pSdoBuf->sdoNr,
		pSdoBuf->remoteIndex, pSdoBuf->remoteSubIndex,
		pSdoBuf->dataLen, result);
}
#endif /* CO_SDOQUEUE_INTERNAL_ACCESS */


/***************************************************************************/
/**
*
* \brief coSdoQueueGetActualSize - get byte count from last transfer
*
* This function returns the actual received byte count
* from the last sdo transfer.
*
* \return
*	received byte count
*/
UNSIGNED32 coSdoQueueGetActualSize(
		void	/* no parameter */
	)
{
	return(sdoReadSize);
}


/***************************************************************************/
/*
* \internal
*
* \brief getSdoBuf
*
*
*/
static CO_SDO_QUEUE_T *getSdoBuf(
		UNSIGNED16		idx
	)
{
	return(&sdoBuf[idx]);
}


/***************************************************************************/
/*
* \brief icoSdoClientVarInit - init sdo client variables
*
*/
void icoSdoQueueVarInit(
		void
	)
{

	{
		rdIdx = 0u;
		wrIdx = 0u;
		sdoReadSize = 0u;
	}
}


#endif /* CO_SDO_QUEUE */
