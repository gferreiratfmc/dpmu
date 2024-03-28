/*
* co_edsparse.c - contains parse rouines for eds files
*
* Copyright (c) 2016-2022 emotas embedded communication GmbH
*-------------------------------------------------------------------
* $Id: co_edsparse.c 41218 2022-07-12 10:35:19Z boe $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \brief EDS parser module
*
* \file co_edsparse.c
* contains EDS parse routines
*
*/

/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <gen_define.h>

#include <co_datatype.h>
#include <co_edsparse.h>
#include <co_odaccess.h>
#include <co_sdo.h>

/* constant definitions
---------------------------------------------------------------------------*/
#define MAX_LINE_SIZE	120		/* max buffer for eds lines */
#define MAX_PARAM_SIZE	50u		/* max parameter string size */
#define MAX_EDS_FILENAME_LEN	100u

#ifndef MAX_PDO_MAP_TABLES
# define MAX_PDO_MAP_TABLES	512u
#endif /* MAX_PDO_MAP_TABLES */

#ifndef MAX_REPO_EDS_FILES
# define MAX_REPO_EDS_FILES	10u		/* max number of EDS files */
#endif /* MAX_REPO_EDS_FILES */

#ifndef CO_MAX_DETECT_EDS_SLAVES
# ifdef CO_SDO_CLIENT_CNT
#   define CO_MAX_DETECT_EDS_SLAVES	CO_SDO_CLIENT_CNT
# else
#   define CO_MAX_DETECT_EDS_SLAVES	127u
# endif /* CO_SDO_CLIENT_CNT */
#endif /* CO_MAX_DETECT_EDS_SLAVES */

typedef char	CHAR;		/* for MISRA check */
typedef int		INTEGER;

/* local defined data types
---------------------------------------------------------------------------*/
typedef struct {
	CHAR	*name;					/* parameter name */
	CHAR	result[MAX_PARAM_SIZE];	/* parameter result */
} EDS_PAIR_T;

typedef struct {
	UNSIGNED32	identity[4];		/* identity entries */
	CHAR		edsFileName[MAX_EDS_FILENAME_LEN];
} EDS_REPO_T;

typedef struct {
	CO_DETECT_SLAVE_FCT_T	finishFct;	/* function to call after eds was assigned */
	UNSIGNED32	identity[4];		/* identity entries */
	UNSIGNED8	state;				/* working state */
	UNSIGNED8	sdoNr;				/* used SDO number */
	UNSIGNED8	nodeId;				/* remote node id */
} DETECT_SLAVE_DATA_T;

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
static RET_T saveRPdoMapping(UNSIGNED8 nodeId, UNSIGNED16 pdoNr,
		UNSIGNED8 nrOfSubs, UNSIGNED8 nrOfMappedObj,
		const CO_EDS_MAP_ENTRY_T *mapEntry, BOOL_T writable);
static RET_T saveTPdoMapping(UNSIGNED8 nodeId, UNSIGNED16 pdoNr,
		UNSIGNED8 nrOfSubs, UNSIGNED8 nrOfMappedObj,
		const CO_EDS_MAP_ENTRY_T *mapEntry, BOOL_T writable);
static CO_EDS_MAP_TABLE_T *findMapEntry(CO_EDS_MAP_TABLE_T	*mapTable,
		UNSIGNED8 nodeId, UNSIGNED16 pdoNr);
static const CHAR *edsParseSection(CHAR *pEds, const CHAR *pSection);
static INTEGER8 edsParseParameter(CHAR *pEds, EDS_PAIR_T *edsPairList,
		UNSIGNED8 edsPairCnt);
static void sdoClientInd(void *pData, UNSIGNED32 result);
static BOOL_T edsParseFile(const CHAR *edsFileName, const CHAR *section,
		EDS_PAIR_T *edsPairList, UNSIGNED8 edsPairCnt);
static const CHAR *strstrlower(const CHAR *strg, const CHAR *needle);
static const CHAR *findStrg( const CHAR *strg, const CHAR *needle);

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
static EDS_REPO_T	edsRepo[MAX_REPO_EDS_FILES];
static UNSIGNED16	edsFileCnt = 0u;
static CO_EDS_MAP_TABLE_T	tPdoMapTable[MAX_PDO_MAP_TABLES];
static CO_EDS_MAP_TABLE_T	rPdoMapTable[MAX_PDO_MAP_TABLES];
static DETECT_SLAVE_DATA_T	detectSlaveData[CO_MAX_DETECT_EDS_SLAVES];



