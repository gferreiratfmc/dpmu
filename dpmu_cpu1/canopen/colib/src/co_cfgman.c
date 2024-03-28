/*
* co_cfgman.c - contains configuration manager handling
*
* Copyright (c) 2014-2022 emotas embedded communication GmbH
*-------------------------------------------------------------------
* $Id: co_cfgman.c 41921 2022-09-01 10:39:03Z boe $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \brief config manager handling
*
* \file co_cfgman.c
* contains configuration manager handling
*
*/

/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <gen_define.h>

#ifdef CO_CFG_MANAGER
#include <co_datatype.h>
#include <co_cfgman.h>
#include <co_sdo.h>
#include <co_odaccess.h>

/* constant definitions
---------------------------------------------------------------------------*/
#ifdef CO_NO_CONCISE_CONVERTING
#else /* CO_NO_CONCISE_CONVERTING */
# define CO_CONCISE_CONVERT
#endif /* CO_NO_CONCISE_CONVERT */

#ifdef CO_EVENT_DYNAMIC_CFG_MAN
# define CO_EVENT_CFG_MAN_CNT     (CO_EVENT_DYNAMIC_CFG_MAN)
#endif /* CO_EVENT_DYNAMIC_CFG_MAN */

#if defined(CO_EVENT_STATIC_CFG_MAN) || defined(CO_EVENT_CFG_MAN_CNT)
# define CO_EVENT_CFG_MAN   1
#endif /* defined(CO_EVENT_STATIC_SYNC) || defined(CO_EVENT_CFG_MAN_CNT) */

#define MAX_TMP_BUF	20u

/* local defined data types
---------------------------------------------------------------------------*/
typedef enum {
	CO_DCF_CONVERT_START,
 	CO_DCF_CONVERT_MAPPING,
 	CO_DCF_CONVERT_PDO
} CO_DCF_MODE_T;

typedef struct {
	UNSIGNED32	nrOfEntries;
	UNSIGNED32	bufLen;
	UNSIGNED32	bufIdx;
	UNSIGNED32	tOut;
	UNSIGNED16	idx;
	UNSIGNED8	busy;
	UNSIGNED8	*pBuf;
	UNSIGNED8	sdoNr;
	UNSIGNED8	subIdx;
	UNSIGNED8	sdoClientNr;
} CFG_SDO_T;

typedef char	CHAR;

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/
#ifdef CO_EVENT_STATIC_CFG_MAN
extern CO_CONST CO_EVENT_CFG_MANAGER_T coEventCfgManInd;
#endif /* CO_EVENT_STATIC_CFG_MAN */


/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
#ifdef CO_CONCISE_CONVERT
static RET_T convertToConcise(CO_DCF_MODE_T mode, CHAR *pDcf,
			UNSIGNED8 *pConsBuf, UNSIGNED32	*pConsBufLen);
static RET_T prepareDcfEntry(UNSIGNED8 *pConsBuf,
			UNSIGNED32 *pConsBufLen, UNSIGNED16	idx, UNSIGNED8	subIdx);
static UNSIGNED32 convertDcfEntry(
			CHAR *pDataStart, CHAR *pDataEnd, UNSIGNED32 *pVal);
static RET_T getKeyString(CO_CONST CHAR *pDataStart, CO_CONST CHAR *pDataEnd,
			CO_CONST CHAR *keyStrg, CHAR *pDst, UNSIGNED8 maxLen);
static RET_T saveDcfEntry(UNSIGNED8 *pConsBuf,
			UNSIGNED32 *pConsBufLen,
			UNSIGNED16 idx, UNSIGNED8 subIdx, UNSIGNED32 val, UNSIGNED32 len);
static RET_T disablePdo(UNSIGNED8 *pConsBuf,
			UNSIGNED32	*pConsBufLen, UNSIGNED16 idx);
static RET_T disableMapping(UNSIGNED8 *pConsBuf,
			UNSIGNED32	*pConsBufLen, UNSIGNED16 idx);
static RET_T saveMappingDcfEntry(UNSIGNED8 *pConsBuf,
			UNSIGNED32 *pConsBufLen,
			UNSIGNED16 idx, UNSIGNED8 subIdx, UNSIGNED32 val, UNSIGNED32 len);
static RET_T savePdoDcfEntry(UNSIGNED8 *pConsBuf,
			UNSIGNED32 *pConsBufLen,
			UNSIGNED16 idx, UNSIGNED8 subIdx, UNSIGNED32 val, UNSIGNED32 len);
#endif /* CO_CONCISE_CONVERT */
static RET_T cfgSdoRequest(UNSIGNED8 sdoNr);
static void cfgSdoAnswer(UNSIGNED8 sdoNr, UNSIGNED16 index,
			UNSIGNED8 subIndex, UNSIGNED32 result);
