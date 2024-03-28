/*
* iso_tp.c - contains iso-tp routines
*
* Copyright (c) 2018-2022 emotas embedded communication GmbH
*-------------------------------------------------------------------------------
* $Id: iso_tp.c 41921 2022-09-01 10:39:03Z boe $
*
*
*-------------------------------------------------------------------------------
*
*
*/


/******************************************************************************/
/**
* \brief iso-tp implementation - iso 15765-2
*
* \file iso_tp.c
* contains iso-tp routines
*
* \section i Introduction
* The ISO-TP Stack of emotas is a software library
* that provides all communication services
* of the "CANopen Application Layer and Communication Profile"
* ISO 15765-2,
*
*
* The main features are:
*	- well-defined interface between driver and iso-tp stack
*	- ANSI-C conform
*	- MISRA checked
*	- easy-to-handle Application Programming Interface
*	- configurable and scalable
*
* This reference manual describes the functions
* for the API to evaluate the received data
* and to use the iso-tp services in the network.
*
* Configuration and features settings are supported
* by the graphical configuration tool ISO-TP DeviceDesigner.
*
* \section g General
* The iso-tp stack uses only data pointers from the application, so the
* application has to ensure that all the data pointers are valid and protected
* during runtime.
*
* \section u Using ISO TP in an application
* At startup, some initialization functions are necessary:
* 	- codrvHardwareInit()	- generic, CAN related hardware initialization
* 	- codrvCanInit()	- initialize CAN driver (Classical Mode)
* 	- codrvCanfdInit()	- initialize CAN driver (FD-Mode)
* 	- coCanOpenStackInit()	- initialize CANopen and ISO TP functionality
* 	- codrvTimerSetup()	- initialize hardware timer
* 	- codrvCanEnable()	- start CAN communication
*
* For the CANopen functionality,
* the central function
* coCommTask()
* has to be called in case of
*	- new CAN message was received
*	- timer period has been ellapsed.
*
* Therefore signal handlers should be used
* or a cyclic call of the function coCommTask() is necessary.
* For operating systems (like LINUX) the function
* codrvWaitForEvent()
* can be used to wait for events.
* <br>All CANopen functionality is handled inside this function.
*
* The start of CANopen services are also possible.
*
* \section c Indication functions
* Indication functions inform application about CAN and ISO-TP service events.
* <br>To receive an indication,
* the application has to register a function
* by the apropriate service register function like isoTpEventRegister_CLIENT().
* <br>Every time the event occures,
* the registered indication function is called.
*/

/* header of standard C - libraries
------------------------------------------------------------------------------*/
#include <stddef.h>
#include <string.h>

/* header of project specific types
------------------------------------------------------------------------------*/
#include <gen_define.h>

#ifdef ISOTP_SUPPORTED
#include <co_datatype.h>
#include <co_drv.h>
#include <co_timer.h>
#include <iso_tp.h>
#include "ico_cobhandler.h"
#include "ico_queue.h"
#include "ico_event.h"
#include "iiso_tp.h"

/* constant definitions
------------------------------------------------------------------------------*/
#ifndef MEMCPY
# define MEMCPY	memcpy
#endif /* MEMCPY */
#ifndef MEMSET
# define MEMSET	memset
#endif /* MEMSET */

#ifndef ISOTP_MAX_DATA_SIZE
#define ISOTP_MAX_DATA_SIZE (4u * 1024u)
#endif /* ISOTP_MAX_DATA_SIZE */

#ifndef ISOTP_MAX_BLOCK_SIZE
# define ISOTP_MAX_BLOCK_SIZE	20u
#endif /* ISOTP_MAX_BLOCK_SIZE */

#ifndef ISOTP_BLOCK_TIMEOUT
# define ISOTP_BLOCK_TIMEOUT	2u
#endif /* ISOTP_BLOCK_TIMEOUT */

#define ISOTP_FRAME_MASK			0xf0u
#define ISOTP_FRAME_SINGLE			0x00u
#define ISOTP_FRAME_FIRST			0x10u
#define ISOTP_FRAME_CONSECUTIVE		0x20u
#define ISOTP_FRAME_FLOW_CONTROL	0x30u

#define ISOTP_PRIORITY	(0x6u << 26)

/* local defined data types
------------------------------------------------------------------------------*/
typedef struct  {
	CO_EVENT_T				event;
	CO_TIMER_T				timer;
	COB_REFERENZ_T			trCob;
	COB_REFERENZ_T			recCob;
	ISOTP_SERVER_STATE_T	state;
	UNSIGNED16				expectedSize;
	UNSIGNED16				transferedSize;
#ifdef ISOTP_EVENT_SERVER_DATA_SPLIT
	UNSIGNED16				splitTransSize;
#endif /* ISOTP_EVENT_SERVER_DATA_SPLIT */
	UNSIGNED16				recMaxSize;
	UNSIGNED8				*pRecData;
	UNSIGNED8				*pData;
	UNSIGNED8				seqNbr;
	UNSIGNED8				blockCnt;
	UNSIGNED8				localAddr;
	UNSIGNED8				remoteAddr;
	UNSIGNED8				timeout;
	ISO_TP_ADDRESS_T		type;
	ISO_TP_FORMAT_T			formatType;
} ISO_TP_CONN_T;


/* list of external used functions, if not in headers
------------------------------------------------------------------------------*/

/* list of global defined functions
------------------------------------------------------------------------------*/

/* list of local defined functions
------------------------------------------------------------------------------*/
static void reqFlowControl(ISO_TP_CONN_T *pIsoTp);
static void recDataMsg(ISO_TP_CONN_T *pIsoTp,
		const CO_REC_DATA_T	*pRecData);
static void sendDataMsg(void *pData);

static void recFlowControl(ISO_TP_CONN_T *pIsoTp,
		const CO_REC_DATA_T *pRecData);
static void recFlowControlWait(void *pData);

static RET_T serverUserHandler(ISO_TP_CONN_T *pIsoTp);
static RET_T clientUserHandler(ISO_TP_CONN_T *pIsoTp, RET_T result);
static RET_T clientTxAckUserHandler(ISO_TP_CONN_T* pIsoTp,
		UNSIGNED8* pData, UNSIGNED16 size);

static ISO_TP_CONN_T *getClientChannel(UNSIGNED16	idx);
static ISO_TP_CONN_T *searchClientChannelDstAddr(UNSIGNED8 addr);
static ISO_TP_CONN_T *searchClientChannelCobRef(UNSIGNED16 cobRef);
static ISO_TP_CONN_T *getServerChannel(UNSIGNED16	idx);
static ISO_TP_CONN_T *searchServerChannelSrcAddr(UNSIGNED8 addr);

static ISO_TP_ADDRESS_T currentType = {ISOTP_ADDRESS_UNKNOWN};
static UNSIGNED32 calculateId(UNSIGNED8 destAddr, UNSIGNED8 srcAddr,
		ISO_TP_ADDRESS_T addrType);


/* external variables
------------------------------------------------------------------------------*/

/* global variables
------------------------------------------------------------------------------*/