/***************************************************************************/
/**
* \brief coEdsparseAddEdsToRepository - add file to eds repository
*
* This function add an EDS file to the internal repository
* and parse it for identity data
*
* \return RET_T
*
*/
RET_T coEdsparseAddEdsToRepository(
		const CHAR	*edsFilePath		/**< eds file name */
	)
{
EDS_PAIR_T	edsPairList[2];
UNSIGNED8	i;
CHAR		section[20];
BOOL_T		found;

	/* internal ressource available ? */
	if (edsFileCnt >= MAX_REPO_EDS_FILES)  {
		return(RET_EVENT_NO_RESSOURCE);
	}

	/* copy filename to our repository */
	strncpy(edsRepo[edsFileCnt].edsFileName, edsFilePath, MAX_EDS_FILENAME_LEN);

	/* get identity values from EDS file */
	for (i = 0u; i < 4u; i++)  {
		edsPairList[0].name = "DefaultValue";
		edsPairList[1].name = "ParameterValue";
		sprintf(section, "1018sub%x", i + 1u);
		found = edsParseFile(edsFilePath, section, &edsPairList[0], 2u);
		/* if found, save it at our repository */
		if (found == CO_TRUE)  {
			if (strlen(edsPairList[1].result) != 0u)  {
				edsRepo[edsFileCnt].identity[i] = (UNSIGNED32)strtol(edsPairList[1].result,
					NULL, 0);
			} else {
				edsRepo[edsFileCnt].identity[i] = (UNSIGNED32)strtol(edsPairList[0].result,
					NULL, 0);
			}
		}
	}

	edsFileCnt++;

	return(RET_OK);
}


/***************************************************************************/
/**
* \brief detectSlaveEds - detect slave EDS file
*
* This function read the identity from the given slave
* and checks it by available identity parameter from EDS repository.
* If it fit the identity from the device and the EDS
* given finishFct returns the fitting EDS file name.
*
* If an error occurs, the finishFct returns without EDS file name
* but with the appropriate error.
*
* \return RET_T
*
*/
RET_T coEdsparseDetectSlaveEds(
		UNSIGNED8		nodeId,			/**< node id */
		UNSIGNED8		sdoClientNr,	/**< SDO client number */
		CO_DETECT_SLAVE_FCT_T	finishFct	/**< function for finish action */
	)
{
RET_T	retVal;
DETECT_SLAVE_DATA_T	*pSlaveData = NULL;
UNSIGNED16	i;

	/* look for unused structure */
	for (i = 0u; i < CO_MAX_DETECT_EDS_SLAVES; i++)  {
		if (detectSlaveData[i].state == 0u)  {
			pSlaveData = &detectSlaveData[i];
			break;
		}
	}

	/* free structure found ? */
	if (pSlaveData == NULL)  {
		return(RET_SERVICE_BUSY);
	}

	/* save given data */
	pSlaveData->finishFct = finishFct;
	pSlaveData->sdoNr = sdoClientNr;
	pSlaveData->nodeId = nodeId;

	/* setup cobid for SDO */
	retVal = coOdSetCobid((0x1280u + sdoClientNr) - 1u, 1u,
		(UNSIGNED32)0x600u + nodeId);
	if (retVal != RET_OK)  {
		/* error init sdo cob */
		return(retVal);
	}
	retVal = coOdSetCobid((0x1280u + sdoClientNr) - 1u, 2u,
		(UNSIGNED32)0x580u + nodeId);
	if (retVal != RET_OK)  {
		/* error init sdo cob */
		return(retVal);
	}

	/* start first sdo transfer */
	sdoClientInd(pSlaveData, 0ul);

	return(RET_OK);
}


