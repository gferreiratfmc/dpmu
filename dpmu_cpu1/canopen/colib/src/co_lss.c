/*
* co_lss.c - contains LSS slave services
*
* Copyright (c) 2012-2022 emotas embedded communication GmbH
*-------------------------------------------------------------------
* $Id: co_lss.c 41218 2022-07-12 10:35:19Z boe $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \brief LSS slave handling
*
* \file co_lss.c
* contains LSS slave services
*
*/

/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <string.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <gen_define.h>

#ifdef CO_LSS_SUPPORTED
#include <co_datatype.h>
#include <co_drv.h>
#include <co_timer.h>
#include <co_odaccess.h>
#include <co_lss.h>
#include <co_nmt.h>
#include "ico_common.h"
#include "ico_cobhandler.h"
#include "ico_queue.h"
#include "ico_nmt.h"
#include "ico_lss.h"
#include "ico_indication.h"

/* constant definitions
---------------------------------------------------------------------------*/
#ifdef CO_EVENT_DYNAMIC_LSS
# ifdef CO_EVENT_PROFILE_LSS
#  define CO_EVENT_LSS_CNT	(CO_EVENT_DYNAMIC_LSS + CO_EVENT_PROFILE_LSS)
# else /* CO_EVENT_PROFILE_LSS */
#  define CO_EVENT_LSS_CNT	(CO_EVENT_DYNAMIC_LSS)
# endif /* CO_EVENT_PROFILE_LSS */
#else /* CO_EVENT_DYNAMIC_LSS */
# ifdef CO_EVENT_PROFILE_LSS
#  define CO_EVENT_LSS_CNT	(CO_EVENT_PROFILE_LSS)
# endif /* CO_EVENT_PROFILE_LSS */
#endif /* CO_EVENT_DYNAMIC_LSS */

#if defined(CO_EVENT_STATIC_LSS) || defined(CO_EVENT_LSS_CNT)
# define CO_EVENT_LSS	1u
#endif /* defined(CO_EVENT_STATIC_LSS) || defined(CO_EVENT_LSS_CNT) */

#ifndef CO_LSS_FD_SWITCH_TIMEOUT
# define CO_LSS_FD_SWITCH_TIMEOUT 30u	/**< LSS FD switch selective timeout in ms */
#endif /* CO_LSS_FD_SWITCH_TIMEOUT */

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/
#ifdef CO_EVENT_STATIC_LSS
extern CO_CONST CO_EVENT_LSS_T coEventLssInd;
#endif /* CO_EVENT_STATIC_LSS */


/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
static void lssSlaveIdentify(void);
static void lssFastScan(const CO_CAN_REC_MSG_T *pMsg);
static void lssNodeIdReceived(const CO_CAN_REC_MSG_T *pMsg);
static void	lssSwitchGlobal(CO_LSS_STATE_T mode);
static void lssStoreReceived(const CO_CAN_REC_MSG_T *pMsg);
static void lssInquireService(const CO_CAN_REC_MSG_T *pMsg);
static void lssBitrateReceived(UNSIGNED8 tableSelector, UNSIGNED8 tableIndex);
static void	lssBitrateActivate(UNSIGNED16 switchDelay);
static void bitrateSwitchFct(void *pData);
static void lssSwitchSelective(UNSIGNED8 lssCs, const UNSIGNED8	*pData);
static void lssIdentSlave(UNSIGNED8 lssCs, const UNSIGNED8 *pData);


#ifdef CO_EVENT_LSS
static void lssEvent(CO_LSS_SERVICE_T service, UNSIGNED16 bitrate,
		UNSIGNED8 *pErrorCode, UNSIGNED8 *pErrorSpec);
#endif /* CO_EVENT_LSS */
 
