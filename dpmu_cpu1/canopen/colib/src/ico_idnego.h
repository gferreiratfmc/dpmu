/*
* ico_idnego.h - contains internal defines for NMT
*
* Copyright (c) 2019 emotas embedded communication GmbH
*
*-------------------------------------------------------------------
* $Id: ico_idnego.h 33000 2020-08-05 11:57:49Z phi $
*
*-------------------------------------------------------------------
*
*
*/

/**
* \file
* \brief dataty type header
*/

#ifndef ICO_IDNEGO_H
#define ICO_IDNEGO_H 1

/* datatypes */

/* object 0x1f80 */

/* function prototypes */
void icoIdNegoHandler(const CO_REC_DATA_T	*pRecData);
void icoIdNegoReset(UNSIGNED8 *pNodeId);
void icoIdNegoVarInit(void);

#endif /* ICO_IDNEGO_H */




