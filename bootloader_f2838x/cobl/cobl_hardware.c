/*
* cobl_hardware.c
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
* \brief cobl_hardware.c - hardware adaptations
*
*
*/


/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <string.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <user_config.h>
#include <cobl_type.h>

#include <cobl_hardware.h>
#include <cobl_call.h>

/* hardware header
---------------------------------------------------------------------------*/

/* constant definitions
---------------------------------------------------------------------------*/

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
static void initCanHW(void);

#ifdef COBL_DEBUG
static void initDebugHardware(void);
#endif

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/

/********************************************************************/
/**
 * \brief initialization for the consistency check
 * 
 * Minimalistic initialization for CRC check
 * - clocks
 */

void initBaseHardware(void)
{
	/* clock initialization */
    Device_init();

    /* disable interrupt */
    DINT;
    IER = 0x0000;
    IFR = 0x0000;


    /* initizalze basic flash */
    InitFlash();

#ifdef COBL_DEBUG
	/* only for test and UART required */
	initDebugHardware();
#endif

}

/********************************************************************/
/**
 * \brief initialization additional hardware
 * 
 * Initialize the additional hardware, that is required to flash
 * a new software
 * - CAN
 */

void initHardware(void)
{
	/* init CAN */
	initCanHW();

	/* init CPU Timer  */
	InitCpuTimers(); /* we only use CpuTimer 0 */
}

/***************************************************************************/
/**
* initialize debug hardware, e.g. UART
*
*/
#ifdef COBL_DEBUG
static void initDebugHardware(void)
{

#ifdef NO_PRINTF
#else /* NO_PRINTF */
	//SetupUART(); /* only req. for printf */
#endif /* NO_PRINTF */

}
#endif /* COBL_DEBUG */

/***************************************************************************/
/**
* \brief initCanHW - customer hardware initialization
*
*/
static void initCanHW(void)
{
    /* GPIO Pins
     * Initialize GPIO and configure GPIO pins for CANTX/CANRX
     * on CANB */
     Device_initGPIO();
//     GPIO_setPinConfig(DEVICE_GPIO_CFG_CANRXB);
//     GPIO_setPinConfig(DEVICE_GPIO_CFG_CANTXB);

     GPIO_setPinConfig(DEVICE_GPIO_CFG_CANRXA);
     GPIO_setPinConfig(DEVICE_GPIO_CFG_CANTXA);


}

/************************************************************************/
/**
* \brief toggle watchdog
*/
void toggleWD(void)
{

}