/***************************************************************************/
/**
* \internal
*
* \brief sdoClientInd - client indication
*
* Called to start new sdo transfer to get identity parameter
* After subindex 4 was requested, check sub 1..3 with internal database
* If it fit, return eds file name
* If an error occured, return error code
*
*
* \return RET_T
*
*/
static void sdoClientInd(
		void		*pData,
		UNSIGNED32	result
	)
{
DETECT_SLAVE_DATA_T	*pSlaveData = (DETECT_SLAVE_DATA_T *)pData;
RET_T	retVal;
UNSIGNED8	i;

	/* only subindex 1..3 are relevant, but we get also sub 4 ...*/
	if (pSlaveData->state < 4u)  {
		/* if error, nothing to do */
		if (result != 0ul)  {
			/* error */
			if (pSlaveData->finishFct != NULL)  {
				pSlaveData->finishFct(pSlaveData->nodeId, NULL);
			}

			pSlaveData->state = 0u;
			return;
		}

		/* save data at OD */
		if (pSlaveData->state != 0u)  {
			(void)coOdPutObj_u32(0x1f84u + pSlaveData->state,
				pSlaveData->nodeId, 
				pSlaveData->identity[pSlaveData->state - 1u]);
		}

		/* start next transfer */
		retVal = coSdoQueueAddTransfer(CO_FALSE, pSlaveData->sdoNr,
			0x1018u, pSlaveData->state + 1u,
			(UNSIGNED8 *)&pSlaveData->identity[pSlaveData->state], 4u,
			sdoClientInd, pSlaveData, 1000u);
		if (retVal != RET_OK)  {
			if (pSlaveData->finishFct != NULL)  {
				pSlaveData->finishFct(pSlaveData->nodeId, NULL);
			}
			pSlaveData->state = 0u;
			return;
		}

		pSlaveData->state ++;

	} else {
		/* if no error, save at OD */
		if (result == 0ul)  {
			(void)coOdPutObj_u32(0x1f87u, pSlaveData->nodeId, 
				pSlaveData->identity[pSlaveData->state - 1u]);
		}

		/* all data are received, start dectection of EDS */
		for (i = 0u; i < edsFileCnt; i++)  {
/*
 printf("%d ?= %d, %d ?= %d, %d ?= %d\n",  
			pSlaveData->identity[0], edsRepo[i].identity[0],
			pSlaveData->identity[1], edsRepo[i].identity[1],
			pSlaveData->identity[2], edsRepo[i].identity[2]);
*/

			if ((pSlaveData->identity[0] == edsRepo[i].identity[0])
			 && (pSlaveData->identity[1] == edsRepo[i].identity[1])
			 && (pSlaveData->identity[2] == edsRepo[i].identity[2]))  {
/*				printf("eds file for node %d is %s\n", pSlaveData->nodeId, edsRepo[i].edsFileName); */

				/* call finish function */
				if (pSlaveData->finishFct != NULL)  {
					pSlaveData->finishFct(pSlaveData->nodeId,
						edsRepo[i].edsFileName);
					break;
				}
			}
		}
		/* no eds found? */
		if (i == edsFileCnt)  {
			pSlaveData->finishFct(pSlaveData->nodeId, NULL);
		}

		pSlaveData->state = 0u;
	}
}


