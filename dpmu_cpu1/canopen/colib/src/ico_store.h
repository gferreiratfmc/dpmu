/*
* ico_store.h - contains internal defines for STORE
*
* Copyright (c) 2012-2019 emotas embedded communication GmbH
*
*-------------------------------------------------------------------
* $Id: ico_store.h 41218 2022-07-12 10:35:19Z boe $

*-------------------------------------------------------------------
*
*
*/

/**
* \file
* \brief dataty type header
*/

#ifndef ICO_STORE_H
#define ICO_STORE_H 1


/* datatypes */


/* function prototypes */

RET_T	icoStoreLoadReq(UNSIGNED8 subIndex);
void	*icoStoreGetObjectAddr(UNSIGNED16 idx, UNSIGNED8 subIndex);
RET_T	icoStoreCheckObjLimit_u32(UNSIGNED16 index, UNSIGNED32 signature);
RET_T	icoStoreObjChanged(UNSIGNED16 index, UNSIGNED8 subIndex);
void	icoStoreVarInit(void);
void	icoStoreReset(void);

#endif /* ICO_STORE_H */