static void cfgIndication(CO_CFG_TRANSFER_T type, UNSIGNED8 sdoNr,
			UNSIGNED16 index, UNSIGNED8 subIndex, UNSIGNED32 reason);
static CFG_SDO_T *getSdoCfg(UNSIGNED8 sdoClientNr);

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
# ifdef CO_CONCISE_CONVERT
static UNSIGNED8	tPdoDisabled[512u / 8u];
static UNSIGNED8	rPdoDisabled[512u / 8u];
static UNSIGNED8	tMapDisabled[512u / 8u];
static UNSIGNED8	rMapDisabled[512u / 8u];
static UNSIGNED32	maxConsBuf;
static UNSIGNED8	nodeId;
static UNSIGNED32	nrOfEntries;
# endif /* CO_CONCISE_CONVERT */
static CFG_SDO_T	sdoCfg[CO_SDO_CLIENT_CNT];
static BOOL_T		sdoIndRegistered = { CO_FALSE };
# ifdef CO_EVENT_CFG_MAN_CNT
static UNSIGNED16	cfgManEventTableCnt = 0u;
static CO_EVENT_CFG_MANAGER_T	cfgManEventTable[CO_EVENT_CFG_MAN_CNT];
# endif /* CO_EVENT_CFG_MAN_CNT */


/***************************************************************************/
/**
* \brief co_cfgStart - start configuration 
*
* This function starts the SDO transfer to setup a node
* with a new configuration.
* Parameter are given as concise DCF buffer.
* For the SDO transfer, the client with sdoNr is used.
* If parameter srvNodeId != 0, then the sdo channel is automatically configured
* with the default server sdo cobs for the given nodeId.
*
* If transfer is started successful, the function returns RET_OK.
* Finish of the whole transfer is indicated by the function
* configured by coEventRegister_CFG_MANAGER().
*
* \return RET_T
*
*/
RET_T coCfgStart(
		UNSIGNED8	sdoNr,				/**< use sdo number */
		UNSIGNED8	srvNodeId,			/**< write to node n */
		UNSIGNED8	*pBuf,				/**< pointer to concise dcf buffer */
		UNSIGNED32	bufLen,				/**< len of concise dcf buffer */
		UNSIGNED32	sdoTimeOut			/**< SDO timeout in msec */
	)
{
RET_T	retVal;
UNSIGNED16	index;
CFG_SDO_T *pSdoCfg;

	/* register SDO indication first time call this function */
	if (sdoIndRegistered == CO_FALSE)  {
		retVal = coEventRegister_SDO_CLIENT_WRITE(cfgSdoAnswer);
		if (retVal != RET_OK)  {
			return(retVal);
		}
		sdoIndRegistered = CO_TRUE;
	}

	/* check sdo Nr */
	if ((sdoNr < 1u) || (sdoNr > 128u))  {
		return(RET_INVALID_PARAMETER);
	}

	pSdoCfg = getSdoCfg(sdoNr);
	if (pSdoCfg == NULL)  {
		return(RET_INVALID_PARAMETER);
	}

	/* if configuration in work ? */
	if (pSdoCfg->busy != 0u)  {
		return(RET_INVALID_PARAMETER);
	}

	/* setup sdo */
	if ((srvNodeId > 0u) && (srvNodeId < 128u))  {
		index = 0x1280u + sdoNr;
		index -= 1u;

		retVal = coOdSetCobid(index, 1u, 0x600ul + srvNodeId);
		if (retVal != RET_OK)  {
			return(retVal);
		}
		retVal = coOdSetCobid(index, 2u, 0x580ul + srvNodeId);
		if (retVal != RET_OK)  {
			return(retVal);
		}
	}

	/* save concise buffer and size (ignore number of entries) */
	memcpy(&pSdoCfg->nrOfEntries, pBuf, 4);
	pSdoCfg->pBuf = pBuf + 4u;
	pSdoCfg->bufLen = bufLen - 4u;
	pSdoCfg->bufIdx = 0u;
	pSdoCfg->sdoNr = sdoNr;
	pSdoCfg->tOut = sdoTimeOut;

	/* call first SDO transfer */
	retVal = cfgSdoRequest(sdoNr);
	if (retVal == RET_SERVICE_BUSY)  {
		pSdoCfg->busy = 1u;
		retVal = RET_OK;
	}

	return(retVal);
}