/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
static COB_REFERENZ_T	lssTransCob;
static COB_REFERENZ_T	lssRecCob;
static CO_LSS_STATE_T	lssState;
static UNSIGNED8	lssPendingNodeId;
static UNSIGNED8	lssNewNodeId = { 0u };
static UNSIGNED16	lssPendingBitrate;
static CO_TIMER_T	lssBitrateTimer;
static CO_LSS_SERVICE_T	bitrateSwitch;


#ifdef CO_EVENT_LSS_CNT
static UNSIGNED16	lssEventTableCnt = 0u;
static CO_EVENT_LSS_T lssEventTable[CO_EVENT_LSS_CNT];
#endif /* CO_EVENT_LSS_CNT */

/***************************************************************************/
/**
* \internal
*
* \brief icoSynandler - handle TIME
*
*
* \return none
*
*/
void icoLssHandler(
		CO_REC_DATA_T	*pRecData	/**< pointer to received data */
	)
{
CO_CAN_REC_MSG_T* pMsg = &pRecData->msg;

	/* start service depending on lss command */
	switch (pMsg->data[0])  {
		case LSS_CS_NON_CONFIG_REMOTE_SLAVE:
			coLssNonConfigSlave(pMsg);
			break;

		case LSS_CS_FAST_SCAN:
			lssFastScan(pMsg);
			break;

		case LSS_CS_NODE_ID:
			lssNodeIdReceived(pMsg);
			break;

		case LSS_CS_BITRATE:
			lssBitrateReceived(pMsg->data[1], pMsg->data[2]);
			break;

		case LSS_CS_STORE:
			lssStoreReceived(pMsg);
			break;

		case LSS_CS_SWITCH_GLOBAL:
			lssSwitchGlobal((CO_LSS_STATE_T)pMsg->data[1]);
			break;

		case LSS_CS_INQUIRE_NODEID:
		case LSS_CS_INQUIRE_VENDOR:
		case LSS_CS_INQUIRE_PRODUCT:
		case LSS_CS_INQUIRE_REVISION:
		case LSS_CS_INQUIRE_SERIAL:
			lssInquireService(pMsg);
			break;

		case LSS_CS_ACTIVATE_BITRATE:
			lssBitrateActivate(pMsg->data[1]
				| (UNSIGNED16)((UNSIGNED16)pMsg->data[2] << 8u));
			break;

		case LSS_CS_SWITCH_SEL_VENDOR:
		case LSS_CS_SWITCH_SEL_PRODUCT:
		case LSS_CS_SWITCH_SEL_REVISION:
		case LSS_CS_SWITCH_SEL_SERIAL:
			lssSwitchSelective(pMsg->data[0], &pMsg->data[1]);
			break;


		case LSS_CS_IDENT_VENDOR:
		case LSS_CS_IDENT_PRODUCT:
		case LSS_CS_IDENT_REVISION_LOW:
		case LSS_CS_IDENT_REVISION_HIGH:
		case LSS_CS_IDENT_SERIAL_LOW:
		case LSS_CS_IDENT_SERIAL_HIGH:
			lssIdentSlave(pMsg->data[0], &pMsg->data[1]);
			break;


		default:
			/* ignore unknown commands */
			break;
	}
}


