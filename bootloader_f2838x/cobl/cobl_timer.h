/*
* cobl_timer.h
*
* Copyright (c) 2012-2020 emotas embedded communication GmbH
*-------------------------------------------------------------------
* SVN  $Date: 2020-08-28 14:28:54 +0200 (Fr, 28. Aug 2020) $
* SVN  $Rev: 33311 $
* SVN  $Author: ged $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \file
* \brief cobl_timer.h - Timer functionality
*
*/



#ifndef COBL_TIMER_H
#define COBL_TIMER_H 1

/* Prototypes
*--------------------------------------------------------------------*/
void timerInit(void);
U8 timerTimeExpired(U16 timeVal);
void timerWait100ms(void); 

#endif /* COBL_TIMER_H */