/***************************************************************************/
/**
* \brief coEdsparseReadEdsMapping - read mapping from EDS file
*
* This function read the EDS file and save the values
* at the internal mapping tables.
*
* \return RET_T
*
*/
RET_T coEdsparseReadEdsMapping(
		UNSIGNED8		nodeId,			/**< node id */
		const CHAR		*edsFileName	/**< eds file name */
	)
{
EDS_PAIR_T	edsPairList[4];
BOOL_T		found;
UNSIGNED16	i;
UNSIGNED16	index;
UNSIGNED8	sub;
CHAR		strg[20];
UNSIGNED16	nrOfObjects;
UNSIGNED8	nrOfSubs;
CO_EDS_MAP_ENTRY_T	mapEntry[8];
UNSIGNED8	mapCnt = 0u;
BOOL_T		mapWrite = CO_FALSE;
UNSIGNED32	mapVal;
RET_T		ret = RET_OK;

	/* get number of optional objects */
	edsPairList[0].name = "SupportedObjects";
	found = edsParseFile(edsFileName, "OptionalObjects", &edsPairList[0], 1u);
	if (found != CO_TRUE)  {
		/*printf("Error get supported objects - abort\n"); */
		return(RET_EVENT_NO_RESSOURCE);
	}
	nrOfObjects = (UNSIGNED16)atoi(edsPairList[0].result);

	/* for all supported objects */
	for (i = 1u; i <= nrOfObjects; i++)  {
		/* get index */
		sprintf(strg, "%d", i);
		edsPairList[0].name = strg;
		found = edsParseFile(edsFileName, "OptionalObjects", &edsPairList[0], 1u);
		if (found != CO_TRUE)  {
			/* printf("Error get objects entry %d - abort\n", i); */
			return(RET_IDX_NOT_FOUND);
		}

		/* check if it a PDO entry */
		index = (UNSIGNED16)strtol(edsPairList[0].result, NULL, 0);

		/* RDPO mapping ? */
		if (((index >= 0x1600u) && (index <= 0x17ffu))
		 || ((index >= 0x1a00u) && (index <= 0x1bffu)))  {
			/* get number of subs */
			edsPairList[0].name = "SubNumber";
			sprintf(strg, "%x", index);
			found = edsParseFile(edsFileName, strg, &edsPairList[0], 1u);
			if (found != CO_TRUE)  {
				/* printf("Error get number of subs index %x - abort\n", index); */
				return(RET_SUBIDX_NOT_FOUND);
			}
			nrOfSubs = (UNSIGNED8)atoi(edsPairList[0].result);
			/* for all subs */
			for (sub = 0u; sub < nrOfSubs; sub++)  {
				/* get infos from subindex */
				edsPairList[0].name = "AccessType";
				edsPairList[1].name = "DefaultValue";
				edsPairList[2].name = "DataType";
				edsPairList[3].name = "ParameterValue";
				sprintf(strg, "%xsub%x", index, sub);
				found = edsParseFile(edsFileName, strg, &edsPairList[0], 4u);
				if (found != CO_TRUE)  {
					/* printf("Error get subs %d - abort\n", sub); */
					return(RET_SUBIDX_NOT_FOUND);
				}

				/* printf("%x:%d: %s %s\n", index, sub, edsPairList[0].result, edsPairList[1].result); */

				if (sub == 0u)  {
					/* Parametervalue available ? */
					if (strlen(edsPairList[3].result) != 0u)  {
						mapCnt = (UNSIGNED8)strtol(edsPairList[3].result, NULL, 0);
					} else {
						mapCnt = (UNSIGNED8)strtol(edsPairList[1].result, NULL, 0);
					}
					if (strcmp(edsPairList[0].result, "rw") == 0)  {
						mapWrite = CO_TRUE;
					} else {
						mapWrite = CO_FALSE;
					}
				} else {
					/* Parametervalue available ? */
					if (strlen(edsPairList[3].result) != 0u)  {
						mapVal = (UNSIGNED32)strtol(edsPairList[3].result, NULL, 0);
					} else {
						mapVal = (UNSIGNED32)strtol(edsPairList[1].result, NULL, 0);
					}
					mapEntry[sub - 1u].index = (UNSIGNED16)(mapVal >> 16);
					mapEntry[sub - 1u].subIndex = (UNSIGNED8)((mapVal >> 8) & 0xffu);

					/* get datatype for mapped object */
					edsPairList[0].name = "DataType";
					sprintf(strg, "%xsub%x", mapEntry[sub - 1u].index,
							mapEntry[sub - 1u].subIndex);
					found = edsParseFile(edsFileName, strg, &edsPairList[0], 1u);
					if (found != CO_TRUE)  {
						/* printf("Error get datatype %s - try without sub\n", strg); */
						sprintf(strg, "%x", mapEntry[sub - 1u].index);
						found = edsParseFile(edsFileName, strg, &edsPairList[0], 1u);
						if (found != CO_TRUE)  {
							/* printf("Error get datatype %s - abort\n", strg); */
							return(RET_EVENT_NO_RESSOURCE);
						}
					}

					mapEntry[sub - 1u].dataType = (UNSIGNED8)strtol(edsPairList[0].result, NULL, 0);
				}
			}

			if (index < 0x1800u)  {
				ret = saveRPdoMapping(nodeId, (index & 0x1ffu) + 1u,
					nrOfSubs, mapCnt, &mapEntry[0], mapWrite);
			} else {
				ret = saveTPdoMapping(nodeId, (index & 0x1ffu) + 1u,
					nrOfSubs, mapCnt, &mapEntry[0], mapWrite);
			}
			if (ret != RET_OK)  {
				/* printf("save Mapping failed...\n"); */
				return(ret);
			}
		}
	}

	return(ret);
}


