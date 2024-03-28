/*
* ico_srd.h - contains defines for srd services
*
* Copyright (c) 2015-2019 emotas embedded communication GmbH
*
*-------------------------------------------------------------------
* $Id: ico_srd.h 29114 2019-08-30 15:38:11Z phi $

*-------------------------------------------------------------------
*
*
*/

/**
* \file
* \brief dataty type header
*/

#ifndef ICO_SRD_H
#define ICO_SRD_H 1


/* datatypes */

/* function prototypes */

void icoSrdReset(void);
void *icoSrdGetObjectAddr(UNSIGNED16 idx);
RET_T icoSrdCheckObjLimit_u32(UNSIGNED32 value);
void icoSrdVarInit(void);


#endif /* ICO_SRD_H */

