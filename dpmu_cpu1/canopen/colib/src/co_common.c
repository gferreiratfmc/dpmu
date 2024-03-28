/*
* co_common.c - contains functions for common initialisation
*
* Copyright (c) 2021-2021 emotas embedded communication GmbH
*-------------------------------------------------------------------
* $Id: $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \brief Functions for common stack intialization
*
* \file co_common.c contains functions for common initialization
*
*/

/* header of standard C - libraries
---------------------------------------------------------------------------*/

/* header of project specific types
---------------------------------------------------------------------------*/
#include <gen_define.h>
#include <co_datatype.h>
#include <co_drv.h>
#include <co_timer.h>
#include "ico_cobhandler.h"
#include "ico_queue.h"
#include "ico_commtask.h"
#include "ico_event.h"
#include "ico_candebug.h"
#ifdef CO_USER_EXTENSION_SUPPORTED
# include "ico_user.h"
#endif /* CO_USER_EXTENSION_SUPPORTED */
#include "ico_common.h"
#ifdef ISOTP_SUPPORTED
#include <iso_tp.h>
#endif /* ISOTP_SUPPORTED */


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

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
static MP_PROTOCOL_TYPE_T	protocolType;



/***************************************************************************/
/**
*
* \brief coCommonInit - init of variables of the stack
*
* This function initializes all common global and local variables of the stack.
* 
* It can also be used to reinitialize the stack.
*
*/
RET_T commonStackInit(
		const CO_COMMON_INIT_VAL_T *pCommonInitVals	/**< pointer to init vals */
	)
{
RET_T	retVal = RET_OK;

	/* check for correct init struct */
	if (pCommonInitVals->structVersion != CO_COMMON_INIT_STRUCT_VERSION)  {
		return(RET_PARAMETER_INCOMPATIBLE);
	}

	coTimerInit(CO_TIMER_INTERVAL);

	protocolType = pCommonInitVals->protocolType[0];

	icoCobHandlerVarInit();

	icoCommTaskVarInit();

	icoEventInit();

	icoQueueVarInit(&pCommonInitVals->recBufferCnt[0],
		&pCommonInitVals->trBufferCnt[0]);

#ifdef ISOTP_SUPPORTED
	retVal = isoTpInit(&pCommonInitVals->isotpClientCnt[0],
					&pCommonInitVals->isotpServerCnt[0]);
	if (retVal != RET_OK)  {
		return(retVal);
	}
#endif /* ISOTP_SUPPORTED */

#ifdef CO_CAN_DEBUG_SUPPORTED
	icoCanDebugVarInit();
#endif /* CO_CAN_DEBUG_SUPPORTED */

#ifdef CO_USER_EXTENSION_SUPPORTED
	icoUserVarInit();
#endif /* CO_USER_EXTENSION_SUPPORTED */

	return(retVal);
}




/***************************************************************************/
/**
* \internal
*
* \brief icoProtocolType - get actual protocol type
*
* This function returns the actual protocol type for the given line
* 
* \return
*	protocol type
*/
MP_PROTOCOL_TYPE_T icoProtocolType(
		void	/* no parameter */
	)
{
	return(protocolType);
}