#ifdef CO_CONCISE_CONVERT
/***************************************************************************/
/**
* \brief coCfgConvToConciseDcf - convert to concise DCF
*
* This function convert the given data to the concise DCF.
* At function call the parameter pConsBufLen contains the maximal buffer length,
* and is updated with the real len of written buffer.
*
* \return RET_T
*
*/
RET_T coCfgConvToConcise(
		CHAR		*pDcfData,		/**< pointer to DCF data */
		UNSIGNED8	*pConsBuf,		/**< pointer to concise DCF buffer */
		UNSIGNED32	*pConsBufLen	/**< max len of concise DCF buffer */
	)
{
RET_T	retVal;
UNSIGNED16	i;

	maxConsBuf = *pConsBufLen;
	if (*pConsBufLen < 4)  {
		return(RET_OUT_OF_MEMORY);
	}
	*pConsBufLen = 4u;
	nrOfEntries = 0;
	nodeId = 0u;
	for (i = 0u; i < (512u / 8u); i++)  {
		tPdoDisabled[i] = 0u;
		rPdoDisabled[i] = 0u;
		tMapDisabled[i] = 0u;
		rMapDisabled[i] = 0u;
	}

	/*
	 * Konfiguriere alle Daten ausser PDO-COBs und Mapping Eintrag 0
	 * 		Disable all used PDO entries COB-ID
	 *  	Disable all used PDO entries mapping (if used)
	 *  	COB-IDs werden automatisch erst disabled und anschliessend neu geschrieben
	 * Configure all mapping entries sub 0
	 * Enable alle genutzten PDOs
	 */

	retVal = convertToConcise(CO_DCF_CONVERT_START, pDcfData, pConsBuf, pConsBufLen);
	if (retVal != RET_OK)  {
		return(retVal);
	}
	retVal = convertToConcise(CO_DCF_CONVERT_MAPPING, pDcfData, pConsBuf, pConsBufLen);
	if (retVal != RET_OK)  {
		return(retVal);
	}
	retVal = convertToConcise(CO_DCF_CONVERT_PDO, pDcfData, pConsBuf, pConsBufLen);
	if (retVal != RET_OK)  {
		return(retVal);
	}

	if (nodeId == 0u)  {
		/* save number of entries at start of buffer */
		memcpy(pConsBuf, &nrOfEntries, 4u);

		/* no default node id known, return */
		return(retVal);
	}

	/* check for not enabled default PDOs */
	for (i = 0u; i < 4u; i++)  {
		if ((tPdoDisabled[0u] & (1ul << i)) != 0u)  {
			/* setup default cobid */
			retVal = saveDcfEntry(pConsBuf, pConsBufLen, 0x1800u + i,
				1u, 0x180ul + (0x100ul * i) + nodeId, 4ul);
			if (retVal != RET_OK)  {
				return(retVal);
			}
		}

		if ((rPdoDisabled[0u] & (1ul << i)) != 0u)  {
			/* setup default cobid */
			retVal = saveDcfEntry(pConsBuf, pConsBufLen, 0x1400u + i,
				1u, 0x200ul + (0x100ul * i) + nodeId, 4ul);
			if (retVal != RET_OK)  {
				return(retVal);
			}
		}
	}

	/* save number of entries at start of buffer */
	memcpy(pConsBuf, &nrOfEntries, 4u);

	return(retVal);
}
#endif /* CO_CONCISE_CONVERT */


/***************************************************************************/
/**
* \internal
*
* \brief cfgSdoAnswer - answer of client sdo transfer
*
* This function is the indication function after a sdo transfer was finished.
* If no error was indicated, the next transfer is started.
*
* \return void
*
*/
static void cfgSdoAnswer(
		UNSIGNED8 sdoNr,				/* sdo number */
		UNSIGNED16 index,				/* object index */
		UNSIGNED8 subIndex,				/* object subindex */
		UNSIGNED32 result				/* result of transfer */
	)
{
RET_T	retVal;
CFG_SDO_T	*pCfg;

	pCfg = getSdoCfg(sdoNr);

	/* check only own request */
	if ((sdoNr != pCfg->sdoNr)
	 || (index != pCfg->idx)
	 || (subIndex != pCfg->subIdx)
	 || (pCfg->busy == 0u))  {
		return;
	}

	if (result != 0u)  {
		/* call user indication */
		cfgIndication(CO_CFG_TRANSFER_ABORT, sdoNr,
				index, subIndex, result);
	} else {

		/* call next entry */
		retVal = cfgSdoRequest(sdoNr);
		if (retVal == RET_OK)  {
			/* transfer finished, call indication */
			cfgIndication(CO_CFG_TRANSFER_FINISHED, sdoNr, 0u, 0u, 0u);
		} else 
		if (retVal == RET_SERVICE_BUSY)  {
			/* transfer in progress */

		} else {
			/* error, abort indication */
			cfgIndication(CO_CFG_TRANSFER_ERROR, sdoNr,
					index, subIndex, (UNSIGNED32)retVal);
		}
	}
}