/* local defined variables
------------------------------------------------------------------------------*/
#ifdef ISOTP_SERVER_CONNECTION_CNT
static ISO_TP_CONN_T isotpServerChan[ISOTP_SERVER_CONNECTION_CNT];
static UNSIGNED16 isotpServerChanCnt = {0u};

# ifdef ISOTP_EVENT_DYNAMIC_SERVER
static ISOTP_EVENT_SERVER_T	isotpServerTable[ISOTP_EVENT_DYNAMIC_SERVER];
static UNSIGNED16	isotpServerTableCnt = {0u};
# endif /* ISOTP_EVENT_DYNAMIC_SERVER */

# ifdef ISOTP_EVENT_DYNAMIC_SSPLIT
static ISOTP_EVENT_SERVER_SPLIT_T isotpServerSplitTable[ISOTP_EVENT_DYNAMIC_SSPLIT];
static UNSIGNED16 isotpServSplitTableCnt = {0u};
# endif /* ISOTP_EVENT_DYNAMIC_SSPLIT */
#endif /* ISOTP_SERVER_CONNECTION_CNT */


#ifdef ISOTP_CLIENT_CONNECTION_CNT
static ISO_TP_CONN_T isotpClientChan[ISOTP_CLIENT_CONNECTION_CNT];
static UNSIGNED16 isoTpClientChanCnt = {0u};

# ifdef ISOTP_EVENT_DYNAMIC_CLIENT
static ISOTP_EVENT_CLIENT_T	isotpClientTable[ISOTP_EVENT_DYNAMIC_CLIENT];
static UNSIGNED16 isotpClientTableCnt = {0u};
# endif /* ISOTP_EVENT_DYNAMIC_CLIENT */

# ifdef ISOTP_EVENT_DYNAMIC_CLIENT_TXACK
static ISOTP_EVENT_CLIENT_TXACK_T	isotpClientTxAckTable[ISOTP_EVENT_DYNAMIC_CLIENT_TXACK];
static UNSIGNED16 isotpClientTxAckTableCnt = {0u};
# endif /* ISOTP_EVENT_DYNAMIC_CLIENT_TXACK */
#endif /* ISOTP_CLIENT_CONNECTION_CNT */



/* implementation
------------------------------------------------------------------------------*/

#ifdef ISOTP_CLIENT_CONNECTION_CNT
/***************************************************************************/
/**
* \brief isoTpSendReq - send isotp request
*
* This function transmit the given data to the destAddr.
* The isotp channel has to be initialized before.
*
* Please note - the data has to be valid until transfer was finished.
*
* \return RET_T
*
*/
RET_T isoTpSendReq(
		UNSIGNED8	destAddr,		/**< destination address */
		UNSIGNED8	*pData,			/**< pointer to transmit data */
		UNSIGNED16	dataLen,		/**< length of transmit data */
		UNSIGNED8	flags			/**< flags for inhibit/indicaton */
	)
{
RET_T			retVal;
ISO_TP_CONN_T	*pIsoTp = NULL;
UNSIGNED8		trData[CO_CAN_MAX_DATA_LEN];
UNSIGNED8		extFormatOffs = 0u;

	if (pData == NULL)  {
		return(RET_HARDWARE_ERROR);
	}
	if (dataLen > ((4u * 1024u) -1u))  {
		return(RET_INVALID_PARAMETER);
	}
	pIsoTp = searchClientChannelDstAddr(destAddr);
	if (pIsoTp == NULL)  {
		return(RET_EVENT_NO_RESSOURCE);
	}

	if (pIsoTp->state != ISOTP_STATE_CLIENT_FREE)  {
		return(RET_SERVICE_BUSY);
	}  

	/* extended format needs 2 bytes */
	if (pIsoTp->formatType == ISOTP_FORMAT_EXTENDED)  {
		extFormatOffs = 1u;
	}

	if (pIsoTp->type == ISOTP_ADDRESS_FUNC)  {
		if (dataLen > (CO_CAN_MAX_DATA_LEN - (1u + extFormatOffs)))  {
			return(RET_DATA_TYPE_MISMATCH);
		}
	}

	/* fill complete buffer with 0x55 */
	memset(trData, 0x55u, CO_CAN_MAX_DATA_LEN);

	/* extended addressing - first byte is dest address */
	if (pIsoTp->formatType == ISOTP_FORMAT_EXTENDED)  {
		trData[0] = destAddr;
	}

	if (dataLen > (CO_CAN_MAX_DATA_LEN - (1u + extFormatOffs)))  {
		pIsoTp->expectedSize = dataLen;
		pIsoTp->transferedSize = 6u - extFormatOffs;
		pIsoTp->pData = pData;
		pIsoTp->seqNbr = 0u;

		trData[1u + extFormatOffs] = (UNSIGNED8)(dataLen & 0xffu);
		dataLen >>= 8u;
		trData[extFormatOffs] = (UNSIGNED8)(dataLen & 0xffu);
		trData[extFormatOffs] |= 0x10u;
		memcpy(&trData[2u + extFormatOffs], pData, 6u - extFormatOffs);

		pIsoTp->state = ISOTP_STATE_CLIENT_WAIT_CTS;
		retVal = icoTransmitMessage(pIsoTp->trCob, &trData[0], 0u);
	} else {
		/* single frame */
		trData[extFormatOffs] = (UNSIGNED8)dataLen;
		memcpy(&trData[1u + extFormatOffs], pData, dataLen);
		retVal = icoTransmitMessage(pIsoTp->trCob, &trData[0], flags);
		pIsoTp->state = ISOTP_STATE_CLIENT_FREE;
		clientUserHandler(pIsoTp, retVal);
	}

	return(retVal);
}
#endif /* ISOTP_CLIENT_CONNECTION_CNT */


/***************************************************************************/
/**
* \brief isoTpSetRecDataPtr - set ISO-TP receive pointer
*
* This function sets the adress and the size of a receive connection.<br>
* At the initialization time,
* there are no receive buffers.
* This has to be done by this function.
*
* \return RET_T
*
*/
RET_T isoTpSetRecDataPtr(
		ISO_TP_DATAPTR_T direction,			/**< direction */
		UNSIGNED8	destAddr,				/**< destination address */
		UNSIGNED8	*pRecData,				/**< pointer for receive data */
		UNSIGNED16	size					/**< max len of data */
	)
{
ISO_TP_CONN_T	*pIsoTp = NULL;

	if (size > ISOTP_MAX_DATA_SIZE)  {
		return(RET_INVALID_PARAMETER);
	}

	if (direction == ISOTP_DATAPTR_SERVER)  {
#ifdef ISOTP_SERVER_CONNECTION_CNT
		pIsoTp = searchServerChannelSrcAddr(destAddr);
	} else {
#else /* ISOTP_SERVER_CONNECTION_CNT */
		return(RET_INVALID_PARAMETER);
#endif /* ISOTP_SERVER_CONNECTION_CNT */
	}

	if (pIsoTp == NULL)  {
		return(RET_EVENT_NO_RESSOURCE);
	}
	if (size > ISOTP_MAX_DATA_SIZE)  {
		return(RET_INVALID_PARAMETER);
	}

	pIsoTp->pRecData = pRecData;
	pIsoTp->recMaxSize = size;

	return(RET_OK);
}


