/*
* ico_odaccess.h - contains internal defines for od access
*
* Copyright (c) 2012-2019 emotas embedded communication GmbH
*
*-------------------------------------------------------------------
* $Id: ico_odaccess.h 29114 2019-08-30 15:38:11Z phi $

*-------------------------------------------------------------------
*
*
*/

/**
* \file
* \brief odaccess header
*/

#ifndef ICO_OD_ACCESS_H
#define ICO_OD_ACCESS_H 1


/* datatypes */

/* function prototypes */

RET_T	icoOdCheckObjLimits(CO_CONST CO_OBJECT_DESC_T *pDesc,
			CO_CONST UNSIGNED8	*pData);
RET_T	icoEventObjectChanged(CO_CONST CO_OBJECT_DESC_T *pDesc,
			UNSIGNED16 index, UNSIGNED8 subIndex, BOOL_T changed);
RET_T	icoOdGetObj(const CO_OBJECT_DESC_T *pDesc, UNSIGNED8 subIndex,
			void *pData, UNSIGNED32 offset, UNSIGNED32 maxData, BOOL_T internal);
RET_T	icoOdPutObj(const CO_OBJECT_DESC_T *pDesc, UNSIGNED8 subIndex,
			CO_CONST void *pData,
			UNSIGNED32 offset, UNSIGNED32 maxData, BOOL_T internal, BOOL_T *pChanged);
RET_T	icoOdGetObjRecMapData(UNSIGNED16 index, UNSIGNED8 subIndex,
			UNSIGNED8 bitLen, void **pVar, UNSIGNED8	*pLen, BOOL_T *pNumeric);
RET_T	icoOdGetObjTrMapData(UNSIGNED16 index, UNSIGNED8 subIndex,
			UNSIGNED8 bitLen, CO_CONST void **pVar, UNSIGNED8	*pLen, BOOL_T *pNumeric);
UNSIGNED8	icoOdGetNumberOfSubs(UNSIGNED16 index);
void	icoOdReset(UNSIGNED16 startIdx, UNSIGNED16 lastIdx);
RET_T	icoOdCheckObjAttr(UNSIGNED16 index, UNSIGNED8 subIndex,
			UNSIGNED16	checkAttr);
void	icoOdSetLoadParaState(BOOL_T newState);
CO_EVENT_OBJECT_CHANGED_FCT_T icoEventObjectChangedFunction(UNSIGNED16 index, UNSIGNED8	subIndex);
void	icoOdAccessVarInit(void);

#endif /* IOD_ACCESS_H */