/***************************************************************************/
/**
* \internal
*
* \brief cfgSdoRequest - start a new sdo transfer
*
* This function starts a new sdo transfer with the parameter
* from the concise dcf buffer
*
* \return RET_T
*
*/
static RET_T cfgSdoRequest(
		UNSIGNED8 sdoNr		/* sdo number */
	)
{
UNSIGNED32	len;
UNSIGNED8	*pData;
RET_T		retVal;
CFG_SDO_T	*pCfg;

	pCfg = getSdoCfg(sdoNr);
	if (pCfg == NULL)  {
		return(RET_INVALID_PARAMETER);
	}

	/* more request available ? */
	if (pCfg->nrOfEntries == 0)  {
		return(RET_OK);
	}
	if ((pCfg->bufIdx + 7u) > pCfg->bufLen)  {
		return(RET_OK);
	}

	/* get next request data from dcf buffer */
	memcpy(&pCfg->idx, &pCfg->pBuf[pCfg->bufIdx], 2u);
	pCfg->bufIdx += 2u;
	memcpy(&pCfg->subIdx, &pCfg->pBuf[pCfg->bufIdx], 1u);
	pCfg->bufIdx += 1u;
	memcpy(&len, &pCfg->pBuf[pCfg->bufIdx], 4u);
	pCfg->bufIdx += 4u;
	pData = &pCfg->pBuf[pCfg->bufIdx];
	pCfg->bufIdx += len;

	if (pCfg->bufIdx > pCfg->bufLen)  {
		return(RET_OUT_OF_MEMORY);
	}
	/* start SDO transfer */
	retVal = coSdoWrite(sdoNr, pCfg->idx, pCfg->subIdx, pData, len, 1u, pCfg->tOut);
	if (retVal == RET_OK)  {
		pCfg->nrOfEntries--;
		retVal = RET_SERVICE_BUSY;
	} else {
		retVal = RET_INTERNAL_ERROR;
	}

	return(retVal);
}


#ifdef CO_CONCISE_CONVERT
/***************************************************************************/
/**
* \internal
*
* \brief convertToConcise - convert data to concise DCF
*
* convert dcf to concise dcf for different modes
*
* \return RET_T
*
*/
static RET_T convertToConcise(
		CO_DCF_MODE_T	mode,		/* mode */
		CHAR	*pDcf,				/* pointer to DCF data */
		UNSIGNED8	*pConsBuf,		/* pointer to concise DCF buffer */
		UNSIGNED32	*pConsBufLen	/* max len of concise DCF buffer */
	)
{
CHAR	*pBrL; 
CHAR	*pBrR = NULL;
UNSIGNED16	idx;
UNSIGNED8	subIdx;
CHAR	buf[MAX_TMP_BUF];
CHAR	*pSub;
UNSIGNED32	len;
UNSIGNED32	val;
RET_T	retVal;

	/* for all given data at DCF buffer */
	while (pDcf != NULL)  {

		/* search for first [ */
		pBrL = strchr(pDcf, '[');
		if (pBrL == NULL)  {
			break;
		}
		/* search for ] */
		pBrR = strchr(pBrL, ']');
		if (pBrR == NULL)  {
			return(RET_CFG_CONVERT_ERROR);
		}
		pDcf = pBrR + 1u;

		/* index starts with a digit */
		if (isdigit(pBrL[1u]) != 0)  {
			/* index found - copy to temp buf */
			len =  (UNSIGNED32)(pBrR - pBrL);
			if (len > MAX_TMP_BUF)  {
				return(RET_CFG_CONVERT_ERROR);
			}
			len -= 1u;
			strncpy(buf, pBrL + 1u, len);
			buf[len] = '\0';
			/* "sub" in string ? */
			pSub = strstr(buf, "sub");
			if (pSub == NULL)  {
				/* no subindex available */
				idx = (UNSIGNED16)strtol(buf, NULL, 16);
				subIdx = 0u;
			} else {
				/* subindex available */
				idx = (UNSIGNED16)strtol(buf, &pSub, 16);
				subIdx = (UNSIGNED8)strtol(pSub + 3u, NULL, 16);
			}
			if (idx == 0u)  {
				return(RET_CFG_CONVERT_ERROR);
			}

			/* search next bracket */
			pBrL = strchr(pDcf, '[');
			if (pBrL == NULL)  {
				/* or use end of dcf buffer */
				pBrL = &pDcf[strlen(pDcf) - 1];
			}

			/* valid entry found ? */
			len = convertDcfEntry(pBrR, pBrL, &val);

			/* Standard convert mode without PDO COB-IDs and mapping count */
			if (mode == CO_DCF_CONVERT_START)  {
				if (len != 0u)  {
					retVal = prepareDcfEntry(pConsBuf, pConsBufLen,
						idx, subIdx);
					if (retVal == RET_OK)  {
						retVal = saveDcfEntry(pConsBuf, pConsBufLen,
								idx, subIdx, val, len);
						if (retVal != RET_OK)  {
							return(retVal);
						}
					}
				}
			}

			/* write PDO Mapping count */
			if (mode == CO_DCF_CONVERT_MAPPING)  {
				if (len != 0u)  {
					retVal = saveMappingDcfEntry(pConsBuf, pConsBufLen, idx, subIdx, val, len);
					if (retVal != RET_OK)  {
						return(retVal);
					}
				}
			}

			/* write PDO cobs */
			if (mode == CO_DCF_CONVERT_PDO)  {
				retVal = savePdoDcfEntry(pConsBuf, pConsBufLen, idx, subIdx, val, len);
				if (retVal != RET_OK)  {
					return(retVal);
				}
			}
		} else {
			/* ignore all except DeviceCommissioning nodeId */
			if (strstr(pBrL, "DeviceComissioning") != NULL)  {
                CHAR    nodeIdBuf[10];

				/* search next bracket */
				pBrL = strchr(pDcf, '[');

				retVal = getKeyString(pBrR, pBrL, "NodeID", nodeIdBuf, 10u);
				if (retVal == RET_OK)  {
					nodeId = (UNSIGNED8)strtol(nodeIdBuf, NULL, 0);
				}
			}
		}
	}

	return(RET_OK);
}