#ifdef ISOTP_CLIENT_CONNECTION_CNT
/******************************************************************************/
/**
*  \internal
*
* \brief iisoTpMessageHandler - handles all iso-tp messages
*
*
*/
void iisoTpClientMessageHandler(
		const CO_REC_DATA_T	*pRecData		/* pointer to received data */
	)
{
ISO_TP_CONN_T *pIsoTp;
UNSIGNED8 command;
UNSIGNED8 extFormatOffs = 0u;

	/* check max service index */
	if (pRecData->spec >= isoTpClientChanCnt)  {
		return;
	}
	pIsoTp = getClientChannel(pRecData->spec);

	/* extended format ?*/
	if (pIsoTp->formatType == ISOTP_FORMAT_EXTENDED)  {
		if (pIsoTp->localAddr != pRecData->msg.data[0])  {
			return;
		}

		extFormatOffs = 1u;
	}

	/* the command is the most left nibble */
	command = pRecData->msg.data[extFormatOffs] & ISOTP_FRAME_MASK;

	if (command == ISOTP_FRAME_FLOW_CONTROL)  {
		recFlowControl(pIsoTp, pRecData);
	}
}
#endif /* ISOTP_CLIENT_CONNECTION_CNT */


#ifdef ISOTP_SERVER_CONNECTION_CNT
/******************************************************************************/
/**
*  \internal
*
* \brief iisoTpServerMessageHandler - handles all iso-tp server messages
*
*
*/
void iisoTpServerMessageHandler(
		const CO_REC_DATA_T	*pRecData		/* pointer to received data */
	)
{
ISO_TP_CONN_T *pIsoTp;
UNSIGNED8	command;
UNSIGNED8	extFormatOffs = 0u;
UNSIGNED8	dstAddr;

	/* check max service index */
	if (pRecData->spec >= isotpServerChanCnt)  {
		return;
	}
	pIsoTp = getServerChannel(pRecData->spec);

	/* extended format ?*/
	if (pIsoTp->formatType == ISOTP_FORMAT_EXTENDED)  {
		if (pRecData->msg.data[0u] != pIsoTp->localAddr)  {
			return;
		}

		extFormatOffs = 1u;
	}

	/* normal fixed ? */
	if (pIsoTp->formatType == ISOTP_FORMAT_NORMAL_FIXED)  {
		/* check local address */
		dstAddr = (pRecData->msg.canId >> 8) & 0xffu;
		if ((dstAddr != pIsoTp->localAddr) && (dstAddr != 0xff))  {
			return;
		}
		pIsoTp->remoteAddr = pRecData->msg.canId & 0xffu;
	}


	/* the command is the most left nibble */
	command = pRecData->msg.data[extFormatOffs] & ISOTP_FRAME_MASK;

	switch (command)  {
	case ISOTP_FRAME_SINGLE:
		if (pIsoTp->pRecData == NULL)  {
			return;
		}
		if (pRecData->msg.data[extFormatOffs] > (7u - extFormatOffs))  {
			return;
		}
		if (pIsoTp->recMaxSize >= pRecData->msg.data[extFormatOffs])  {
			memcpy(pIsoTp->pRecData, &(pRecData->msg.data[1u + extFormatOffs]),
				pRecData->msg.data[extFormatOffs]);
			pIsoTp->transferedSize = pRecData->msg.data[extFormatOffs];
			pIsoTp->expectedSize = pRecData->msg.data[extFormatOffs];
			/* call user indication */
			serverUserHandler(pIsoTp);
		}
		break;

	case ISOTP_FRAME_FIRST:
		if (pIsoTp->type == ISOTP_ADDRESS_FUNC)  {
			return;
		}
		memcpy(pIsoTp->pRecData, &(pRecData->msg.data[2u + extFormatOffs]), 6u - extFormatOffs);
		pIsoTp->transferedSize = 6u - extFormatOffs;

		pIsoTp->expectedSize = pRecData->msg.data[extFormatOffs] & 0x0fu;
		pIsoTp->expectedSize = pIsoTp->expectedSize << 8u;
		pIsoTp->expectedSize |= pRecData->msg.data[extFormatOffs + 1u];

		if (pIsoTp->expectedSize > pIsoTp->recMaxSize)  {
			/* ignore in case we cannot handle the size */
			return;
		}

		pIsoTp->seqNbr = 1u;
		pIsoTp->blockCnt = 0u;

#ifdef ISOTP_EVENT_SERVER_DATA_SPLIT
		pIsoTp->splitTransSize = 6u;
#endif /* ISOTP_EVENT_SERVER_DATA_SPLIT */

		pIsoTp->state = ISOTP_STATE_SERVER_WAIT_DATA;

		/* call flow control request */
		reqFlowControl(pIsoTp);
		break;

	case ISOTP_FRAME_CONSECUTIVE:
		recDataMsg(pIsoTp, pRecData);
		break;

	case ISOTP_FRAME_FLOW_CONTROL:
		recFlowControl(pIsoTp, pRecData);
		break;

	default:
		break;
	}
}
#endif /* ISOTP_SERVER_CONNECTION_CNT */


#ifdef ISOTP_CLIENT_CONNECTION_CNT
/******************************************************************************/
RET_T iisoTpClientTxAck(
		COB_REFERENZ_T cobRef,
		CO_CAN_TR_MSG_T* pMsg
	)
{
ISO_TP_CONN_T* pIsoTp = NULL;
UNSIGNED8	extFormatOffs = 0u;

	pIsoTp = searchClientChannelCobRef(cobRef);
	if (pIsoTp == NULL) {
		return(RET_EVENT_NO_RESSOURCE);
	}

	/* extended format ?*/
	if (pIsoTp->formatType == ISOTP_FORMAT_EXTENDED)  {
		extFormatOffs = 1u;
	}

	if (pMsg->data[extFormatOffs] > 7u)  {
		return(RET_DATA_TYPE_MISMATCH);
	}

	(void)clientTxAckUserHandler(pIsoTp, &pMsg->data[1], pMsg->data[0]);

	return(RET_OK);
}
#endif /* ISOTP_CLIENT_CONNECTION_CNT */


