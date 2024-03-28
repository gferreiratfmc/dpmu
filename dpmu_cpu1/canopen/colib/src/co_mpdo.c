/*
* co_mpdo.c - contains MPDO services
*
* Copyright (c) 2015-2022 emotas embedded communication GmbH
*-------------------------------------------------------------------
* $Id: co_mpdo.c 41921 2022-09-01 10:39:03Z boe $
*
*
*-------------------------------------------------------------------
*
*
*/
 
/********************************************************************/
/**
* \brief MPDO handling
*
* \file co_mpdo.c
* contains MPDO services
*
*/

/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <string.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <gen_define.h>

#if defined(CO_MPDO_PRODUCER) || defined(CO_MPDO_CONSUMER)
#include <co_datatype.h>
#include <co_odaccess.h>
#include <co_drv.h>
#include <co_nmt.h>
#include <co_timer.h>
#include "ico_indication.h"
#include "ico_cobhandler.h"
#include "ico_odaccess.h"
#include "ico_queue.h"
#include "ico_pdo.h"

/* constant definitions
---------------------------------------------------------------------------*/
#define CO_MPDO_ADDR_DEST	0x80u
/* #define CO_MPDO_ADDR_SRC	0x00u */

#ifdef CO_EVENT_DYNAMIC_MPDO
# ifdef CO_EVENT_PROFILE_MPDO
#  define CO_EVENT_NMT_MPDO		(CO_EVENT_DYNAMIC_MPDO + CO_EVENT_PROFILE_MPDO)
# else /* CO_EVENT_PROFILE_MPDO */
#  define CO_EVENT_MPDO_CNT		(CO_EVENT_DYNAMIC_MPDO)
# endif /* CO_EVENT_PROFILE_MPDO */
#else /* CO_EVENT_DYNAMIC_MPDO */
# ifdef CO_EVENT_PROFILE_MPDO
#  define CO_EVENT_MPDO_CNT		(CO_EVENT_PROFILE_MPDO)
# endif /* CO_EVENT_PROFILE_MPDO */
#endif /* CO_EVENT_DYNAMIC_MPDO */

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
# ifdef CO_MPDO_CONSUMER
static void mpdoEventInd(UNSIGNED16 pdoNr, UNSIGNED16 index,
		UNSIGNED8 subIndex);
# endif /* CO_MPDO_CONSUMER */

/* external variables
---------------------------------------------------------------------------*/
# ifdef CO_MPDO_CONSUMER
#  ifdef CO_EVENT_STATIC_MPDO
extern CO_CONST CO_EVENT_MPDO_T coEventMPdoInd;
#  endif /* CO_EVENT_STATIC_MPDO */
# endif /* CO_MPDO_CONSUMER */

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
# ifdef CO_MPDO_CONSUMER
#  ifdef CO_EVENT_MPDO_CNT
static CO_EVENT_MPDO_T mpdoEventTable[CO_EVENT_MPDO_CNT];
static UNSIGNED16	mpdoEventTableCnt = 0u;
#  endif /* CO_EVENT_MPDO_CNT */
#  ifdef  CO_MPDO_SAM 
static UNSIGNED16	samIndex;
static UNSIGNED8	samSubIndex;
static UNSIGNED8	samNodeId;
static BOOL_T	samInCallBack;
#  endif /* CO_MPDO_SAM */
# endif /* CO_MPDO_CONSUMER */


