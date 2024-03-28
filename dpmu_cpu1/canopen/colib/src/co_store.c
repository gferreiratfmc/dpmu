/*
* co_store.c - contains store/restore functionality
*
* Copyright (c) 2012-2022 emotas embedded communication GmbH
*-------------------------------------------------------------------
* $Id: co_store.c 41218 2022-07-12 10:35:19Z boe $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \brief Store/Restore functionality
*
* \file co_store.c
* contains routines for handling store/restore OD data
*
*/


/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <stddef.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <gen_define.h>
#include <co_datatype.h>
#include <co_store.h>
#include <co_odaccess.h>
#include "ico_indication.h"
#include "ico_odaccess.h"
#include "ico_store.h"

/* constant definitions
---------------------------------------------------------------------------*/
#ifdef CO_EVENT_DYNAMIC_STORE
# ifdef CO_EVENT_PROFILE_STORE
#  define CO_EVENT_STORE_CNT	(CO_EVENT_DYNAMIC_STORE + CO_EVENT_PROFILE_STORE)
# else /* CO_EVENT_PROFILE_STORE */
#  define CO_EVENT_STORE_CNT	(CO_EVENT_DYNAMIC_STORE)
# endif /* CO_EVENT_PROFILE_STORE */
#else /* CO_EVENT_DYNAMIC_STORE */
# ifdef CO_EVENT_PROFILE_STORE
#  define CO_EVENT_STORE_CNT	(CO_EVENT_PROFILE_STORE)
# endif /* CO_EVENT_PROFILE_STORE */
#endif /* CO_EVENT_DYNAMIC_STORE */


/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/
#ifdef CO_EVENT_STATIC_STORE
extern CO_CONST CO_EVENT_STORE_T coEventStoreInd;
#endif /* CO_EVENT_STATIC_STORE */


/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CO_EVENT_STORE_CNT
static UNSIGNED16	storeCnt = 0u;
static CO_EVENT_STORE_T	storeLoadFunction[CO_EVENT_STORE_CNT] = { NULL };
# ifdef CO_STORE_SUPPORTED
static CO_EVENT_STORE_T	storeSaveFunction = NULL;
static CO_EVENT_STORE_T	storeClearFunction = NULL;
# endif /* CO_STORE_SUPPORTED */
#endif /* CO_EVENT_STORE_CNT */

#ifdef CO_STORE_SUPPORTED
# ifdef CO_AUTO_STORE_HANDLED_BY_APPLICATION
static UNSIGNED32	storeObj[3];
# else /* CO_AUTO_STORE_HANDLED_BY_APPLICATION */
static UNSIGNED32	storeObj;
# endif /* CO_AUTO_STORE_HANDLED_BY_APPLICATION */
static UNSIGNED32	restoreObj;
#endif /* CO_STORE_SUPPORTED */


/***************************************************************************/
/**
* \internal
*
* \brief void icoStoreLoadReq - load parameter requested
*
*
* \return none
*
*/
RET_T icoStoreLoadReq(
		UNSIGNED8	subIndex
	)
{
RET_T	retVal = RET_OK;
#ifdef CO_EVENT_STORE_CNT
UNSIGNED16	i;
#endif /* CO_EVENT_STORE_CNT */

	(void)subIndex; /* avoid unused warning */

#ifdef CO_EVENT_STORE
	/* inform od functions */
	icoOdSetLoadParaState(CO_TRUE);

# ifdef CO_EVENT_STORE_CNT
	for (i = 0u; i < storeCnt; i++)  {
		/* call load parameter indication */
		retVal = storeLoadFunction[i](subIndex);
	}
# endif /* CO_EVENT_STORE_CNT */

# ifdef CO_EVENT_STATIC_STORE
	coEventStoreInd(subIndex);
# endif /* CO_EVENT_STATIC_STORE */

	/* inform od functions */
	icoOdSetLoadParaState(CO_FALSE);
#endif /* CO_EVENT_STORE */
	return(retVal);
}


#ifdef CO_EVENT_STORE_CNT
/***************************************************************************/
/**
* \brief coEventRegister_LOAD_PARA - register LOAD_PARA event
*
* register indication function for LOAD_PARA events
*
* \return RET_T
*
*/

RET_T coEventRegister_LOAD_PARA(
		CO_EVENT_STORE_T pFunction	/**< pointer to function */
    )
{
	if (storeCnt >= CO_EVENT_STORE_CNT)  {
		return(RET_EVENT_NO_RESSOURCE);
	}

	if (pFunction == NULL)  {
		return(RET_INVALID_PARAMETER);
	}

	storeLoadFunction[storeCnt] = pFunction;		/* save function pointer */
	storeCnt++;

	return(RET_OK);
}


# ifdef CO_STORE_SUPPORTED
/***************************************************************************/
/**
* \brief coEventRegister_SAVE_PARA - register SAVE_PARA event
*
* register indication function for SAVE_PARA events
*
* \return RET_T
*
*/

RET_T coEventRegister_SAVE_PARA(
		CO_EVENT_STORE_T pFunction	/**< pointer to function */
    )
{
	storeSaveFunction = pFunction;		/* save function pointer */

	return(RET_OK);
}