/******************************************************************************/
/**
*  \internal
*
* \brief recDataMsg - handles all received data messages
*
*
*/
static void recDataMsg(
		ISO_TP_CONN_T *pIsoTp,
		const CO_REC_DATA_T	*pRecData
	)
{
#ifdef ISOTP_EVENT_SERVER_DATA_SPLIT
UNSIGNED16 splitCnt;
#endif /* ISOTP_EVENT_SERVER_DATA_SPLIT */
UNSIGNED8	extFormatOffs = 0u;

	if (pIsoTp->state != ISOTP_STATE_SERVER_WAIT_DATA)  {
		/* TODO abort */
		return;
	}

	/* extended format ?*/
	if (pIsoTp->formatType == ISOTP_FORMAT_EXTENDED)  {
		extFormatOffs = 1u;
	}

	if ((pRecData->msg.data[extFormatOffs] & 0x0fu) != pIsoTp->seqNbr)  {
		pIsoTp->state = ISOTP_STATE_SERVER_FREE;
		return;
	}

	pIsoTp->seqNbr++;
	pIsoTp->seqNbr &= 0x0fu;

	pIsoTp->blockCnt++;

#ifdef ISOTP_EVENT_SERVER_DATA_SPLIT
	if ((pIsoTp->splitTransSize + (7u + extFormatOffs)) >= ISOTP_SERVER_DATA_SPLIT_CNT)  {

		splitCnt = (pIsoTp->splitTransSize + (7u + extFormatOffs)) - ISOTP_SERVER_DATA_SPLIT_CNT;
		splitCnt = (CO_CAN_MAX_DATA_LEN - (1u + extFormatOffs)) - splitCnt;

		memcpy(&pIsoTp->pRecData[pIsoTp->splitTransSize], &pRecData->msg.data[1u + extFormatOffs], splitCnt);
		pIsoTp->splitTransSize += splitCnt;
		pIsoTp->transferedSize += splitCnt;
		(void)serverUserHandler(pIsoTp);
		pIsoTp->splitTransSize = 0u;

		splitCnt = (CO_CAN_MAX_DATA_LEN - (1u + extFormatOffs)) - splitCnt;
		memcpy(&pIsoTp->pRecData[pIsoTp->splitTransSize], &pRecData->msg.data[CO_CAN_MAX_DATA_LEN - splitCnt], splitCnt);
		pIsoTp->splitTransSize += splitCnt;
		pIsoTp->transferedSize += splitCnt;

	} else {
		memcpy(&pIsoTp->pRecData[pIsoTp->splitTransSize], &pRecData->msg.data[1u], CO_CAN_MAX_DATA_LEN - 1u);
		pIsoTp->transferedSize += (CO_CAN_MAX_DATA_LEN - 1u);
		pIsoTp->splitTransSize += (CO_CAN_MAX_DATA_LEN - 1u);
	}

	if (pIsoTp->transferedSize >= pIsoTp->expectedSize) {
		pIsoTp->splitTransSize = pIsoTp->expectedSize % ISOTP_SERVER_DATA_SPLIT_CNT;
	}
#else /* ISOTP_EVENT_SERVER_DATA_SPLIT */

	memcpy(&pIsoTp->pRecData[pIsoTp->transferedSize], &pRecData->msg.data[1u + extFormatOffs], CO_CAN_MAX_DATA_LEN - (1u + extFormatOffs));
	pIsoTp->transferedSize += (CO_CAN_MAX_DATA_LEN - (1u + extFormatOffs));
#endif /* ISOTP_EVENT_SERVER_DATA_SPLIT */

	if (pIsoTp->transferedSize >= pIsoTp->expectedSize)  {

		pIsoTp->state = ISOTP_STATE_SERVER_FREE;

		pIsoTp->transferedSize = pIsoTp->expectedSize;
		/* call user indication */
		(void)serverUserHandler(pIsoTp);
		return;
	}
	if (pIsoTp->blockCnt >= ISOTP_MAX_BLOCK_SIZE)  {
		reqFlowControl(pIsoTp);
		pIsoTp->blockCnt = 0u;
	}
}


/******************************************************************************/
/**
*  \internal
*
* \brief reqFlowControl - sends flow control message
*
*
*/
static void reqFlowControl(
		ISO_TP_CONN_T *pIsoTp
	)
{
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];
UNSIGNED8	extFormatOffs = 0u;

	memset(trData, 0x55u, CO_CAN_MAX_DATA_LEN);

	/* extended format ?*/
	if (pIsoTp->formatType == ISOTP_FORMAT_EXTENDED)  {
		trData[0u] = pIsoTp->remoteAddr;
		extFormatOffs = 1u;
	}

	trData[extFormatOffs] = 0x30u;
	if (pIsoTp->expectedSize > pIsoTp->recMaxSize)  {
		trData[extFormatOffs] |= (UNSIGNED8)2u; 
	}

	trData[extFormatOffs + 1u] = ISOTP_MAX_BLOCK_SIZE;
	trData[extFormatOffs + 2u] = ISOTP_BLOCK_TIMEOUT;

	(void)icoTransmitMessage(pIsoTp->trCob, &trData[0], 0u);
}


/******************************************************************************/
/**
*  \internal
*
* \brief recFlowControl - handles received flow control
*
*
*/
static void recFlowControl(
		ISO_TP_CONN_T	*pIsoTp,
		const CO_REC_DATA_T	*pRecData
	)
{
UNSIGNED8 command;
UNSIGNED8 extFormatOffs = 0u;

	/* extended format ?*/
	if (pIsoTp->formatType == ISOTP_FORMAT_EXTENDED)  {
		extFormatOffs = 1u;
	}

	pIsoTp->timeout = pRecData->msg.data[2u + extFormatOffs];
	pIsoTp->blockCnt = pRecData->msg.data[1u + extFormatOffs];

	if (pIsoTp->blockCnt == 0u)  {
		pIsoTp->blockCnt = 0xffu;
	}

	command = pRecData->msg.data[extFormatOffs];
	command = command & 0x0fu;

	switch(command)  {
	case 0x00u: /* Clear To Send */
		sendDataMsg(pIsoTp);
		if (pIsoTp->timeout > 0u)  {
			coTimerStart(&pIsoTp->timer,
					(UNSIGNED32)pIsoTp->timeout * 1000ul,
					sendDataMsg, (void *)pIsoTp, CO_TIMER_ATTR_ROUNDUP_CYCLIC);
		}
		break;
	case 0x01u: /* wait for next frame */
		/* we are supposed to wait, so we wait one second */
		coTimerStart(&pIsoTp->timer, 1000ul * 1000ul,
				recFlowControlWait, (void *)pIsoTp, CO_TIMER_ATTR_ROUNDDOWN);
		break;
	/* case 0x02: *//* overflow at receiver side */
	default:
		break;
	}
}


/******************************************************************************/
/**
*  \internal
*
* \brief recFlowControlWait - timer callback for waiting after flow control
*
*
*/
static void recFlowControlWait(
		void *pData
	)
{
ISO_TP_CONN_T	*pIsoTp = pData;

	sendDataMsg(pIsoTp);
	coTimerStart(&pIsoTp->timer, (UNSIGNED32)pIsoTp->timeout * 1000ul,
			sendDataMsg, pIsoTp, CO_TIMER_ATTR_ROUNDUP_CYCLIC);
}