/***************************************************************************/
/**
* \brief coEdsparseGetRPdoMapEntry - get RPDO map entry from EDS table
*
* This function returns a RPDO map entry from the EDS table
*
*
* \return RET_T
*
*/
CO_EDS_MAP_TABLE_T *coEdsparseGetRPdoMapEntry(
		UNSIGNED16 mapIdx				/**< map index at table */
	)
{
	/* outside table, return */
	if (mapIdx >= MAX_PDO_MAP_TABLES)  {
		return(NULL);
	}

	return(&rPdoMapTable[mapIdx]);
}


/***************************************************************************/
/**
* \brief coEdsparseGetTPdoMapEntry - get TPDO map entry from EDS table
*
* This function returns a TPDO map entry from the EDS table
*
* \return RET_T
*
*/
CO_EDS_MAP_TABLE_T *coEdsparseGetTPdoMapEntry(
		UNSIGNED16 mapIdx				/**< map index at table */
	)
{
	/* outside table, return */
	if (mapIdx >= MAX_PDO_MAP_TABLES)  {
		return(NULL);
	}

	return(&tPdoMapTable[mapIdx]);
}


/***************************************************************************/
/**
* \internal
*
* \brief edsParseFile - parse eds file for defined number of parameter
*
* parse EDS file for number of edsPairCnt parameter
* all requested parameter are saved at edsPairList, result is also
* save there
*
* \return BOOL_T
* \retval CO_TRUE
* 	parameter found and result saved
* \retval CO_FALSE
* 	no parameter found
*
*/
static BOOL_T edsParseFile(
		const CHAR	*edsFileName,	/* eds file name */
		const CHAR	*section,		/* section */
		EDS_PAIR_T	*edsPairList,	/* eds pair list */
		UNSIGNED8	edsPairCnt		/* number of eds pairs */
	)
{
FILE	*fp;
CHAR	buffer[MAX_LINE_SIZE];
const CHAR	*pSection = NULL;
INTEGER8	result = 0;
BOOL_T	found = CO_FALSE;
UNSIGNED8	i;

	/* reset pair values */
	for (i = 0u; i < edsPairCnt; i++)  {
		edsPairList[i].result[0] = '\0';
	}

	fp = fopen(edsFileName, "r");
	if (fp == NULL)  {
		return(CO_FALSE);
	}

	/* read one line */
	while (fgets(&buffer[0], MAX_LINE_SIZE, fp) != NULL)  {
		/* delete CR/LF */
		buffer[strcspn(buffer, "\r\n")] = '\0';
		if (pSection == NULL)  {
			/* look for section */
			pSection = edsParseSection(buffer, section);
		} else {
			/* now look for parameter */
			result = edsParseParameter(buffer, edsPairList, edsPairCnt);
			if (result < 0)  {
				/* finished */
				break;
			} else {
				if (result > 0)  {
					found = CO_TRUE;
				}
			}
		}
	}
	fclose(fp);

	return(found);
}


/***************************************************************************/
/**
* \internal
*
* \brief edsParseSection - parse section from eds file
*
* look for section name in eds file
* function get a line from the eds file and looked for the section name
* in [ and ] part
*
* \return
* 	pointer to section string
*
*/
static const CHAR *edsParseSection(
		CHAR	*pEds,			/* pointer to EDS data */
		const CHAR	*pSection		/* pointer to section name */
	)
{
CHAR	*pBrL, *pBrR;

	/* for all given data at EDS buffer */
	while (*pEds != '\0')  {

		/* search for first [ */
		pBrL = strchr(pEds, '[');
		if (pBrL == NULL)  {
			break;
		}
		/* search for ] */
		pBrR = strchr(pBrL, ']');
		if (pBrR == NULL)  {
			break;
		}

		if (strstrlower(pBrL, pSection) != NULL)  {
			return(pSection);
		}

		pEds++;
	}

	return(NULL);
}