/***************************************************************************/
/**
* \internal
*
* \brief convertDcfEntry - convert a DCF entry
*
* convert DCF entry from string to numeric value
*
* \return data len
*
*/
static UNSIGNED32 convertDcfEntry(
		CHAR		*pDataStart,	/* string data start */
		CHAR		*pDataEnd,		/* string data end */
		UNSIGNED32	*pVal			/* converted data */
	)
{
CHAR	parVal[MAX_TMP_BUF];
CHAR	dataType[MAX_TMP_BUF];
CHAR	accessType[MAX_TMP_BUF];
RET_T	retVal;
UNSIGNED32	len;
REAL32		f;

	/* search for ParameterValue */
	retVal = getKeyString(pDataStart, pDataEnd, "ParameterValue", parVal, MAX_TMP_BUF);
	if (retVal == RET_OK)  {
		/* check accesstype */
		retVal = getKeyString(pDataStart, pDataEnd, "AccessType", accessType, MAX_TMP_BUF);
		if ((strcmp(accessType, "ro") == 0)
		 || (strcmp(accessType, "const") == 0))  {
			retVal = RET_NO_WRITE_PERM;
		}
	}
	if (retVal == RET_OK)  {
		/* parameter value available */
		retVal = getKeyString(pDataStart, pDataEnd, "DataType", dataType, MAX_TMP_BUF);
		if (retVal != RET_OK)  {
			return(0ul);
		}

		*pVal = (UNSIGNED32)strtol(parVal, NULL, 0);
		switch (strtol(dataType, NULL, 16))  {
			case 1:		/* bool */
				len = 1u;
				break;
			case 2:		/* i8 */
				len = 1u;
				break;
			case 3:		/* i16 */
				len = 2u;
				break;
			case 4:		/* i32 */
				len = 4u;
				break;
			case 5:		/* u8 */
				len = 1u;
				break;
			case 6:		/* u16 */
				len = 2u;
				break;
			case 7:		/* u32 */
				len = 4u;
				break;
			case 8:		/* real32 */
				f = strtof(parVal, NULL);
				coNumMemcpy(pVal, &f, 4u, 1u);
				len = 4u;
				break;
			default:
				return(0u);
		}

		return(len);
	}

	return(0u);
}


/***************************************************************************/
/**
* \internal
*
* \brief prepareDCFentry - prepare DCF entry
*
* Write special configuration for some objects like COB-IDs
* - disable PDO
* - disable Mapping
* - disable COB-Id entry
* before real value can be written
*
* \return RET_OK
* 	if DCF entry can be written
* 
*/
static RET_T prepareDcfEntry(
		UNSIGNED8	*pConsBuf,			/* concise buffer */
		UNSIGNED32	*pConsBufLen,		/* concise buffer len */
		UNSIGNED16	idx,				/* index */
		UNSIGNED8	subIdx	 			/* subindex */
	)
{
RET_T	retVal;

	/* special handling for COB-Ids and PDOs */

	/* EMCY */
	if (idx == 0x1014)  {
		/* disables cob */
		retVal = saveDcfEntry(pConsBuf, pConsBufLen, idx, subIdx,
				0x80000000u, 4u);
		if (retVal != RET_OK)  {
			return(retVal);
		}
	}

	/* SYNC */
	/* TIME */

	/* SDOs */
	if (((idx >= 0x1200u) && (idx <= 0x12ffu))
	 && (subIdx != 3u))  {
		/* disables cob */
		retVal = saveDcfEntry(pConsBuf, pConsBufLen, idx, subIdx, 0x80000000u, 4u);
		if (retVal != RET_OK)  {
			return(retVal);
		}
	}

	/* PDO comm para */
	if (((idx >= 0x1400u) && (idx <= 0x15ffu))
	 || ((idx >= 0x1800u) && (idx <= 0x19ffu)))  {
		/* disable PDO */
		retVal = disablePdo(pConsBuf, pConsBufLen, idx);
		if (retVal != RET_OK)  {
			return(retVal);
		}

		/* PDO COB-Id ? */
		if (subIdx == 1u)  {
			return(RET_COB_DISABLED);
		}
	}

	/* PDO Mappings */
	if (((idx >= 0x1600u) && (idx <= 0x17ffu))
	 || ((idx >= 0x1a00u) && (idx <= 0x1bffu)))  {
		retVal = disablePdo(pConsBuf, pConsBufLen, idx);
		if (retVal != RET_OK)  {
			return(retVal);
		}
		/* disable mapping */
		retVal = disableMapping(pConsBuf, pConsBufLen, idx);
		if (retVal != RET_OK)  {
			return(retVal);
		}

		/* mapping count ? */
		if (subIdx == 0u)  {
			return(RET_COB_DISABLED);
		}
	}

	return(RET_OK);
}