/******************************************************************************/
/**
*  \internal
*
* \brief sendDataMsg - sends data message
*
*
*/
static void sendDataMsg(
		void	*pData
	)
{
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];
UNSIGNED16	size;
ISO_TP_CONN_T	*pIsoTp = (ISO_TP_CONN_T *)pData;
UNSIGNED8	extFormatOffs = 0u;

	/* extended format ?*/
	if (pIsoTp->formatType == ISOTP_FORMAT_EXTENDED)  {
		trData[0] = pIsoTp->remoteAddr;
		extFormatOffs = 1u;
	}

	size = pIsoTp->expectedSize - pIsoTp->transferedSize;
	if (size >= (7u - extFormatOffs))  {
		size = 7u - extFormatOffs;
	}

	if ((size == 0u) || (pIsoTp->blockCnt == 0u))  {
		/* nothing to send anymore */
		pIsoTp->state = ISOTP_STATE_CLIENT_FREE;
		coTimerStop(&pIsoTp->timer);
		clientUserHandler(pIsoTp, RET_OK);
		return;
	}
	memset(&trData[extFormatOffs], 0x55u, CO_CAN_MAX_DATA_LEN - extFormatOffs);

	pIsoTp->seqNbr++;
	pIsoTp->seqNbr &= 0x0fu;

	trData[extFormatOffs] = 0x20u;
	trData[extFormatOffs] |= pIsoTp->seqNbr;

	memcpy(&trData[1u + extFormatOffs], &(pIsoTp->pData[pIsoTp->transferedSize]), size);
	pIsoTp->transferedSize += size;
	pIsoTp->blockCnt--;

	(void)icoTransmitMessage(pIsoTp->trCob, &trData[0], 0u);

	if (pIsoTp->timeout == 0u)  {
		icoEventStart(&pIsoTp->event, sendDataMsg, pIsoTp);
	}

}


/******************************************************************************/
/**
* \internal
*
* \brief serverUserHandler - call iso-tp server user indication
*
* \return none
*
*/
static RET_T serverUserHandler(
		ISO_TP_CONN_T	*pIsoTp
	)
{
UNSIGNED16 cnt;

	/* save connection type for callback */
	currentType = pIsoTp->type;

#ifdef ISOTP_EVENT_SERVER_DATA_SPLIT
	cnt = isotpServSplitTableCnt;
	while(cnt--)  {
		isotpServerSplitTable[cnt](pIsoTp->localAddr,
			&pIsoTp->pRecData[0u],
			pIsoTp->splitTransSize, pIsoTp->transferedSize);
	}
#endif /* ISOTP_EVENT_SERVER_DATA_SPLIT */


#ifdef ISOTP_EVENT_DYNAMIC_SERVER
	if (pIsoTp->transferedSize == pIsoTp->expectedSize) {
		cnt = isotpServerTableCnt;
		while (cnt--) {
			isotpServerTable[cnt](pIsoTp->remoteAddr,
				&pIsoTp->pRecData[0u], pIsoTp->transferedSize);
		}
	}
#endif /* ISOTP_EVENT_DYNAMIC_SERVER */

	currentType = ISOTP_ADDRESS_UNKNOWN;

	(void)pIsoTp;
	(void)cnt;

	return(RET_OK);
}


/******************************************************************************/
/**
* \internal
*
* \brief clientUserHandler - call iso-tp client user indication
*
* \return none
*
*/
static RET_T clientUserHandler(
		ISO_TP_CONN_T	*pIsoTp,
		RET_T			result
	)
{
#ifdef ISOTP_EVENT_DYNAMIC_CLIENT
UNSIGNED16 cnt;

	cnt = isotpClientTableCnt;
	while(cnt--)  {
		isotpClientTable[cnt](pIsoTp->localAddr, &pIsoTp->pRecData[0u], result);
	}
#else /* ISOTP_EVENT_DYNAMIC_CLIENT */
	(void)pIsoTp;
	(void)result;
#endif /* ISOTP_EVENT_DYNAMIC_CLIENT */

	return(RET_OK);
}


/******************************************************************************/
/**
* \internal
*
* \brief clientTxAckUserHandler - call iso-tp client user indication 
*
* \return none
*
*/
static RET_T clientTxAckUserHandler(
		ISO_TP_CONN_T* pIsoTp,
		UNSIGNED8 *pData,
		UNSIGNED16 size
	)
{
#ifdef ISOTP_EVENT_DYNAMIC_CLIENT_TXACK
UNSIGNED16 cnt;

	cnt = isotpClientTxAckTableCnt;
	while (cnt--) {
		isotpClientTxAckTable[cnt](pIsoTp->localAddr, pData, size);
	}
#else /* ISOTP_EVENT_DYNAMIC_CLIENT_TXACK */
	(void)pIsoTp;
#endif /* ISOTP_EVENT_DYNAMIC_CLIENT_TXACK */

	return(RET_OK);
}


/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
#ifdef ISOTP_EVENT_DYNAMIC_SERVER
/******************************************************************************/
/**
* \brief isoTpEventRegister_SERVER - register iso-tp server event function
*
* This function registers an iso-tp write indication function.
*
* \return RET_T
*
*/
RET_T isoTpEventRegister_SERVER(
		ISOTP_EVENT_SERVER_T	pFunction	/**< pointer to function */
	)
{
	if (isotpServerTableCnt >= ISOTP_EVENT_DYNAMIC_SERVER) {
		return(RET_EVENT_NO_RESSOURCE);
	}
	/* save function pointer */
	isotpServerTable[isotpServerTableCnt] = pFunction;
	isotpServerTableCnt++;

	return(RET_OK);
}
#endif /* ISOTP_EVENT_DYNAMIC_SERVER */


#ifdef ISOTP_EVENT_DYNAMIC_SSPLIT
/******************************************************************************/
/**
* \brief isoTpEventRegister_SERVER - register iso-tp server event function
*
* This function registers an iso-tp write indication function.
*
* \return RET_T
*
*/
RET_T isoTpEventRegister_SERVER_SPLIT(
		ISOTP_EVENT_SERVER_SPLIT_T	pFunction	/**< pointer to function */
	)
{
	if (isotpServSplitTableCnt >= ISOTP_EVENT_DYNAMIC_SSPLIT) {
		return(RET_EVENT_NO_RESSOURCE);
	}
	/* save function pointer */
	isotpServerSplitTable[isotpServSplitTableCnt] = pFunction;
	isotpServSplitTableCnt++;

	return(RET_OK);
}
#endif /* ISOTP_EVENT_DYNAMIC_SSPLIT */


#ifdef ISOTP_EVENT_DYNAMIC_CLIENT
/******************************************************************************/
/**
* \brief isoTpEventRegister_CLIENT - register iso-tp client event function
*
* This function registers an iso-tp client indication function.
*
* \return RET_T
*
*/
RET_T isoTpEventRegister_CLIENT(
		ISOTP_EVENT_CLIENT_T	pFunction	/**< pointer to function */
	)
{
	if (isotpClientTableCnt >= ISOTP_EVENT_DYNAMIC_CLIENT) {
		return(RET_EVENT_NO_RESSOURCE);
	}
	/* save function pointer */
	isotpClientTable[isotpClientTableCnt] = pFunction;
	isotpClientTableCnt++;

	return(RET_OK);
}
#endif /* ISOTP_EVENT_DYNAMIC_CLIENT */