/***************************************************************************/
/**
* \brief coEventRegister_CLEAR_PARA - register CLEAR_PARA event
*
* register indication function for CLEAR_PARA events
*
* \return RET_T
*
*/

RET_T coEventRegister_CLEAR_PARA(
		CO_EVENT_STORE_T pFunction	/**< pointer to function */
    )
{
	storeClearFunction = pFunction;		/* save function pointer */

	return(RET_OK);
}
# endif /* CO_STORE_SUPPORTED */
#endif /* CO_EVENT_STORE_CNT */


#ifdef CO_STORE_SUPPORTED
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/**
* \internal
*
* \brief icoStorGetObjectAddr - get Store object address
*
*
* \return RET_T
*
*/
void *icoStoreGetObjectAddr(
		UNSIGNED16	idx,			/* index */
		UNSIGNED8	subIndex		/* subindex */
	)
{
void	*pAddr = NULL;
(void)subIndex;

	if (idx == 0x1010u)  {		/* store */
#ifdef CO_AUTO_STORE_HANDLED_BY_APPLICATION
		pAddr = (void *)&storeObj[subIndex - 1u];
#else /* CO_AUTO_STORE_HANDLED_BY_APPLICATION */
		pAddr = (void *)&storeObj;
		/* store supported only on command */
		storeObj = 1u;
#endif /* CO_AUTO_STORE_HANDLED_BY_APPLICATION */
	} else {
		if (idx == 0x1011u)  {		/* restore */
			pAddr = (void *)&restoreObj;
			/* restore supported only on command */
			restoreObj = 1u;
		}
	}

	return(pAddr);
}


/***************************************************************************/
/**
* \internal
*
* \brief icoStoreCheckObjLimit_u32 - check correct signature
*
*
* \return RET_T
*
*/
RET_T icoStoreCheckObjLimit_u32(
		UNSIGNED16	index,		/* index */
		UNSIGNED32	signature	/* written signature */
	)
{
RET_T	retVal = RET_OK;

	if (index == 0x1010u)  {
		/* check for correct signature */
		if (signature != CO_STORE_SIGNATURE_SAVE)  {
			retVal = RET_ERROR_STORE;
		}
	} else {
		if (index == 0x1011u)  {
			/* check for correct signature */
			if (signature != CO_STORE_SIGNATURE_LOAD)  {
				retVal = RET_ERROR_STORE;
			}
		}
	}
	return(retVal);
}


/***************************************************************************/
/**
* \internal
*
* \brief icoStoreObChanged - object was changed
*
*
* \return RET_T
*
*/
RET_T icoStoreObjChanged(
		UNSIGNED16	index,
		UNSIGNED8	subIndex
	)
{
RET_T	retVal = RET_OK;

#ifdef CO_EVENT_STORE_CNT
	if (index == 0x1010u)  {		/* store */
		if (storeSaveFunction != NULL)  {
			/* call save parameter indication */
			retVal = storeSaveFunction(subIndex);
		} else {
			/* inform about not saved */
			retVal = RET_HARDWARE_ERROR;
		}
	} else {
		if (index == 0x1011u)  {		/* restore */
			if (storeClearFunction != NULL)  {
				/* call Clear parameter indication */
				retVal = storeClearFunction(subIndex);
			} else {
				/* inform about not restored */
				retVal = RET_HARDWARE_ERROR;
			}
		}
	}
#else /* CO_EVENT_STORE_CNT */
	(void)index;
	(void)subIndex;
#endif /* CO_EVENT_STORE_CNT */

	/* use similar error message */
	if (retVal != RET_OK)  {
		retVal = RET_HARDWARE_ERROR;
	}
	return(retVal);
}
#endif /* CO_STORE_SUPPORTED */


#ifdef CO_STORE_SUPPORTED
# ifdef CO_AUTO_STORE_HANDLED_BY_APPLICATION
/***************************************************************************/
/**
* \internal
*
* \brief icoStoreReset - reset to default values
*
*
* \return RET_T
*
*/
void icoStoreReset(
		void	/* no parameter */
	)
{
UNSIGNED16	i;
UNSIGNED32	val;
RET_T		retVal;

	for (i = 0u; i < 3u; i++)  {
		retVal = coOdGetDefaultVal_u32(0x1010u, i + 1u, &val);
		if (retVal == RET_OK)  {
			storeObj[i] = val;
		} else {
			storeObj[i] = 1u;
		}
	}
}
# endif /* CO_AUTO_STORE_HANDLED_BY_APPLICATION */
#endif /* CO_STORE_SUPPORTED */


#ifdef CO_EVENT_STORE_CNT
/***************************************************************************/
/**
* \internal
*
* \brief icoStoreVarInit
*
*
* \return RET_T
*
*/
void icoStoreVarInit(
		void
	)
{
	storeCnt = 0u;
# ifdef CO_STORE_SUPPORTED
	storeSaveFunction = NULL;
	storeClearFunction = NULL;

#  ifdef CO_AUTO_STORE_HANDLED_BY_APPLICATION
		storeObj[0] = 1u;
		storeObj[1] = 1u;
		storeObj[2] = 1u;
#  endif /* CO_AUTO_STORE_HANDLED_BY_APPLICATION */
# endif /* CO_STORE_SUPPORTED */
}
#endif /* CO_EVENT_STORE_CNT */