# ifdef CO_MPDO_PRODUCER
/***************************************************************************/
/**
* \brief coMPdoReq - request MPDO transmission
*
* This function requests the transmission of a MPDO.<br>
* The mapped objects are automatically copied into the CAN message.
* If the inhibit time is not active,
* then the message is transmitted immediately.
*
* If the inhibit time is not ellapsed yet,
* the transmission depends on the parameter flags:
*
* Parameter index/subIndex depends on MPDO mode.
* Parameter node is only valid for MPDO Destination mode adress.
*
* MSG_OVERWRITE - if the last PDO is not transmitted yet,
*	overwrite the last data with the new data
* MSG_RET_INHIBIT - return the function with RET_INHIBIT_ACTIVE,
*	if the inhibit is not ellapsed yet
*
* with the same or 
* \return RET_T
* \retval RET_INVALID_NMT_STATE
*	invalid NMT state
* \retval RET_INVALID_PARAMETER
*	unknown PDO number
* \retval RET_COB_DISABLED
*	PDO is disabled
* \retval RET_INHIBIT_ACTIVE
*	inhibit time is not yet ellapsed
* \retval RET_OK
*	all function are ok, but have not to be transmitted yet
*
*/
RET_T coMPdoReq(
		UNSIGNED16		pdoNr,		/**< PDO number */
		UNSIGNED8		dstNode,	/**< destination node */
		UNSIGNED16		index,		/**< destination/source index */
		UNSIGNED8		subIndex,	/**< destination/source subIndex */
		UNSIGNED8		flags		/**< transmit flags */
	)
{
RET_T	retVal = RET_OK;
CO_TR_PDO_T *pPdo;
UNSIGNED8	trData[CO_CAN_MAX_DATA_LEN];	/* data */
#ifdef CO_MPDO_IGNORE_SCANNER_LIST
#else /* CO_MPDO_IGNORE_SCANNER_LIST */
UNSIGNED16	si;
UNSIGNED8	ssi;
UNSIGNED8	sbs;
UNSIGNED8	maxSub;
UNSIGNED16	i;
UNSIGNED8	s;
UNSIGNED32	scanEntry;
#endif/* CO_MPDO_IGNORE_SCANNER_LIST */
UNSIGNED32	len;
void		*ptr;
CO_OBJECT_DESC_T CO_CONST *pDesc;
BOOL_T		found = CO_FALSE;
UNSIGNED16	attr;

	/* OPERATIONAL ? */
	if (coNmtGetState() != CO_NMT_STATE_OPERATIONAL)  {
		return(RET_INVALID_NMT_STATE);
	}

	pPdo = icoPdoSearchTransmitPdo(pdoNr);
	if (pPdo == NULL)  {
		return(RET_INVALID_PARAMETER);
	}

	/* check, if PDO is not disabled */
	if ((pPdo->cobId & CO_COB_VALID_MASK) == CO_COB_INVALID)  {
		return(RET_COB_DISABLED);
	}

	/* check if PDO is MPDO */
	if (pPdo->pdoType == CO_PDO_TYPE_STD)  {
		return(RET_INVALID_PARAMETER);
	}

	/* DAM ? */
	if (pPdo->pdoType == CO_PDO_TYPE_DAM)  {
		trData[0] = CO_MPDO_ADDR_DEST | dstNode; 
		/* map pdo data */
		/* numeric value ? */
		coNumMemcpyUnpack(&trData[4],
			pPdo->mapTableConst->mapEntry[0].pVar,
			(UNSIGNED32)pPdo->mapTableConst->mapEntry[0].len,
			(UNSIGNED16)pPdo->mapTableConst->mapEntry[0].numeric, 0u);
	} else {
		/* SAM */
		trData[0] = coNmtGetNodeId(); 

#ifdef CO_MPDO_IGNORE_SCANNER_LIST
#else /* CO_MPDO_IGNORE_SCANNER_LIST */
		/* search the requested object at the scanner list 0x1fa0..0x1fcf */
		for (i = 0x1fa0u; i <= 0x1fcfu; i++)  {
			retVal = coOdGetObj_u8(i, 0u, &maxSub);
			/* assume, no more objects available */
			if (retVal == RET_OK)  {
				for (s = 1u; s <= maxSub; s++)  {
					/* match requested idx/subidx? */
					retVal = coOdGetObj_u32(i, s, &scanEntry);
					if (retVal == RET_OK)  {
						si = (UNSIGNED16)(scanEntry >> 8u);
						ssi = (UNSIGNED8)(scanEntry & 0xffu);
						sbs = (UNSIGNED8)(scanEntry >> 24u);
						if (((index == si)
						 && (subIndex >= ssi)
						 && (subIndex <= (ssi + sbs))) ||
						 ((si == 0xffffu)
						 && (ssi == 0xffu)
						 && (sbs == 0xffu))) {
							found = CO_TRUE;
							break;
						}
					}
				}
				if (found == CO_TRUE)  {
					break;
				}
			}
		}

		if (found == CO_FALSE)  {
			return(RET_INVALID_PARAMETER);
		}
#endif /* CO_MPDO_IGNORE_SCANNER_LIST */

		/* map data */
		memset(&trData[4], 0, 4u);
		retVal = coOdGetObjDescPtr(index, subIndex, &pDesc);
		if (retVal != RET_OK)  {
			return(RET_INVALID_PARAMETER);
		}

		attr = coOdGetObjAttribute(pDesc);

		/* check for mapable */
		if ((attr & (CO_ATTR_MAP | CO_ATTR_MAP_TR)) == 0u)  {
			return(RET_MAP_ERROR);
		}

		ptr = coOdGetObjAddr(index, subIndex);
		len = coOdGetObjSize(pDesc);

		coNumMemcpyUnpack(&trData[4], ptr, len, 1u, 0u);
	}

	trData[1] = (UNSIGNED8)(index & 0xffu);
	trData[2] = (UNSIGNED8)((index >> 8) & 0xffu); 
	trData[3] = subIndex; 

	/* transmit data */
	retVal = icoTransmitMessage(pPdo->cob, &trData[0], flags);

	return(retVal);
}
# endif /* CO_MPDO_PRODUCER */


