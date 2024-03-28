/*
* cobl_timer.c
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
* \brief cobl_timer.c - Timer functionality
*
*/



/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <time.h>
#include <stddef.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <user_config.h>
#include <cobl_type.h>
#include <cobl_timer.h>

/* hardware header
---------------------------------------------------------------------------*/
#include <cobl_hardware.h>

/* constant definitions
---------------------------------------------------------------------------*/
static const U16 timeTick = 1; /* 1 ms */

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/


/***************************************************************************/
/**
 * \brief initialialize the cyclic timer
 *
 * Note: This function is also used during the reinitialization of the device.
 */
void timerInit(void)
{
    // 200MHz CPU Freq, x ms Period (in uSeconds)
    ConfigCpuTimer(&CpuTimer0, 200u, 1000);

    StartCpuTimer0();
}

/***************************************************************************/
/**
 * \brief check for the end of a period
 *
 * Note: It is possible, that more than one period is over.
 *
 * 
 */
U8 timerTimeExpired(U16 timeVal)
{
static U16 periodCnt = 0;

	if (CpuTimer0.RegsAddr->TCR.bit.TIF != 0) {
		CpuTimer0.RegsAddr->TCR.bit.TIF = 1;
		periodCnt++;
	}

	if (periodCnt >= (timeVal/timeTick)) {
		periodCnt = 0;
		return 1u; /* period is over */
	}

	return 0;
}

/***************************************************************************/
/**
* \brief wait a specific time
*
* This function wait for 100ms.
*
*/
void timerWait100ms(void)
{

	timerInit();

	while(timerTimeExpired(100) == 0) {
		;
	}

	/* 100ms later ... */
}