/***************************************************************************/
/**
* \internal
*
* \brief saveMappingDcfEntry - save mapping entry
*
* Save mapping cnt entry at concise DCF buffer
*
* \return RET_T
* \retval RET_OK
*	no mapping count 
*	mapping count ok
* \retval RET_ERROR
* 	write DCF entry error
*
*/
static RET_T saveMappingDcfEntry(
		UNSIGNED8	*pConsBuf,		/* DCF buffer */
		UNSIGNED32	*pConsBufLen,	/* DCF buffer len */
		UNSIGNED16	idx,			/* index */
		UNSIGNED8	subIdx,			/* subindex */
		UNSIGNED32	val,			/* value */
		UNSIGNED32	len				/* value len */
	)
{
UNSIGNED8	pdoBit;
UNSIGNED16	pdoIdx;
RET_T		retVal = RET_OK;

	/* only mapentries allowed */
	if ((idx < 0x1600u) || (idx > 0x1bffu))  {
		return(RET_OK);
	}
	if ((idx > 0x17ffu) && (idx < 0x1a00u))  {
		return(RET_OK);
	}
	/* and subindex 0 */
	if (subIdx != 0u)  {
		return(RET_OK);
	}

	pdoIdx = (idx & 0x1ffu) >> 3u;
	pdoBit = 1u << (idx & 0x7u);

	/* mapping disabled ? */
	if (idx >= 0x1a00u)  {
		/* tpdo disabled ? */
		if ((tMapDisabled[pdoIdx] & pdoBit) != 0u)  {
			retVal = saveDcfEntry(pConsBuf, pConsBufLen, idx, 0u, val, len);
		}
	} else {
		/* rpdo disabled ? */
		if ((rMapDisabled[pdoIdx] & pdoBit) != 0u)  {
			retVal = saveDcfEntry(pConsBuf, pConsBufLen, idx, 0u, val, len);
		}
	}

	return(retVal);
}


/***************************************************************************/
/**
* \internal
*
* \brief savePDODcfEntry - save PDO Cob-id entry
*
* Save PDO COB-ID entry at concise DCF buffer
*
* \return RET_T
* \retval RET_OK
*	no PDO Cob
*	PDO Cob ok
* \retval RET_ERROR
* 	write DCF entry error
*
*/
static RET_T savePdoDcfEntry(
		UNSIGNED8	*pConsBuf,		/* DCF buffer */
		UNSIGNED32	*pConsBufLen,	/* DCF buffer len */
		UNSIGNED16	idx,			/* index */
		UNSIGNED8	subIdx,			/* subindex */
		UNSIGNED32	val,			/* value */
		UNSIGNED32	len				/* value len */
	)
{
UNSIGNED8	pdoBit;
UNSIGNED16	pdoIdx;
RET_T		retVal = RET_OK;

	/* only mapentries allowed */
	if ((idx < 0x1400u) || (idx > 0x19ffu))  {
		return(RET_OK);
	}
	if ((idx > 0x15ffu) && (idx < 0x1800u))  {
		return(RET_OK);
	}
	/* and subindex 0 */
	if (subIdx != 1u)  {
		return(RET_OK);
	}

	pdoIdx = (idx & 0x1ffu) >> 3;
	pdoBit = 1u << (idx & 0x7u);

	/* no value available, use default */
	if (len == 0u)  {
		return(RET_OK);
	}



	/* mapping disabled ? */
	if (idx >= 0x1800u)  {
		/* tpdo disabled ? */
		if ((tPdoDisabled[pdoIdx] & pdoBit) != 0u)  {
			retVal = saveDcfEntry(pConsBuf, pConsBufLen, idx, 1u, val, len);
			tPdoDisabled[pdoIdx] &= ~pdoBit;
		}
	} else {
		/* rpdo disabled ? */
		if ((rPdoDisabled[pdoIdx] & pdoBit) != 0u)  {
			retVal = saveDcfEntry(pConsBuf, pConsBufLen, idx, 1u, val, len);
			rPdoDisabled[pdoIdx] &= ~pdoBit;
		}
	}

	return(retVal);
}


