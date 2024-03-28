/*
* ico_sleep.h - contains internal defines for SLEEP
*
* Copyright (c) 2013-2019 emotas embedded communication GmbH
*
*-------------------------------------------------------------------
* $Id: ico_sleep.h 33186 2020-08-21 13:24:22Z boe $

*-------------------------------------------------------------------
*
*
*/

/**
* \file
* \brief dataty type header
*/

#ifndef ICO_SLEEP_H
#define ICO_SLEEP_H 1


/* datatypes */



/* function prototypes */

void	icoSleepReset(UNSIGNED8 master, UNSIGNED8 nodeId);
RET_T	icoSleepInit(UNSIGNED8 master);
void	icoSleepMsgHandler(CO_CONST CO_REC_DATA_T *pRec);
void	icoSleepVarInit(void);

void	icoSleepSetMasterSlave(CO_NMT_MASTER_STATE_T mState);

#endif /* ICO_SLEEP_H */