/***************************************************************************/
/**
*
* \brief coLssNonConfigSlave - request for unconfigured slaves
*
* get answer, if node-id == 255
*
* \return none
*
*/
void coLssNonConfigSlave(
		const CO_CAN_REC_MSG_T *pMsg
	)
{
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */
UNSIGNED8	nodeId;

(void)pMsg;

	/* if node-id=255, answer with non config slave */
	nodeId = coNmtGetNodeId();
	if (nodeId == 255u)  {
		trData[0] = LSS_CS_NON_CONFIG_SLAVE;
		memset(&trData[1], 0, (size_t)7u);

		/* transmit data */
		(void)icoTransmitMessage(lssTransCob, &trData[0], MSG_OVERWRITE);
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief lssSlaveIdentify - request for unconfigured slaves
*
* get answer, if node-id == 255
*
* \return none
*
*/
static void lssSlaveIdentify(
		void	/* no parameter */
	)
{
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */

	memset(&trData[0], 0, (size_t)8u);
	trData[0] = LSS_CS_IDENTITY_SLAVE;

	/* transmit data */
	(void)icoTransmitMessage(lssTransCob, &trData[0], MSG_OVERWRITE);
}


/***************************************************************************/
/**
* \internal
*
* \brief lssFastScan - fast scan indication
*
*
* \return none
*
*/
static void lssFastScan(
		const CO_CAN_REC_MSG_T *pMsg
	)
{
UNSIGNED32	idNr;
UNSIGNED8	bitCheck;
UNSIGNED8	lssSub;
UNSIGNED8	lssNext;
static UNSIGNED8	lssPos = 0u;
UNSIGNED8	nodeId;
RET_T		retVal;
UNSIGNED8	i;
UNSIGNED32	val, ex;

	/* only non configured nodes are allowed */
	nodeId = coNmtGetNodeId();
	if (nodeId != 255u)  {
		return;
	}

	/* not allowed in CONFIGURATION state */
	if (lssState != CO_LSS_STATE_WAITING)  {
		return;
	}

	idNr = pMsg->data[1];
	val = pMsg->data[2];
	idNr |= (val << 8);
	val = pMsg->data[3];
	idNr |= (val << 16);
	val = pMsg->data[4];
	idNr |= (val << 24);
	bitCheck = pMsg->data[5];
	lssSub = pMsg->data[6];
	lssNext = pMsg->data[7];

	/* reset fastscan, then LSSPos = 0 (vendor id) */
	if (bitCheck == 128u)  {
		lssPos = 0u;
		lssSlaveIdentify();
		return;
	}

	if (lssPos != lssSub)  {
		/* we are no more active */
		return;
	}

	/* get object from OD */
	retVal = coOdGetObj_u32(0x1018u, lssSub + 1u, &val);
	if (retVal != RET_OK)  {
		return;
	}
	/* mask value */
	ex = val ^ idNr;
	i = 32u;
	while (i > bitCheck)  {
		i--;
		/* bit unequal, then abort */
		if ((ex & (1ul << i)) != 0ul)  {
			return;
		}
	}

	/* value are ok, send answer */
	lssSlaveIdentify();

	lssPos = lssNext;

	if ((bitCheck == 0u) && (lssNext < lssSub))  {
		/* enter configuration mode */
		lssState = CO_LSS_STATE_CONFIGURATION;
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief lssNodeIdReceived
*
*
* \return none
*
*/
static void lssNodeIdReceived(
		const CO_CAN_REC_MSG_T *pMsg
	)
{
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */
UNSIGNED8	errCode = 0u;
UNSIGNED8	newId;

	/* only in config mode */
	if (lssState != CO_LSS_STATE_CONFIGURATION)  {
		return;
	}


	newId = pMsg->data[1u];

	if ((newId < 1u) || ((newId > 127u) && (newId != 255u)))  {
		/* invalid node-id */
		errCode = 1u;
	} else {
#ifdef CO_EVENT_LSS
		lssEvent(CO_LSS_SERVICE_NEW_NODE_ID, (UNSIGNED16)newId, &errCode, 0u);
#else /* CO_EVENT_LSS */
		errCode = 0u;
#endif /* CO_EVENT_LSS */
		if (errCode == 0u)  {
			lssPendingNodeId = newId;
			lssNewNodeId = 1u;
		}
/* printf("*** NODE ID %d received ...\n", lssPendingNodeId); */
	}

	memset(&trData[0], 0, 8u);
	trData[0] = LSS_CS_NODE_ID;
	trData[1] = errCode;
	trData[2] = 0u;

	/* transmit data */
	(void)icoTransmitMessage(lssTransCob, &trData[0], MSG_OVERWRITE);
}


/***************************************************************************/
/**
* \internal
*
* \brief lssBitrateReceived
*
*
* \return none
*
*/
static void lssBitrateReceived(
		UNSIGNED8	tableSelector,
		UNSIGNED8	tableIndex
	)
{
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */
static const UNSIGNED16 bitTable[10] = { 1000u, 800u, 500u, 250u, 125u, 0xffffu, 50u, 20u, 10u, 0u};
UNSIGNED16	bitrate = 1u;
UNSIGNED8	errCode = 0u;
UNSIGNED8	errSpec = 0u;

	/* only in config mode */
	if (lssState != CO_LSS_STATE_CONFIGURATION)  {
		return;
	}

	/* check bitrate for CiA301 */
	if (tableSelector == 0u)  {
		if (tableIndex > 9u)  {
			bitrate = 0xffffu;
			errCode = 1u;
		} else {
			bitrate = bitTable[tableIndex];
		}
	} else {
		bitrate = 0u;
	}

	if (bitrate != 0xffffu)  {
		errCode = tableSelector;
		errSpec = tableIndex;
#ifdef CO_EVENT_LSS
		lssEvent(CO_LSS_SERVICE_NEW_BITRATE, bitrate, &errCode, &errSpec);
#endif /* CO_EVENT_LSS */
	}

	if (errCode == 0u)  {
		/* set new bitrate */
		lssPendingBitrate = bitrate;
	}

	memset(&trData[0], 0, 8u);
	trData[0] = LSS_CS_BITRATE;
	trData[1] = errCode;
	trData[2] = errSpec;

	/* transmit data */
	(void)icoTransmitMessage(lssTransCob, &trData[0], MSG_OVERWRITE);
}


/***************************************************************************/
/**
* \internal
*
* \brief lssBitrateActivate
*
*
* \return none
*
*/
static void lssBitrateActivate(
		UNSIGNED16	switchDelay
	)
{
#ifdef CO_EVENT_LSS
UNSIGNED8	errCode, errSpec;
#endif /* CO_EVENT_LSS */

	/* start timer for switching */
	(void)coTimerStart(&lssBitrateTimer, switchDelay * 1000ul,
			bitrateSwitchFct, NULL, CO_TIMER_ATTR_ROUNDUP_CYCLIC); /*lint !e960 */
	/* Derogation MisraC2004 R.16.9 function identifier used without '&'
	 * or parenthesized parameter */

	bitrateSwitch = CO_LSS_SERVICE_BITRATE_OFF;

#ifdef CO_EVENT_LSS
	lssEvent(CO_LSS_SERVICE_BITRATE_OFF, 0u, &errCode, &errSpec);
#endif /* CO_EVENT_LSS */
}


/***************************************************************************/
/**
* \internal
*
* \brief bitrateSwitchFct - switch bitrate function
*
*
* \return none
*
*/
static void bitrateSwitchFct(
		void			*pData
	)
{
(void)pData;

	if (bitrateSwitch == CO_LSS_SERVICE_BITRATE_OFF)  {
#ifdef CO_EVENT_LSS
		lssEvent(CO_LSS_SERVICE_BITRATE_SET, 0u, 0u, 0u);
#endif /* CO_EVENT_LSS */
		bitrateSwitch = CO_LSS_SERVICE_BITRATE_SET;
	} else {
		if (bitrateSwitch == CO_LSS_SERVICE_BITRATE_SET)  {
#ifdef CO_EVENT_LSS
			lssEvent(CO_LSS_SERVICE_BITRATE_ACTIVE, lssPendingBitrate, 0u, 0u);
#endif /* CO_EVENT_LSS */
			(void)coTimerStop(&lssBitrateTimer);
			bitrateSwitch = CO_LSS_SERVICE_BITRATE_ACTIVE;
		}
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief lssStoreReceived
*
*
* \return none
*
*/
static void lssStoreReceived(
		const CO_CAN_REC_MSG_T *pMsg
	)
{
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */
UNSIGNED8	errCode, errSpec;
(void)pMsg;

	/* only in config mode */
	if (lssState != CO_LSS_STATE_CONFIGURATION)  {
		return;
	}


	/* if no indication is available, return service not supported */
#ifdef CO_EVENT_LSS
	errCode = 1u;
	errSpec = 0u;
	lssEvent(CO_LSS_SERVICE_STORE, (UNSIGNED16)lssPendingNodeId,
				&errCode, &errSpec);
#else /* CO_EVENT_LSS */
	errCode = 1u;
	errSpec = 0u;
#endif /* CO_EVENT_LSS */

	memset(&trData[0], 0, 8u);
	trData[0] = LSS_CS_STORE;
	trData[1] = errCode;
	trData[2] = errSpec;

	/* transmit data */
	(void)icoTransmitMessage(lssTransCob, &trData[0], MSG_OVERWRITE);
}


/***************************************************************************/
/**
* \internal
*
* \brief lssSwitchGlobal - switch mode global
*
*
* \return none
*
*/
static void	lssSwitchGlobal(
		CO_LSS_STATE_T	mode
	)
{
CO_REC_DATA_T nmtRecData;
CO_NMT_STATE_T nmtState;

	if (mode == CO_LSS_STATE_WAITING)  {
		if (lssState == CO_LSS_STATE_CONFIGURATION)  {
			lssState = CO_LSS_STATE_WAITING;
			/* valid lss node id available ? */
			if (lssNewNodeId == 1u)  {
				nmtState = coNmtGetState();
				/* if no valid active node id set, call reset comm autonom. */
				if (nmtState == CO_NMT_STATE_RESET_NODE)  {
					/* start new reset communication */
					nmtRecData.msg.canId = 0u;
					nmtRecData.msg.len = 2u;
					nmtRecData.msg.data[0] = 130u;
					nmtRecData.msg.data[1] = 0u;
					icoNmtMsgHandler(&nmtRecData);
				}

				lssNewNodeId = 0u;
			}
		}
	}
	if (mode == CO_LSS_STATE_CONFIGURATION)  {
		if (lssState == CO_LSS_STATE_WAITING)  {
			lssState = CO_LSS_STATE_CONFIGURATION;
		}
	}
# ifdef CO_LSS_447
	if (mode == CO_LSS_STATE_CONFIGURATION)  {
		if (lssState == CO_LSS_STATE_SILENT_PASSIVE)  {
			lssState = CO_LSS_STATE_CONFIGURATION;
		}
	}
	if (mode == CO_LSS_STATE_SILENT_PASSIVE)  {
		if (lssState == CO_LSS_STATE_CONFIGURATION)  {
			lssState = CO_LSS_STATE_SILENT_PASSIVE;
		}
	}
# endif /* CO_LSS_447 */
}


/***************************************************************************/
/**
* \internal
*
* \brief lssInquireService
*
*
* \return none
*
*/
static void lssInquireService(
		const CO_CAN_REC_MSG_T *pMsg
	)
{
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */
UNSIGNED32	u32;
UNSIGNED8 inqCmd;

	/* only in config mode */
	if (lssState != CO_LSS_STATE_CONFIGURATION)  {
		return;
	}

	inqCmd = pMsg->data[0u];

	memset(&trData[0], 0, CO_CAN_MAX_DATA_LEN);
	trData[0] = inqCmd;


	switch (inqCmd)  {
		case LSS_CS_INQUIRE_NODEID:
			trData[1] = coNmtGetNodeId();
			break;
		case LSS_CS_INQUIRE_VENDOR:
			(void)coOdGetObj_u32(0x1018u, 1u, &u32);
			coNumMemcpyUnpack(&trData[1], &u32, 4u, 1u, CO_ATTR_NUM);
			break;
		case LSS_CS_INQUIRE_PRODUCT:
			(void)coOdGetObj_u32(0x1018u, 2u, &u32);
			coNumMemcpyUnpack(&trData[1], &u32, 4u, 1u, CO_ATTR_NUM);
			break;
		case LSS_CS_INQUIRE_REVISION:
			(void)coOdGetObj_u32(0x1018u, 3u, &u32);
			coNumMemcpyUnpack(&trData[1], &u32, 4u, 1u, CO_ATTR_NUM);
			break;
		case LSS_CS_INQUIRE_SERIAL:
			(void)coOdGetObj_u32(0x1018u, 4u, &u32);
			coNumMemcpyUnpack(&trData[1], &u32, 4u, 1u, CO_ATTR_NUM);
			break;
		default: /* unknown command */
			return;
	}

	/* transmit data */
	(void)icoTransmitMessage(lssTransCob, &trData[0], MSG_OVERWRITE);
}




/***************************************************************************/
/**
* \internal
*
* \brief lssSwitchSelective - switch selective parameter
*
*
* \return none
*
*/
static void lssSwitchSelective(
		UNSIGNED8	lssCs,				/* command specifier */
		const UNSIGNED8	*pData			/* pointer to received data */
	)
{
static UNSIGNED32	vendor[4];
static UNSIGNED8	nextCmd = { LSS_CS_SWITCH_SEL_VENDOR };
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */
UNSIGNED8	i;
RET_T		retVal;
UNSIGNED32	val = 0u;

	/* only commands in correct order are allowed */
	if ((lssCs != nextCmd) && (lssCs != LSS_CS_SWITCH_SEL_VENDOR))  {
		nextCmd = LSS_CS_SWITCH_SEL_VENDOR;
		return;
	}

	switch (lssCs)  {
		case LSS_CS_SWITCH_SEL_VENDOR:
			(void)coNumMemcpyPack(&vendor[0], pData, 4u, 1u, 0u);
			nextCmd = LSS_CS_SWITCH_SEL_PRODUCT;
			break;

		case LSS_CS_SWITCH_SEL_PRODUCT:
			(void)coNumMemcpyPack(&vendor[1], pData, 4u, 1u, 0u);
			nextCmd = LSS_CS_SWITCH_SEL_REVISION;
			break;

		case LSS_CS_SWITCH_SEL_REVISION:
			(void)coNumMemcpyPack(&vendor[2], pData, 4u, 1u, 0u);
			nextCmd = LSS_CS_SWITCH_SEL_SERIAL;
			break;

		case LSS_CS_SWITCH_SEL_SERIAL:
			(void)coNumMemcpyPack(&vendor[3], pData, 4u, 1u, 0u);
			nextCmd = LSS_CS_SWITCH_SEL_VENDOR;

			/* now compare data with 0x1018 object */
			for (i = 0u; i < 4u; i++)  {
				/* get object from OD */
				retVal = coOdGetObj_u32(0x1018u, i + 1u, &val);
				if (retVal != RET_OK)  {
					return;
				}
				if (val != vendor[i])  {
					return;
				}
			}
			/* all parameter fit, send answer */
			memset(&trData[0], 0, 8u);
			trData[0] = LSS_CS_SWITCH_SEL_ANSWER;

			/* transmit data */
			(void) icoTransmitMessage(lssTransCob, &trData[0], MSG_OVERWRITE);

			/* enter configuration mode */
			lssState = CO_LSS_STATE_CONFIGURATION;
			break;

		default:
			nextCmd = LSS_CS_SWITCH_SEL_VENDOR;
			break;
	}
}




/***************************************************************************/
/**
* \internal
*
* \brief lssIdent - ident lss slave
*
*
* \return none
*
*/
static void lssIdentSlave(
		UNSIGNED8		lssCs,		/* command specifier */
		const UNSIGNED8	*pData		/* pointer to received data */
	)
{
static UNSIGNED32	vendor[6];
static UNSIGNED8	nextCmd = { LSS_CS_IDENT_VENDOR };
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */
UNSIGNED8	i;
RET_T		retVal;
UNSIGNED32	val = 0u;


	/* only commands in correct order are allowed */
	if ((lssCs != nextCmd) && (lssCs != LSS_CS_IDENT_VENDOR))  {
		nextCmd = LSS_CS_IDENT_VENDOR;
		return;
	}

	switch (lssCs)  {
		case LSS_CS_IDENT_VENDOR:
			(void)coNumMemcpyPack(&vendor[0], pData, 4u, 1u, 0u);
			nextCmd = LSS_CS_IDENT_PRODUCT;
			break;

		case LSS_CS_IDENT_PRODUCT:
			(void)coNumMemcpyPack(&vendor[1], pData, 4u, 1u, 0u);
			nextCmd = LSS_CS_IDENT_REVISION_LOW;
			break;

		case LSS_CS_IDENT_REVISION_LOW:
			(void)coNumMemcpyPack(&vendor[2], pData, 4u, 1u, 0u);
			nextCmd = LSS_CS_IDENT_REVISION_HIGH;
			break;

		case LSS_CS_IDENT_REVISION_HIGH:
			(void)coNumMemcpyPack(&vendor[3], pData, 4u, 1u, 0u);
			nextCmd = LSS_CS_IDENT_SERIAL_LOW;
			break;

		case LSS_CS_IDENT_SERIAL_LOW:
			(void)coNumMemcpyPack(&vendor[4], pData, 4u, 1u, 0u);
			nextCmd = LSS_CS_IDENT_SERIAL_HIGH;
			break;

		case LSS_CS_IDENT_SERIAL_HIGH:
			(void)coNumMemcpyPack(&vendor[5], pData, 4u, 1u, 0u);
			nextCmd = LSS_CS_IDENT_VENDOR;

			/* now compare data with 0x1018 object */
			for (i = 0u; i < 2u; i++)  {
				/* get object from OD */
				retVal = coOdGetObj_u32(0x1018u, i + 1u, &val);
				if (retVal != RET_OK)  {
					return;
				}
				if (val != vendor[i])  {
					return;
				}
			}
			/* version area */
			retVal = coOdGetObj_u32(0x1018u, 3u, &val);
			if (retVal != RET_OK)  {
				return;
			}
			if ((val < vendor[2]) || (val > vendor[3])) {
				return;
			}
			/* serial area */
			retVal = coOdGetObj_u32(0x1018u, 4u, &val);
			if (retVal != RET_OK)  {
				return;
			}
			if ((val < vendor[4]) || (val > vendor[5])) {
				return;
			}

			/* all parameter fit, send answer */
			memset(&trData[0], 0, 8u);
			trData[0] = LSS_CS_IDENTITY_SLAVE;

			/* transmit data */
			(void)icoTransmitMessage(lssTransCob, &trData[0], MSG_OVERWRITE);
			break;

		default:
			nextCmd = LSS_CS_SWITCH_SEL_VENDOR;
			break;
	}
}


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/

#ifdef CO_EVENT_LSS_CNT
/***************************************************************************/
/**
* \brief coEventRegister_LSS - register LSS event
*
* This function registers an indication function for LSS events.
*
* \return RET_T
*
*/

RET_T coEventRegister_LSS(
		CO_EVENT_LSS_T pFunction	/**< pointer to function */
    )
{
	if (lssEventTableCnt >= CO_EVENT_LSS_CNT) {
		return(RET_EVENT_NO_RESSOURCE);
	}

	/* set new indication function as first at the list */
	lssEventTable[lssEventTableCnt] = pFunction;
	lssEventTableCnt++;

	return(RET_OK);
}
#endif /* CO_EVENT_LSS_CNT */


#ifdef CO_EVENT_LSS
/***************************************************************************/
/**
* \internal
*
* \brief lssEvent - call user indication
*
*
* \return RET_T
*
*/
static void lssEvent(
		CO_LSS_SERVICE_T service,
		UNSIGNED16	bitrate,
		UNSIGNED8	*pErrorCode,
		UNSIGNED8	*pErrorSpec
	)
{
#ifdef CO_EVENT_LSS_CNT
UNSIGNED16	cnt;

	/* inform application */
	cnt = lssEventTableCnt;
	while (cnt > 0u)  {
		cnt--;
		/* call user indication */
		lssEventTable[cnt](service, bitrate, pErrorCode, pErrorSpec);
	}
# endif /* CO_EVENT_LSS_CNT */

# ifdef CO_EVENT_STATIC_LSS
	coEventLssInd(service, bitrate, pErrorCode, pErrorSpec);
# endif /* CO_EVENT_STATIC_LSS */
}
#endif /* CO_EVENT_LSS */


/***************************************************************************/
/**
* \internal
*
* \brief coLssResetNodeId - reset LSS pending node id
*
* This function set the pending node id to 255 (invalid),
* so at reset communication no node id is available.
*
* Additional, the coLssNonConfigSlave() is called.
*
* \return none
*
*/
void icoLssResetNodeId(
		void	/* no parameter */
	)
{
UNSIGNED8	nodeId;

	lssPendingNodeId = (UNSIGNED8)CO_NODE_ID;

	/* allow communication for LSS */
	icoLssReset(&nodeId);

	/* if node id == 255, request new node id */
	if (lssPendingNodeId == 255u)  {
		/* call for new node id */
		/* coLssNonConfigSlave(); */
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief icoLssReset - lss for reset communication
*
* \return none
*
*/
void icoLssReset(
		UNSIGNED8	*pNodeId
	)
{
	(void)icoCobSet(lssTransCob, LSS_CAN_ID_SLAVE, CO_COB_RTR_NONE, 8u);
	(void)icoCobSet(lssRecCob, LSS_CAN_ID_MASTER, CO_COB_RTR_NONE, 8u);


	*pNodeId = lssPendingNodeId;

	lssState = CO_LSS_STATE_WAITING;
}


/***************************************************************************/
/**
* \internal
*
* \brief coLssResetAppl() - call at reset application
*
* \return none
*
*/
void icoLssResetAppl(
		void	/* no parameter */
	)
{
	lssPendingNodeId = (UNSIGNED8)CO_NODE_ID;
}


/***************************************************************************/
/**
* \internal
*
* \brief coLssResetAppl() - call at reset application
*
* \return none
*
*/
void icoLssVarInit(
		void
	)
{

	{
		lssNewNodeId = 0u;

		lssTransCob = 0xffffu;
		lssRecCob = 0xffffu;
	}

#ifdef CO_EVENT_LSS_CNT
	lssEventTableCnt = 0u;
#endif /* CO_EVENT_LSS_CNT */
}


/***************************************************************************/
/**
* \brief coLssInit - init LSS functionality
*
* This function initializes the LSS functionality,
* depending on the define CO_LSS_SLAVE_SUPPORTED or CO_LSS_MASTER_SUPPORTED
* as slave or master.
*
* \return RET_T
*
*/
RET_T coLssInit(
		void	/* no parameter */
	)
{
UNSIGNED8	nodeId = 255u;

	lssTransCob = icoCobCreate(CO_COB_TYPE_TRANSMIT, CO_SERVICE_LSS_S_TRANSMIT, LSS_CAN_ID_TYPE_CC);
	if (lssTransCob == 0xffffu)  {
		return(RET_NO_COB_AVAILABLE);
	}

	lssRecCob = icoCobCreate(CO_COB_TYPE_RECEIVE, CO_SERVICE_LSS_S_RECEIVE, LSS_CAN_ID_TYPE_CC);
	if (lssRecCob == 0xffffu)  {
		return(RET_NO_COB_AVAILABLE);
	}

	icoLssReset(&nodeId);

	return(RET_OK);
}
#endif /* CO_LSS_SUPPORTED */