/***************************************************************************/
/**
* \internal
*
* \brief edsParseParameter - parse eds string for parameter
*
* parse EDS line for number of edsPairCnt parameter
* all requested parameter are saved at edsPairList, result is also
* save there
*
* \return
* 	-1	nothing found (no [ section ] found
*	0	parameter not found
*	1	parameter found, result saved
*
*/
static INTEGER8 edsParseParameter(
		CHAR	*pEds,			/* pointer to EDS data */
		EDS_PAIR_T	*edsPairList,	/* eds pair list */
		UNSIGNED8	edsPairCnt		/* number of eds pairs */
	)
{
CHAR	*pBrL;
CHAR	*pSrc;
UNSIGNED8	i;

	/* search for first [ */
	pBrL = strchr(pEds, '[');
	if ((pBrL != NULL) && (pBrL == pEds)) {
		return(-1);
	}

	for (i = 0u; i < edsPairCnt; i++)  {
		if (strstrlower(pEds, edsPairList[i].name) == pEds)  {
			if (pEds[strlen(edsPairList[i].name)] == '=')  {
				pSrc = strchr(pEds, '=');
				if (pSrc != NULL)  {
					pSrc += 1;
					strncpy(edsPairList[i].result, pSrc, MAX_PARAM_SIZE);
				}
				return(1);
			}
		}
	}

	return(0);
}


/***************************************************************************/
/**
* \internal
*
* \brief saveRPdoMapping - save RPDO map entry at mapping table
*
* Save mapping entries at global RPDO mapping table
*
* \return RET_T
*
*/
static RET_T saveRPdoMapping(
		UNSIGNED8	nodeId,				/* node id */
		UNSIGNED16	pdoNr,				/* pdo number */
		UNSIGNED8	nrOfSubs,			/* number of subindex */
		UNSIGNED8	nrOfMappedObj,		/* number of mapped objects */
		const CO_EDS_MAP_ENTRY_T	*mapEntry,	/* mapping entries */
		BOOL_T		writable			/* dynamic/static mapping */
	)
{
CO_EDS_MAP_TABLE_T	*pMap;

	/* find free entry at global mapping table */
	pMap = findMapEntry(&rPdoMapTable[0], nodeId, pdoNr);
	if (pMap == NULL)  {
		/* printf("kein freier Mappingeintrag\n"); */
		return(RET_EVENT_NO_RESSOURCE);
	}

	/* save data */
	pMap->nodeId = nodeId;
	pMap->pdoNr = pdoNr;
	pMap->nrOfSubs = nrOfSubs;
	pMap->nrOfMappedObj = nrOfMappedObj;
	memset(&pMap->mapEntry[0], 0, sizeof(CO_EDS_MAP_ENTRY_T) * 8u);
	memcpy(&pMap->mapEntry[0], mapEntry, sizeof(CO_EDS_MAP_ENTRY_T) * nrOfSubs);
	pMap->writable = writable;

	return(RET_OK);
}


/***************************************************************************/
/**
* \internal
*
* \brief saveTPdoMapping - save TPDO map entry at mapping table
*
* Save mapping entries at global TPDO mapping table
*
* \return RET_T
*
*/
static RET_T saveTPdoMapping(
		UNSIGNED8	nodeId,				/* node id */
		UNSIGNED16	pdoNr,				/* pdo number */
		UNSIGNED8	nrOfSubs,			/* number of subindex */
		UNSIGNED8	nrOfMappedObj,		/* number of mapped objects */
		const CO_EDS_MAP_ENTRY_T	*mapEntry,	/* mapping entries */
		BOOL_T		writable			/* dynamic/static mapping */
	)
{
CO_EDS_MAP_TABLE_T	*pMap;

	/* find free entry at global mapping table */
	pMap = findMapEntry(&tPdoMapTable[0], nodeId, pdoNr);
	if (pMap == NULL)  {
		/* printf("kein freier Mappingeintrag\n"); */
		return(RET_EVENT_NO_RESSOURCE);
	}

	/* save data */
	pMap->nodeId = nodeId;
	pMap->pdoNr = pdoNr;
	pMap->nrOfSubs = nrOfSubs;
	pMap->nrOfMappedObj = nrOfMappedObj;
	memset(&pMap->mapEntry[0], 0, sizeof(CO_EDS_MAP_ENTRY_T) * 8u);
	memcpy(&pMap->mapEntry[0], mapEntry, sizeof(CO_EDS_MAP_ENTRY_T) * nrOfSubs);
	pMap->writable = writable;

	return(RET_OK);
}


