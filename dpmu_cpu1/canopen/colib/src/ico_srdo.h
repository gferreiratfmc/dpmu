/*
* ico_srdo.h - contains internal defines for SRDO
*
* Copyright (c) 2015-2019 emotas embedded communication GmbH
*
*-------------------------------------------------------------------
* $Id: ico_srdo.h 29114 2019-08-30 15:38:11Z phi $
*-------------------------------------------------------------------
*
*
*
*/

/**
* \file
* \brief dataty type header
*/

#ifndef ICO_SRDO_H
#define ICO_SRDO_H 1


/* datatypes */


/* function prototypes */

void	icoSrdoVarInit(UNSIGNED16 *pSrdoCnt);
void	*icoSrdoGetObjectAddr(UNSIGNED16 index, UNSIGNED8 subIndex, CO_DATA_TYPE_T dataType);
RET_T	icoSrdoObjChanged(UNSIGNED16 index, UNSIGNED8	subIndex, CO_DATA_TYPE_T dataType);
RET_T	icoSrdoCheckObjLimit_u8(UNSIGNED16 index, UNSIGNED8 subIndex, CO_DATA_TYPE_T dataType,
			UNSIGNED8	value);
RET_T	icoSrdoCheckObjLimit_u16(UNSIGNED16 index, UNSIGNED8 subIndex, CO_DATA_TYPE_T dataType,
			UNSIGNED16	value);
RET_T	icoSrdoCheckObjLimit_u32(UNSIGNED16 index, UNSIGNED8 subIndex, CO_DATA_TYPE_T dataType,
			UNSIGNED32	value);
void	icoSrdoOperational(void);
void	icoSrdoPreOperational(void);
RET_T	icoSrdoCheckOperational(void);
void	icoSrdoSetDefaultValue(void);
void	icoSrdoReset(void);
void	icoSrdoHandler(CO_CONST CO_REC_DATA_T *pRecData);
void	icoSrdoTimerCheck(void);

#endif /* ICO_SRDO_H */
