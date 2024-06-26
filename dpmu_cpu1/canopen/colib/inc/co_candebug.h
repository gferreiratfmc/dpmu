/*
* co_candebug.h - contains defines for can debug
*
* Copyright (c) 2012-2019 emotas embedded communication GmbH
*
*-------------------------------------------------------------------
* $Id: co_candebug.h 29114 2019-08-30 15:38:11Z phi $
*-------------------------------------------------------------------
*
*
*/

/**
* \brief defines for can debug
*
* \file co_candebug.h - contains defines for can debug services
*
*/

#ifndef CO_CAN_DEBUG_H
#define CO_CAN_DEBUG_H 1

#include <co_datatype.h>


/* constant */

/* function prototypes */
EXTERN_DECL RET_T coCanDebugPrint(UNSIGNED32 canId,
			UNSIGNED8 dataLen, const UNSIGNED8 *pData);


#endif /* CO_CAN_DEBUG_H */