/***************************************************************************/
/**
* \internal
*
* \brief disablePdo - disable PDO
*
* If PDO is not disabled, save disable value at DCF buffer
* Save disabled PDO at internal variable
*
* \return RET_T
*
*/
static RET_T disablePdo(
		UNSIGNED8	*pConsBuf,		/* concise buffer */
		UNSIGNED32	*pConsBufLen,	/* buffer len */
		UNSIGNED16	idx				/* index */
	)
{
UNSIGNED8	pdoBit;
UNSIGNED16	pdoIdx;
RET_T		retVal = RET_OK;

	pdoIdx = (idx & 0x1ffu) >> 3;
	pdoBit = 1u << (idx & 0x7u);

	/* PDO already disabled ? */
	if (idx >= 0x1800u)  {
		/* tpdo disabled ? */
		if ((tPdoDisabled[pdoIdx] & pdoBit) == 0u)  {
			retVal = saveDcfEntry(pConsBuf, pConsBufLen,
				idx & 0x19ffu, 1u, 0x80000000u, 4u);
			tPdoDisabled[pdoIdx] |= pdoBit;
		}
	} else {
		/* rpdo disabled ? */
		if ((rPdoDisabled[pdoIdx] & pdoBit) == 0u)  {
			retVal = saveDcfEntry(pConsBuf, pConsBufLen,
				idx & 0x15ffu, 1u, 0x80000000u, 4u);
			rPdoDisabled[pdoIdx] |= pdoBit;
		}
	}

	return(retVal);
}


/***************************************************************************/
/**
* \internal
*
* \brief disableMapping - disable Mapping
*
* If Mapping is not disabled, save disable value at DCF buffer
* Save disabled Mapping at internal variable
*
* \return RET_T
*
*/
static RET_T disableMapping(
		UNSIGNED8	*pConsBuf,		/* concise buffer */
		UNSIGNED32	*pConsBufLen,	/* buffer len */
		UNSIGNED16	idx				/* index */
	)
{
UNSIGNED8	pdoBit;
UNSIGNED16	pdoIdx;
RET_T		retVal = RET_OK;

	pdoIdx = (idx & 0x1ffu) >> 3u;
	pdoBit = 1u << (idx & 0x7u);

	/* PDO already disabled ? */
	if (idx >= 0x1a00u)  {
		/* tpdo disabled ? */
		if ((tMapDisabled[pdoIdx] & pdoBit) == 0u)  {
			retVal = saveDcfEntry(pConsBuf, pConsBufLen,
				idx & 0x1bffu, 0u, 0x0u, 1u);
			tMapDisabled[pdoIdx] |= pdoBit;
		}
	} else {
		/* rpdo disabled ? */
		if ((rMapDisabled[pdoIdx] & pdoBit) == 0u)  {
			retVal = saveDcfEntry(pConsBuf, pConsBufLen,
				idx & 0x17ffu, 0u, 0x0u, 1u);
			rMapDisabled[pdoIdx] |= pdoBit;
		}
	}

	return(retVal);
}


/***************************************************************************/
/**
* \internal
*
* \brief saveDcfEntry - save DCF entry at buffer
*
* Save DCF entry at concise DCF buffer
*
* \return RET_T
*
*/
static RET_T saveDcfEntry(
		UNSIGNED8	*pConsBuf,			/* concise buffer */
		UNSIGNED32	*pConsBufLen,		/* buffer len */
		UNSIGNED16	idx,				/* index */
		UNSIGNED8	subIdx,				/* subIndex */
		UNSIGNED32	val,				/* value */
		UNSIGNED32	len					/* value len */
	)
{
	/* check for buffer size */
	if ((*pConsBufLen + 7u + len) > maxConsBuf)  {
		return(RET_OUT_OF_MEMORY);
	}

#ifdef xxx
printf("saveDcfEntry: %x:%d (%ld) %x %x %x %x\n", idx, subIdx, len,
		(UNSIGNED8)(val) & 0xff,
		(UNSIGNED8)(val >> 8) & 0xff,
		(UNSIGNED8)(val >> 16) & 0xff,
		(UNSIGNED8)(val >> 24) & 0xff);
#endif /* xxx */
	/* save idx, subidx */
	memcpy(&pConsBuf[*pConsBufLen], &idx, 2u);
	*pConsBufLen += 2u;
	memcpy(&pConsBuf[*pConsBufLen], &subIdx, 1u);
	*pConsBufLen += 1u;
	memcpy(&pConsBuf[*pConsBufLen], &len, 4u);
	*pConsBufLen += 4u;
	memcpy(&pConsBuf[*pConsBufLen], &val, len);
	*pConsBufLen += len;

	nrOfEntries++;

	return(RET_OK);
}


