/*
* co_sync.h - contains defines for sync services
*
* Copyright (c) 2012-2019 emotas embedded communication GmbH
*
*-------------------------------------------------------------------
* $Id: co_sync.h 29114 2019-08-30 15:38:11Z phi $
*-------------------------------------------------------------------
*
*
*
*/

/**
* \brief defines for sync services
*
* \file co_sync.h - contains defines for sync services
*/

#ifndef CO_SYNC_H
#define CO_SYNC_H 1

#include <co_datatype.h>


/* datatypes */

/** \brief function pointer to SYNC indication
 * \param syncCounter - actual SYNC counter
 * 
 */
typedef void (* CO_EVENT_SYNC_T)(UNSIGNED8);


/** \brief function pointer to SYNC Finished indication
 * \param syncCounter - actual SYNC counter
 * 
 */
typedef void (* CO_EVENT_SYNC_FINISHED_T)(UNSIGNED8);


EXTERN_DECL RET_T coSyncInit(UNSIGNED32	cobId);
EXTERN_DECL RET_T coEventRegister_SYNC(CO_EVENT_SYNC_T pFunction);
EXTERN_DECL RET_T coEventRegister_SYNC_FINISHED(CO_EVENT_SYNC_FINISHED_T pFunction);

#endif /* CO_SYNC_H */