#ifdef ISOTP_EVENT_DYNAMIC_CLIENT_TXACK
/******************************************************************************/
/**
* \brief isoTpEventRegister_CLIENT - register iso-tp client event function
*
* This function registers an iso-tp client indication function.
*
* \return RET_T
*
*/
RET_T isoTpEventRegister_CLIENT_TXACK(
		ISOTP_EVENT_CLIENT_TXACK_T	pFunction	/**< pointer to function */
	)
{
	if (isotpClientTxAckTableCnt >= ISOTP_EVENT_DYNAMIC_CLIENT_TXACK) {
		return(RET_EVENT_NO_RESSOURCE);
	}
	/* save function pointer */
	isotpClientTxAckTable[isotpClientTxAckTableCnt] = pFunction;
	isotpClientTxAckTableCnt++;

	return(RET_OK);
}
#endif /* ISOTP_EVENT_DYNAMIC_CLIENT_TXACK */


#ifdef ISOTP_CLIENT_CONNECTION_CNT
/******************************************************************************/
/**
* \brief isoTpInitClientChannel - initialize iso tp client channel
*
* This function initialize an iso tp client channel.
* It can be used for transmit data.
*
* \return RET_T
*
*/
RET_T isoTpInitClientChannel(
		UNSIGNED32 reqId,				/**< request CAN id */
		UNSIGNED32 respId,				/**< response CAN id */
		UNSIGNED8 localAddr,				/**< source address */
		UNSIGNED8 destAddr,				/**< destination address */
		ISO_TP_ADDRESS_T addrType,		/**< address type */
		ISO_TP_FORMAT_T formatType,		/**< format type */
		UNSIGNED8 *pRecDataBuffer,		/**< receive data buffer */		
		UNSIGNED16 recBufSize			/**< receive data buffer size */
	)
{
ISO_TP_CONN_T *pIsoTp;

	if (isoTpClientChanCnt >= ISOTP_CLIENT_CONNECTION_CNT)  {
		return(RET_EVENT_NO_RESSOURCE);
	}

	pIsoTp = getClientChannel(isoTpClientChanCnt);

	switch (formatType)  {
	case ISOTP_FORMAT_NORMAL:		/* normal adressing */
		break;
	case ISOTP_FORMAT_NORMAL_FIXED:	/* normal fixed addressing */
		break;
	case ISOTP_FORMAT_EXTENDED:		/* extended addressing */
		break;
	case ISOTP_FORMAT_MIXED:		/* mixed addressing */
		/* no break - currently not provided */
	default:
		return(RET_INVALID_PARAMETER);
	}

	pIsoTp->recCob = icoCobCreate(CO_COB_TYPE_RECEIVE, CO_SERVICE_ISOTP_CLIENT, isoTpClientChanCnt);
	if (pIsoTp->recCob == 0xffffu)  {
		return(RET_NO_COB_AVAILABLE);
	}

	pIsoTp->trCob = icoCobCreate(CO_COB_TYPE_TRANSMIT, CO_SERVICE_ISOTP_CLIENT, isoTpClientChanCnt);
	if (pIsoTp->trCob == 0xffffu)  {
		return(RET_NO_COB_AVAILABLE);
	}

	/* for ISOTP_FORMAT_NORMAL use respId and reqId */
	if (formatType == ISOTP_FORMAT_NORMAL_FIXED)  {
		respId = calculateId(localAddr, destAddr, addrType);
		reqId = calculateId(destAddr, localAddr, addrType);
	}

	(void)icoCobSet(pIsoTp->recCob, respId, CO_COB_RTR_NONE, 8u);
	(void)icoCobSet(pIsoTp->trCob, reqId, CO_COB_RTR_NONE, 8u);

	//pIsoTp->localAddr = destAddr;
	pIsoTp->localAddr = localAddr;
	pIsoTp->remoteAddr = destAddr;
	pIsoTp->type = addrType;
	pIsoTp->formatType = formatType;

	pIsoTp->pRecData = pRecDataBuffer;
	pIsoTp->recMaxSize = recBufSize;

	isoTpClientChanCnt++;

	(void)localAddr;

	return(RET_OK);
}
#endif /* ISOTP_CLIENT_CONNECTION_CNT */


/******************************************************************************/
/**
* \brief isoTpSetIds - change request and response CAN-IDs
*
* This function changes the isotp client and server IDs
* for request and response
* depending on the given destination address.
*
* If a transfer is in progess or remoteAddr wasn't found,
* the function returns an error.
*
* This function can only be used for normal fixed address mode.
*
* \return RET_T
*
*/
RET_T isoTpSetIds(
		UNSIGNED8 remoteAddr,			/**< remote address */
		UNSIGNED32 reqId,				/**< request CAN id */
		UNSIGNED32 respId				/**< response CAN id */
	)
{
ISO_TP_CONN_T *pIsoTp;
RET_T	retVal;
UNSIGNED8	localAddr;

	/* get client channel */
	pIsoTp = searchClientChannelDstAddr(remoteAddr);
	if (pIsoTp == NULL)  {
		return(RET_INVALID_PARAMETER);
	}

	/* if transfer in progress ? */
	if (pIsoTp->state != ISOTP_STATE_CLIENT_FREE)  {
		return(RET_SERVICE_BUSY);
	}

	/* only ISOTP_FORMAT_NORMAL and ISOTP_FORMAT_EXTENDED use respId and reqId*/
	if ((pIsoTp->formatType != ISOTP_FORMAT_NORMAL)
	 && (pIsoTp->formatType != ISOTP_FORMAT_EXTENDED)) {
		return(RET_INVALID_PARAMETER);
	}

	retVal = icoCobSet(pIsoTp->recCob, reqId, CO_COB_RTR_NONE, 8u);
	if (retVal != RET_OK)  {
		return(retVal);
	}
	retVal = icoCobSet(pIsoTp->trCob, respId, CO_COB_RTR_NONE, 8u);
	if (retVal != RET_OK)  {
		return(retVal);
	}

	localAddr = pIsoTp->localAddr;

	/* get server channel */
	pIsoTp = searchServerChannelSrcAddr(localAddr);
	if (pIsoTp == NULL)  {
		return(RET_INVALID_PARAMETER);
	}

	/* if transfer in progress ? */
	if (pIsoTp->state != ISOTP_STATE_SERVER_FREE)  {
		return(RET_SERVICE_BUSY);
	}

	/* only ISOTP_FORMAT_NORMAL use respId and reqId */
	if (pIsoTp->formatType != ISOTP_FORMAT_NORMAL)  {
		return(RET_INVALID_PARAMETER);
	}

	retVal = icoCobSet(pIsoTp->recCob, reqId, CO_COB_RTR_NONE, 8u);
	if (retVal != RET_OK)  {
		return(retVal);
	}
	retVal = icoCobSet(pIsoTp->trCob, respId, CO_COB_RTR_NONE, 8u);
	if (retVal != RET_OK)  {
		return(retVal);
	}
	return(RET_OK);
}