/***************************************************************************/
/**
* \internal
*
* \brief findMapEntry - find map entry at global mapping table
*
* Look for mapping entry for given node id and pdo number
* if not found, look for free and return it
*
* \return pointer to mapping table entry
*
*/
static CO_EDS_MAP_TABLE_T *findMapEntry(
		CO_EDS_MAP_TABLE_T	*mapTable,
		UNSIGNED8	nodeId,
		UNSIGNED16	pdoNr
	)
{
UNSIGNED16	i;

	for (i = 0u; i < MAX_PDO_MAP_TABLES; i++)  {
		if (mapTable[i].nodeId == nodeId)  {
			if (mapTable[i].pdoNr == pdoNr)  {
				/* found */
				return(&mapTable[i]);
			}
		} else {
			/* empty entry ? */
			if (mapTable[i].nodeId == 0u)  {
				/* return empty entry */
				return(&mapTable[i]);
			}
		}
	}

	return(NULL);
}


/***************************************************************************/
/**
*
* \brief coEdsparseGetSupportedIndexCnt - return number of supported index
*
* This function counts the supported index for the given section
* and return the number of supported index
*
* section should be one of
* 	MandatoryObjects
*	OptionalObjects
*	ManufacturerObjects
*
* \return number of supported index
*
*/
UNSIGNED16 coEdsparseGetSupportedObjCnt(
		const CHAR	*edsFileName,	/**< eds file name */
		const CHAR	*section		/**< section name */
	)
{
EDS_PAIR_T	edsPairList[3];
BOOL_T		found;
UNSIGNED16	nrOfObjects;

	/* get number of optional objects */
	edsPairList[0].name = "SupportedObjects";
	found = edsParseFile(edsFileName, section, &edsPairList[0], 1u);
	if (found != CO_TRUE)  {
		/*printf("Error get supported objects - abort\n"); */
		return(0u);
	}
	/* convert to number */
	nrOfObjects = (UNSIGNED16)atoi(edsPairList[0].result);

	return(nrOfObjects);
}


/***************************************************************************/
/**
*
* \brief coEdsparseGetIndexDesc - return index description
*
* This function returns some information about the object index
* given by eds index. 
* The maximum number of eds index can get by function 
* coEdsparseGetSupportedObjCnt()
*
* section should be one of
* 	MandatoryObjects
*	OptionalObjects
*	ManufacturerObjects
*
* \return
*	RET_T
*/
RET_T coEdsparseGetIndexDesc(
		const CHAR	*edsFileName,	/**< eds file name */
		const CHAR	*pSection,	/**< section name */
		UNSIGNED16	edsIdx,		/**< index at eds file list */
		UNSIGNED16	*pIndex,	/**< object index */
		UNSIGNED8	*pNrOfSubs	/**< number of subindex */
	)
{
EDS_PAIR_T	edsPairList[1];
CHAR	strg[10];
BOOL_T	found;

	/* get index */
	sprintf(strg, "%d", (INTEGER16)edsIdx + 1);

	edsPairList[0].name = strg;
	found = edsParseFile(edsFileName, pSection, &edsPairList[0], 1u);
	if (found != CO_TRUE)  {
		/* printf("Error get objects entry %d - abort\n", i); */
		return(RET_IDX_NOT_FOUND);
	}

	/* convert index to number */
	*pIndex = (UNSIGNED16)strtol(edsPairList[0].result, NULL, 0);

	/* get object info from sub 0 */
	sprintf(strg, "%x", *pIndex);
	edsPairList[0].name = "SubNumber";
	found = edsParseFile(edsFileName, strg, &edsPairList[0], 1u);
	if (found == CO_TRUE)  {
		*pNrOfSubs = (UNSIGNED8)strtol(edsPairList[0].result, NULL, 0);
	} else {
		*pNrOfSubs = 1u;
	}

	return(RET_OK);
}


