/*
* ico_time.h - contains defines for time services
*
* Copyright (c) 2012-2019 emotas embedded communication GmbH
*
*-------------------------------------------------------------------
* $Id: ico_time.h 29114 2019-08-30 15:38:11Z phi $

*-------------------------------------------------------------------
*
*
*/

/**
* \file
* \brief dataty type header
*/

#ifndef ICO_TIME_H
#define ICO_TIME_H 1


/* datatypes */

/* function prototypes */

void	icoTimeReset(void);
void	icoTimeSetDefaultValue(void);
void	*icoTimeGetObjectAddr(void);
RET_T	icoTimeCheckObjLimit_u32(UNSIGNED32	value);
RET_T	icoTimeObjChanged(void);
void	icoTimeHandler(const UNSIGNED8 *pData);
void	icoTimeVarInit(void);

#endif /* ICO_TIME_H */

