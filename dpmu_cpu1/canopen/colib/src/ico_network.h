/*
* ico_network.h - contains internal defines for multi level networking
*
* Copyright (c) 2014-2019 emotas embedded communication GmbH
*
*-------------------------------------------------------------------
* $Id: ico_network.h 29114 2019-08-30 15:38:11Z phi $
*-------------------------------------------------------------------
*
*
*/

/**
* \file
* \brief dataty type header
*/

#ifndef ICO_NETWORK_H
#define ICO_NETWORK_H 1

/* datatypes */

/* function prototypes */

UNSIGNED16 icoNetworkLocalId(void);
void *icoNetworkGetObjectAddr(UNSIGNED16 index, UNSIGNED8	subIndex);
void icoNetworkSetDefaultValue(void);
void icoNetworkVarInit(CO_CONST UNSIGNED8 *pList);

#endif /* ICO_NETWORK_H */