/***************************************************************************/
/**
* \internal
*
* \brief getKeyString - return value of key string
*
* get value for the given key string
* and copy it to pDst buffer
*
* \return RET_T
*
*/
static RET_T getKeyString(
		CO_CONST CHAR	*pDataStart,/* start buffer addr */
		CO_CONST CHAR	*pDataEnd,	/* end buffer addr */
		CO_CONST CHAR	*keyStrg,	/* key string looking for */
		CHAR	*pDst,				/* start result buffer */ 
		UNSIGNED8	maxLen			/* len of result buffer */
	)
{
CHAR	*pStrg;
CHAR	*pLast;
UNSIGNED8	len;

	/* search key */
	pStrg = strstr(pDataStart, keyStrg);
	if ((pStrg == NULL) || (pStrg > pDataEnd))  {
		return(RET_CFG_CONVERT_ERROR);
	}

	/* search "=" */
	pStrg = strchr(pStrg, '=');
	if (pStrg == NULL)  {
		return(RET_CFG_CONVERT_ERROR);
	}
	pStrg++;

	/* search end of line */
	pLast = strchr(pStrg, '\n');
	if ((pLast == NULL) || (pLast > pDataEnd))  {
		return(RET_CFG_CONVERT_ERROR);
	}

	/* search start of value */
	len = (UNSIGNED8)(pLast - pStrg);
	/* ignore possible \r */
	if (pStrg[len - 1] == '\r')  {
		len -= 1u;
	}
	while (len != 0u)  {
		if (isspace(pStrg[0u]))  {
			pStrg++;
		} else {
			break;
		}
		len--;
	}

	if ((len == 0u) || (len > maxLen)) {
		return(RET_CFG_CONVERT_ERROR);
	}

	strncpy(pDst, pStrg, (size_t)len);
	pDst[len] = 0u;

	return(RET_OK);
}
#endif /* CO_CONCISE_CONVERT */


/***************************************************************************/
/**
* \internal
*
* \brief cfgIndication - user indication
*
* call user indication if registered
*
* \return data len
*
*/
static void cfgIndication(
		CO_CFG_TRANSFER_T	type,		/* result type */
		UNSIGNED8			sdoNr,		/* sdo nr */
		UNSIGNED16			index,		/* index */
		UNSIGNED8			subIndex,	/* subindex */
		UNSIGNED32			reason		/* error reason */
	)
{
CFG_SDO_T	*pCfg;
#ifdef CO_EVENT_CFG_MAN_CNT
UNSIGNED16	i;
#endif /* CO_EVENT_CFG_MAN_CNT */

	pCfg = getSdoCfg(sdoNr);
	pCfg->busy = 0u;

#ifdef CO_EVENT_CFG_MAN_CNT
	for (i = 0; i < cfgManEventTableCnt; i++)  {
		cfgManEventTable[i](type, sdoNr, index, subIndex, reason);
	}
#endif /* CO_EVENT_CFG_MAN_CNT */

#ifdef CO_EVENT_STATIC_CFG_MAN
	coEventCfgManInd(type, sdoNr, index, subIndex, reason);
#endif /* CO_EVENT_STATIC_CFG_MAN */

}


/***************************************************************************/
/**
* \internal
*
* \brief getSdoCfg - get sdo config structure
*
* Look for sdo client structure. If not yet reservered, use a new one
*
* \return pointer to cfg struct
*
*/
static CFG_SDO_T *getSdoCfg(
		UNSIGNED8	sdoClientNr			/* use sdo number */
	)
{
CFG_SDO_T *pSdoCfg = NULL;
	pSdoCfg = &sdoCfg[sdoClientNr - 1];

	return(pSdoCfg);
}


/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
# ifdef CO_EVENT_CFG_MAN_CNT
/***************************************************************************/
/**
* \brief coEventRegister_CFG_MAN - register CFG_MAN event
*
* This function registers an indication function for CFG_MAN events.
* The indication function is called after transfer to slave has been finished
*
* \return RET_T
*
*/

RET_T coEventRegister_CFG_MANAGER(
		CO_EVENT_CFG_MANAGER_T pFunction       /**< pointer to function */
    )
{
	if (cfgManEventTableCnt >= CO_EVENT_CFG_MAN_CNT) {
		return(RET_EVENT_NO_RESSOURCE);
	}

	/* set new indication function as first at the list */
	cfgManEventTable[cfgManEventTableCnt] = pFunction;
	cfgManEventTableCnt++;

	return(RET_OK);
}
# endif /* CO_EVENT_CFG_MAN_CNT */
#endif /* CO_CFG_MANAGER */