/******************************************************************************/
/*
* \brief calculateId - calculate can-ids
*
*/
static UNSIGNED32 calculateId(
		UNSIGNED8 destAddr,				/* destination address */
		UNSIGNED8 srcAddr,				/* source address */
		ISO_TP_ADDRESS_T addrType		/* address type */
	)
{
UNSIGNED32	id;

	id = CO_COB_29BIT | ISOTP_PRIORITY | (destAddr << 8) | srcAddr;
	if (addrType == ISOTP_ADDRESS_PHYS)  {
		id  |= 0xDA0000u;
	} else
	if (addrType == ISOTP_ADDRESS_FUNC)  {
		id  |= 0xDB0000u;
	} else {
		id = 0u;
	}

	return(id);
}


#ifdef ISOTP_SERVER_CONNECTION_CNT
/******************************************************************************/
/**
* \brief isoTpInitServerChannel - initialize iso tp server channel
*
* This function initialize an iso tp server channel.
* It can be used for transmit and for receive of data.
*
* \return RET_T
*
*/
RET_T isoTpInitServerChannel(
		UNSIGNED32 reqId,				/**< request CAN id */
		UNSIGNED32 respId,				/**< response CAN id */
		UNSIGNED8 localAddr,				/**< source address */
		UNSIGNED8 destAddr,				/**< destination address */
		ISO_TP_ADDRESS_T addrType,		/**< address type */
		ISO_TP_FORMAT_T formatType,		/**< format type */
		UNSIGNED8 *pRecDataBuffer,		/**< receive data buffer */		
		UNSIGNED16 recBufSize			/**< receive data buffer size */
	)
{
ISO_TP_CONN_T *pIsoTp;

	if (isotpServerChanCnt >= ISOTP_SERVER_CONNECTION_CNT)  {
		return(RET_EVENT_NO_RESSOURCE);
	}

	pIsoTp = getServerChannel(isotpServerChanCnt);

	switch (formatType)  {
	case ISOTP_FORMAT_NORMAL:
		break;
	case ISOTP_FORMAT_NORMAL_FIXED:	/**< normal fixed addressing */
		break;
	case ISOTP_FORMAT_EXTENDED:		/**< extended addressing */
		break;
	case ISOTP_FORMAT_MIXED:		/**< mixed addressing */
		/* no break - currently not supported */
	default:
		return(RET_INVALID_PARAMETER);
	}

	pIsoTp->recCob = icoCobCreate(CO_COB_TYPE_RECEIVE,
			CO_SERVICE_ISOTP_SERVER, isotpServerChanCnt);
	if (pIsoTp->recCob == 0xffffu)  {
		return(RET_NO_COB_AVAILABLE);
	}

	pIsoTp->trCob = icoCobCreate(CO_COB_TYPE_TRANSMIT,
			CO_SERVICE_ISOTP_SERVER, isotpServerChanCnt);
	if (pIsoTp->trCob == 0xffffu)  {
		return(RET_NO_COB_AVAILABLE);
	}

	/* for ISOTP_FORMAT_NORMAL use respId and reqId */
	if (formatType == ISOTP_FORMAT_NORMAL_FIXED)  {
		respId = calculateId(destAddr, localAddr, addrType);
		reqId  = calculateId(localAddr, destAddr, addrType);
	}

	(void)icoCobSet(pIsoTp->recCob, reqId, CO_COB_RTR_NONE, 8u);
	(void)icoCobSet(pIsoTp->trCob, respId, CO_COB_RTR_NONE, 8u);

	if (formatType == ISOTP_FORMAT_NORMAL_FIXED)  {
		(void)icoCobSetIgnore(pIsoTp->recCob, 0xffffu);
	}

	pIsoTp->localAddr = localAddr;
	pIsoTp->remoteAddr = destAddr;
	pIsoTp->type = addrType;
	pIsoTp->formatType = formatType;

	pIsoTp->pRecData = pRecDataBuffer;
	pIsoTp->recMaxSize = recBufSize;

	isotpServerChanCnt++;

	return(RET_OK);
}
#endif /* ISOTP_SERVER_CONNECTION_CNT */


/******************************************************************************/
/**
* \brief isoTpSetClientAddr - change client address
*
* This function changes the isotp client address
* for the given destination channel.
*
* If a transfer is in progess or clientAddr wasn't found,
* the function returns an error.
*
* If newLocalAddr or newRemoteAddr is zero, the address will not be changed.
*
* \return RET_T
*
*/
RET_T isoTpSetClientAddr(
		UNSIGNED8 remoteAddr,				/**< destination iso-tp address */
		UNSIGNED8 newLocalAddr,				/**< new local iso-tp address */
		UNSIGNED8 newRemoteAddr				/**< new remote address */
	)
{
ISO_TP_CONN_T *pIsoTp;
RET_T	retVal;
UNSIGNED32	canId;

	/* get client channel */
	pIsoTp = searchClientChannelDstAddr(remoteAddr);
	if (pIsoTp == NULL)  {
		return(RET_INVALID_PARAMETER);
	}

	/* if transfer in progress ? */
	if (pIsoTp->state != ISOTP_STATE_CLIENT_FREE)  {
		return(RET_SERVICE_BUSY);
	}

	/* only ISOTP_FORMAT_NORMAL_FIXED uses dst and src address */
	if ((pIsoTp->formatType != ISOTP_FORMAT_NORMAL_FIXED)
	 && (pIsoTp->formatType != ISOTP_FORMAT_EXTENDED))  {
		return(RET_INVALID_PARAMETER);
	}

	if (newRemoteAddr != 0u)  {
		pIsoTp->remoteAddr = newRemoteAddr;
	}
	if (newLocalAddr != 0u)  {
		pIsoTp->localAddr = newLocalAddr;
	}

	if (pIsoTp->formatType == ISOTP_FORMAT_NORMAL_FIXED)  {
		canId = calculateId(pIsoTp->localAddr, pIsoTp->remoteAddr, pIsoTp->type);
		retVal = icoCobSet(pIsoTp->recCob, canId, CO_COB_RTR_NONE, 8u);
		if (retVal != RET_OK)  {
			return(retVal);
		}

		canId = calculateId(pIsoTp->remoteAddr, pIsoTp->localAddr, pIsoTp->type);
		retVal = icoCobSet(pIsoTp->trCob, canId, CO_COB_RTR_NONE, 8u);
		if (retVal != RET_OK)  {
			return(retVal);
		}
	}

	return(RET_OK);
}