/***************************************************************************/
/**
*
* \brief coEdsparseGetObjectDesc - get object description
*
* This function returns object description from EDS
* for the given object index. 
*
* \returns
* 	RET_T
*
*/
RET_T coEdsparseGetObjectDesc(
		const CHAR	*edsFileName,	/**< eds file name */
		UNSIGNED16	index,			/**< object index */
		UNSIGNED8	subIndex,		/**< object subindex */
		UNSIGNED16	*pDataType,		/**< pointer for data type */
		UNSIGNED16	*pAttr,			/**< pointer for object attributes */
		CHAR		*pDefaultVal	/**< pointer for default val */
	)
{
CHAR	strg[10];
EDS_PAIR_T	edsPairList[5];
BOOL_T	found;

	/* get object info from sub n */

	sprintf(strg, "%x", index);
	
	/* simple object or array ? */
	edsPairList[0].name = "DataType";
	found = edsParseFile(edsFileName, strg, &edsPairList[0], 1u);
	if (found != CO_TRUE)  {
		/* no simple object, get object info from sub n */
		sprintf(strg, "%xsub%x", index, subIndex);
	}

	edsPairList[1].name = "AccessType";
	edsPairList[2].name = "PDOMapping";
	edsPairList[3].name = "DefaultValue";
	edsPairList[4].name = "ParameterValue";
	found = edsParseFile(edsFileName, strg, &edsPairList[0], 5u);
	if (found != CO_TRUE)  {
		/* printf("Error get objects entry %d - abort\n", i); */
		return(RET_SUBIDX_NOT_FOUND);
	}

	/* setup attributes */
	if (strcmp(edsPairList[1].result, "ro") == 0)  {
		*pAttr = CO_ATTR_READ;
	} else
	if (strcmp(edsPairList[1].result, "rw") == 0)  {
		*pAttr = CO_ATTR_READ | CO_ATTR_WRITE;
	} else
	if (strcmp(edsPairList[1].result, "wo") == 0)  {
		*pAttr = CO_ATTR_WRITE;
	} else
	if (strcmp(edsPairList[1].result, "rwr") == 0)  {
		*pAttr = CO_ATTR_READ | CO_ATTR_WRITE | CO_ATTR_MAP_TR;
	} else
	if (strcmp(edsPairList[1].result, "rww") == 0)  {
		*pAttr = CO_ATTR_READ | CO_ATTR_WRITE | CO_ATTR_MAP_REC;
	} else {
	}

	/* Mapping allowed ? */
	if (strtol(edsPairList[2].result, NULL, 0) != 0)  {
		*pAttr |= CO_ATTR_MAP;
	}

	/* default value available ? */
	if (strlen(edsPairList[3].result) != 0u)  {
		strcpy(pDefaultVal, edsPairList[3].result);
		*pAttr |= CO_ATTR_DEFVAL;
	}

	/* parameter value available ? */
	if (strlen(edsPairList[4].result) != 0u)  {
		/* overwrite default value */
		strcpy(pDefaultVal, edsPairList[4].result);
		*pAttr |= CO_ATTR_DEFVAL;
	}

	/* datatype */
	*pDataType = (UNSIGNED16)strtol(edsPairList[0].result, NULL, 0);
	if (*pDataType < 9u)  {
		*pAttr |= CO_ATTR_NUM;
	}

	return(RET_OK);
}


/***************************************************************************/
/**
* \internal
*
* \brief strstrlower - simple implementation of strcasestr()
*
*
* \returns
* 	RET_T
*
*/
static const CHAR *strstrlower(
		const CHAR *strg,
		const CHAR *needle
	)
{
UNSIGNED16	lNeedle = (UNSIGNED16)strlen(needle);
UNSIGNED16	lStrg = (UNSIGNED16)strlen(strg);
UNSIGNED16	start = 0u;

	/* if searchstring greater than string */
	while (lNeedle <= lStrg)  {
		/* first char ok ? */
		INTEGER c1 = tolower((INTEGER)strg[start]);
		INTEGER c2 = tolower((INTEGER)*needle);
		if (c1 == c2)  {
			if (findStrg(&strg[start], needle) != NULL)  {
				return(&strg[start]);
			}
		}
		start++;
		lStrg--;
	}

	return(NULL);
}


/***************************************************************************/
/**
* \internal
*
* \brief findStrg()
*
*
* \returns
* 	char *
*
*/
static const CHAR *findStrg(
		const CHAR *strg,
		const CHAR *needle
	)
{

	/* not end of string reached */
	while (*needle != '\0')  {
		INTEGER c1 = tolower((INTEGER)*strg);
		INTEGER c2 = tolower((INTEGER)*needle);
		if (c1 == c2)  {
			return(NULL);
		}
		strg++;
		needle++;
	}

	return(strg);
}