# ifdef CO_MPDO_CONSUMER
/***************************************************************************/
/**
* \internal
*
* \brief icoMPdoReceive - receive MPDO
*
*/
void icoMPdoReceive(
		CO_CONST CO_REC_PDO_T	*pPdo,		/* PDO */
		CO_CONST CO_REC_DATA_T	*pRecData	/* pointer to received data */
	)
{
UNSIGNED32	len;
void		*ptr;
CO_OBJECT_DESC_T CO_CONST *pDesc;
UNSIGNED16	index;
UNSIGNED8	subIndex;
UNSIGNED64	dispEntry;
UNSIGNED8	rnid;
UNSIGNED8	rsi = 0u;
UNSIGNED8	subIdxOffs;
RET_T		retVal;
BOOL_T		found = CO_FALSE;
BOOL_T		changed;

	memset(&dispEntry, 0, 8u);
	index = ((UNSIGNED16)pRecData->msg.data[2]) << 8u;
	index |= pRecData->msg.data[1];
	subIndex = pRecData->msg.data[3];

	/* destination mode ? */
	if (pPdo->pdoType == CO_PDO_TYPE_DAM)  {
		/* broadcast ? */
		if (pRecData->msg.data[0] != CO_MPDO_ADDR_DEST) {
			/* my node ? */
			rnid = coNmtGetNodeId();
			rnid |= CO_MPDO_ADDR_DEST;
			if (pRecData->msg.data[0] != rnid)  {
				return;
			}
		}
	} else  {
#  ifdef CO_MPDO_SAM
	UNSIGNED8	maxSub;
	UNSIGNED16	ri;
	UNSIGNED8	bs;
	UNSIGNED8	nid;
	UNSIGNED16	i;
	UNSIGNED8	s;
		/* src mode - use dispatcher list at 0x1fd00 */
		nid = pRecData->msg.data[0];

		/* is the dest bit set, or source adress invalid, ignore */
		if ((nid > 0x7fu) || (nid == 0u))  {
			return;
		}

		for (i = 0x1fd0u; i <= 0x1fffu; i++)  {
			retVal = coOdGetObj_u8(i, 0u, &maxSub);
			/* assume, no more objects available */
			if (retVal == RET_OK)  {
				for (s = 1u; s <= maxSub; s++)  {
					/* match requested idx/subidx? */
					retVal = coOdGetObj_u64(i, s, &dispEntry);
					if (retVal == RET_OK)  {
						/* get remote id, idx, subidx, blocksize */
						rnid = dispEntry.val[0];
						rsi = dispEntry.val[1];
						ri = (UNSIGNED16)dispEntry.val[3] << 8;
						ri |= dispEntry.val[2];
						bs = dispEntry.val[7];
#ifdef CO_MPDO_WILDCARD_DISPATCHER_LIST
						/* allows to receive MPDOs from:
						 * all remote node IDs when setting the sender nodeID to 0xFF
						 * all remote indices when setting the sedner index to 0xFFFF
						 * all remote sub indices when setting the remote sub indices to 0xFF */
						if (((nid == rnid) || (rnid == 0xff)) &&
							((index == ri) || (ri == 0xffff)) &&
							(((subIndex >= rsi) && (subIndex <= (rsi + bs))) || (rsi >= 0xFF)))  {
#else /* CO_MPDO_WILDCARD_DISPATCHER_LIST */
						/* use normal dispatcher list */
						if (((nid == rnid)
						 && (index == ri)
						 && (subIndex >= rsi)
						 && (subIndex <= (rsi + bs))) 
						 ||	((ri == 0xffffu)
						 && (rsi == 0xffu)
						 && (rnid== 0xffu)))  {
#endif /* CO_MPDO_WILDCARD_DISPATCHER_LIST */
							found = CO_TRUE;
							/* save the remote information for user */
							samSubIndex = subIndex;
							samNodeId = nid & 0x7fu;
							samIndex = index;
							break;
						}
					}
				}
			}
			if ((retVal != RET_OK) || (found == CO_TRUE))  {
				break;
			}
		}
#  endif /* CO_MPDO_SAM */
		if (found == CO_FALSE)  {
			return;
		}

		/* save data */
		index = (UNSIGNED16)dispEntry.val[6] << 8;
		index |= dispEntry.val[5];
		/* calculate offs subindex */
		if (rsi != 0xffu) {
			subIdxOffs = subIndex - rsi;
		} else {
			subIdxOffs = 0u;
		}
		subIndex = (dispEntry.val[4]) + subIdxOffs;
	}

	retVal = coOdGetObjDescPtr(index, subIndex, &pDesc);
	if (retVal != RET_OK)  {
		return;
	}
	ptr = coOdGetObjAddr(index, subIndex);
	len = coOdGetObjSize(pDesc);

	changed = coNumMemcpyPack(ptr, &pRecData->msg.data[4], len, 1u, 0u);
#ifdef CO_EVENT_OBJECT_CHANGED
	if (changed == CO_TRUE)  {
		(void)icoEventObjectChanged(pDesc, index, subIndex, changed);
	}
#else /* CO_EVENT_OBJECT_CHANGED */
	(void)changed;
#endif /* CO_EVENT_OBJECT_CHANGED */

	/* call indication */
	mpdoEventInd(pPdo->pdoNr, index, subIndex);
}


# ifdef CO_MPDO_CONSUMER
#  ifdef CO_MPDO_SAM
/***************************************************************************/
/**
* \brief coMPdoGetSamInfo - get source mode information
*
* In the MPdo callback this function can be used to get the source
* address mode informations.
*
* \return RET_T
*
*/
RET_T coMPdoGetSamInfo(
		UNSIGNED16	*pRIndex,
		UNSIGNED8	*pRSubIndex,
		UNSIGNED8	*pRNodeId
	)
{
	if (samInCallBack != CO_TRUE)  {
		return(RET_VALUE_NOT_AVAILABLE);
	}

	if ((pRSubIndex == NULL) ||
		(pRNodeId == NULL) ||
		(pRIndex == NULL))  {
		return(RET_EVENT_NO_RESSOURCE);
	}

	*pRIndex = samIndex;
	*pRNodeId = samNodeId;
	*pRSubIndex = samSubIndex;

	return(RET_OK);
}
#  endif /* CO_MPDO_SAM */
# endif /* CO_MPDO_CONSUMER */


#  ifdef CO_EVENT_MPDO_CNT
/***************************************************************************/
/**
* \brief coEventRegister_MPDO - register MPDO event
*
* Register an indication function for Multiplexed PDOs.
*
* After a PDO has been received, the data are stored in the object dictionary,
* and then the given indication function is called.
* This function is only valid for MPDOs.
*
* \return RET_T
*
*/
RET_T coEventRegister_MPDO(
		CO_EVENT_MPDO_T pFunction	/**< pointer to function */
    )
{
	if (mpdoEventTableCnt >= CO_EVENT_MPDO_CNT) {
		return(RET_EVENT_NO_RESSOURCE);
	}

	if (pFunction == NULL) {
		return(RET_NOT_INITIALIZED);
	}

	/* set new indication function as first at the list */
	mpdoEventTable[mpdoEventTableCnt] = pFunction;
	mpdoEventTableCnt++;

	return(RET_OK);
}
#  endif /* CO_EVENT_MPDO_CNT */


/***************************************************************************/
/**
* \internal
*
* \brief mpdoEventInd - mpdo event occured
*
* This function is called, if mpdo was received
*
* \return RET_T
*
*/
static void mpdoEventInd(
		UNSIGNED16		pdoNr,		/* pdo number */
		UNSIGNED16		index,		/* index */
		UNSIGNED8		subIndex	/* subIndex */
	)
{
UNSIGNED16	cnt;

#  ifdef CO_MPDO_SAM
	samInCallBack = CO_TRUE;
#  endif /* CO_MPDO_SAM */

#  ifdef CO_EVENT_MPDO_CNT
	/* call indication to execute */
	cnt = mpdoEventTableCnt;
	while (cnt > 0u)  {
		cnt--;
		/* call user indication */
		mpdoEventTable[cnt](pdoNr, index, subIndex);
	}
#  endif /*  CO_EVENT_MPDO_CNT */

#  ifdef CO_EVENT_STATIC_MPDO
	coEventMPdoInd(pdoNr, index, subIndex);
#  endif /* CO_EVENT_STATIC_MPDO */

#  ifdef CO_MPDO_SAM
	samInCallBack = CO_FALSE;
#  endif /* CO_MPDO_SAM */

(void)cnt;
(void)pdoNr;
(void)index;
(void)subIndex;

	return;
}
# endif /* CO_MPDO_CONSUMER */
#endif /* defined(CO_MPDO_PRODUCER) || defined(CO_MPDO_CONSUMER) */