/******************************************************************************/
/**
* \brief isoTpSetServerAddr - change server address
*
* This function changes the isotp server address
* (local and remote)
* for the given server channel.
*
* If a transfer is in progess or localAddr wasn't found,
* the function returns an error.
*
* If newLocalAddr or newRemoteAddr is zero, the address will not be changed.
*
* \return RET_T
*
*/
RET_T isoTpSetServerAddr(
		UNSIGNED8 localAddr,				/**< local iso-tp address */
		UNSIGNED8 newLocalAddr,				/**< new local iso-tp address */
		UNSIGNED8 newRemoteAddr				/**< new remote address */
	)
{
ISO_TP_CONN_T *pIsoTp;
RET_T	retVal;
UNSIGNED32	canId;

	/* get server channel */
	pIsoTp = searchServerChannelSrcAddr(localAddr);
	if (pIsoTp == NULL)  {
		return(RET_INVALID_PARAMETER);
	}

	/* if transfer in progress ? */
	if (pIsoTp->state != ISOTP_STATE_SERVER_FREE)  {
		return(RET_SERVICE_BUSY);
	}

	/* only ISOTP_FORMAT_NORMAL use respId and reqId */
	if ((pIsoTp->formatType != ISOTP_FORMAT_NORMAL_FIXED)
	 && (pIsoTp->formatType != ISOTP_FORMAT_EXTENDED))  {
		return(RET_INVALID_PARAMETER);
	}

	if (newRemoteAddr != 0u)  {
		pIsoTp->remoteAddr = newRemoteAddr;
	}
	if (newLocalAddr != 0u)  {
		pIsoTp->localAddr = newLocalAddr;
	}

	if (pIsoTp->formatType == ISOTP_FORMAT_NORMAL_FIXED)  {
		canId = calculateId(pIsoTp->remoteAddr, pIsoTp->localAddr, pIsoTp->type);
		retVal = icoCobSet(pIsoTp->trCob, canId, CO_COB_RTR_NONE, 8u);
		if (retVal != RET_OK)  {
			return(retVal);
		}
		canId = calculateId(pIsoTp->localAddr, pIsoTp->remoteAddr, pIsoTp->type);
		retVal = icoCobSet(pIsoTp->recCob, canId, CO_COB_RTR_NONE, 8u);
		if (retVal != RET_OK)  {
			return(retVal);
		}
	}

	return(RET_OK);
}


/******************************************************************************/
/**
* \brief isoTpInit - init iso tp module
*
*
* \return RET_T
*
*/
RET_T isoTpInit(
		const UNSIGNED16	*pClientCnt,
		const UNSIGNED16	*pServerCnt
	)
{
UNSIGNED16 cnt;

#ifdef ISOTP_EVENT_DYNAMIC_SERVER
	isotpServerTableCnt = 0u;

	for (cnt = 0u; cnt < ISOTP_EVENT_DYNAMIC_SERVER; cnt++)  {
		isotpServerTable[cnt] = NULL;
	}
#endif /* ISOTP_EVENT_DYNAMIC_SERVER */

#ifdef ISOTP_EVENT_DYNAMIC_SSPLIT
	for (cnt = 0u; cnt < ISOTP_EVENT_DYNAMIC_SSPLIT; cnt++)  {
		isotpServerSplitTable[cnt] = NULL;
	}
	isotpServSplitTableCnt = 0u;
#endif /* ISOTP_EVENT_DYNAMIC_SSPLIT */

#ifdef ISOTP_EVENT_DYNAMIC_CLIENT
	isotpClientTableCnt = 0u;

	for (cnt = 0u; cnt < ISOTP_EVENT_DYNAMIC_CLIENT; cnt++)  {
		isotpClientTable[cnt] = NULL;
	}
#endif /* ISOTP_EVENT_DYNAMIC_CLIENT */

#ifdef ISOTP_CLIENT_CONNECTION_CNT
	for (cnt = 0u; cnt < ISOTP_CLIENT_CONNECTION_CNT; cnt++)  {
		isotpClientChan[cnt].state = ISOTP_STATE_CLIENT_FREE;
	}

		isoTpClientChanCnt = 0u;

#endif /* ISOTP_CLIENT_CONNECTION_CNT */

#ifdef ISOTP_SERVER_CONNECTION_CNT
	for (cnt = 0u; cnt < ISOTP_SERVER_CONNECTION_CNT; cnt++)  {
		isotpServerChan[cnt].state = ISOTP_STATE_SERVER_FREE;
	}


		isotpServerChanCnt = 0u;

#endif /* ISOTP_SERVER_CONNECTION_CNT */

	(void)cnt;

	return(RET_OK);
}


/******************************************************************************/
/**
*
* \brief isoTpGetCurrectConType - get current connection type
*
*
* \return RET_T
*
*/
ISO_TP_ADDRESS_T isoTpGetCurrectConType(
		void	/* no parameter */
	)
{
	return(currentType);
}


#ifdef ISOTP_CLIENT_CONNECTION_CNT
/******************************************************************************/
/**
* \internal
*
* \brief getClientChannel - get client channel
*
*
* \return RET_T
*
*/
static ISO_TP_CONN_T *getClientChannel(
		UNSIGNED16			idx
	)
{
ISO_TP_CONN_T *pIsoTp;

	pIsoTp = &isotpClientChan[idx];

	return(pIsoTp);
}


/******************************************************************************/
/**
* \internal
*
* \brief searchClientChannelDstAddr - search client channel by Dst address
*
*
* \return RET_T
*
*/
static ISO_TP_CONN_T *searchClientChannelDstAddr(
		UNSIGNED8			addr			/* remote address */
	)
{
ISO_TP_CONN_T *pIsoTp = NULL;
UNSIGNED16	cnt;

	for (cnt = 0u; cnt < isoTpClientChanCnt; cnt++)  {
		if (isotpClientChan[cnt].remoteAddr == addr)  {
			pIsoTp = &isotpClientChan[cnt];
			break;
		}
	}

	return(pIsoTp);
}


/******************************************************************************/
/**
* \internal
*
* \brief searchClientChannelCobRef - search client channel by cob reference
*
*
* \return RET_T
*
*/
static ISO_TP_CONN_T *searchClientChannelCobRef(
		UNSIGNED16			cobRef
	)
{
ISO_TP_CONN_T *pIsoTp = NULL;
UNSIGNED16	cnt;

	for (cnt = 0u; cnt < isoTpClientChanCnt; cnt++) {
		if (isotpClientChan[cnt].trCob == cobRef){
			pIsoTp = &isotpClientChan[cnt];
			break;
		}
	}

	return(pIsoTp);
}
#endif /* ISOTP_CLIENT_CONNECTION_CNT */


#ifdef ISOTP_SERVER_CONNECTION_CNT
/******************************************************************************/
/**
* \internal
*
* \brief getServerChannel - get server channel
*
*
* \return RET_T
*
*/
static ISO_TP_CONN_T *getServerChannel(
		UNSIGNED16			idx
	)
{
ISO_TP_CONN_T *pIsoTp;

	pIsoTp = &isotpServerChan[idx];

	return(pIsoTp);
}


/******************************************************************************/
/**
* \internal
*
* \brief searchServerChannelSrcAddr - search server channel by source addr
*
*
* \return RET_T
*
*/
static ISO_TP_CONN_T *searchServerChannelSrcAddr(
		UNSIGNED8			addr
	)
{
ISO_TP_CONN_T *pIsoTp = NULL;
UNSIGNED16	cnt;

	for (cnt = 0u; cnt < isotpServerChanCnt; cnt++)  {
		pIsoTp = getServerChannel(cnt);
		if (pIsoTp->localAddr == addr)  {
			return(pIsoTp);
		}
	}
	return(NULL);
}
#endif /* ISOTP_SERVER_CONNECTION_CNT */

#endif /* ISOTP_SUPPORTED */
