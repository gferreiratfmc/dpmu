/*
* codrv_cpu_28379d.c - contains driver for cpu
*
* Copyright (c) 2012-2016 emotas embedded communication GmbH
*-------------------------------------------------------------------
* $Id: codrv_cpu_280049.c 30789 2020-02-20 14:06:34Z hil $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \brief CPU specific routines
*
* \file codrv_cpu_280049c.c
* cpu specific routines
*
* This module contains the cpu specific routines for initialization
* and timer handling.
*
* \author emtas GmbH
*/

/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <stdio.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <gen_define.h>

#include <co_datatype.h>
#include <co_timer.h>

#include "codrv_cpu_280049.h"
#include "codrv_dcan.h"

#include "f28004x_device.h"
#include "f28004x_cla_typedefs.h"   // f28004x CLA Type definitions
#include "f28004x_device.h"         // f28004x Headerfile Include File
#include "f28004x_examples.h"       // f28004x Examples Include File
#include "device.h"


#ifdef CO_RTOS_SUPPORTED
/* BIOS module Headers */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/cfg/global.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Event.h>

/* semaphores and events */


#endif


/* constant definitions
---------------------------------------------------------------------------*/
#ifndef CAN_IRQ_BEGIN
# define CAN_IRQ_BEGIN
# define CAN_IRQ_END
#endif

/* OS related default definition */
#ifdef CO_OS_SIGNAL_TIMER
#else /* CO_OS_SIGNAL_TIMER */
#  define CO_OS_SIGNAL_TIMER
#endif /* CO_OS_SIGNAL_TIMER */



/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/
extern void codrvCanInterrupt(void);

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
interrupt void codrvTimerISR(void);

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
extern struct CPUTIMER_VARS CpuTimer0;

/***************************************************************************/
/**
* \brief codrvHardwareInit - hardware initialization
*
* This function initializes the hardware, incl. clock and CAN hardware.
*/
void codrvHardwareInit(void)
{
	/* init PLL, activate clocks */
    Device_init();

    DINT;



    InitPieCtrl();
    InitPieVectTable();
    InitCpuTimers(); /* we only use CpuTimer 0 */

	codrvHardwareCanInit();
}

/***************************************************************************/
/**
* \brief codrvInitCanHW - CAN related hardware initialization
*
* Within this function you find the CAN only hardware part.
* Goal of it is, that you can have your own hardware initialization
* like codrvHardwareInit(), but you can add our tested CAN 
* initialization.
*
*/
interrupt void codrvCan0Irq(void);
void codrvHardwareCanInit(void)
{
    // GPIO Pins
    // Initialize GPIO and configure GPIO pins for CANTX/CANRX

    Device_initGPIO();
    GPIO_setPinConfig(GPIO_33_CANRXA);
    GPIO_setPinConfig(GPIO_32_CANTXA);

    EALLOW;
    PieVectTable.CANA0_INT = codrvCan0Irq;
    EDIS;
}

/***************************************************************************/
/**
* \brief codrvCanEnableInterrupt - enable the CAN interrupt
*
*/
void codrvCanEnableInterrupt(void)
{
	/* enable CAN interrupts */
    Interrupt_enable(INT_CANA0);
    CAN_enableGlobalInterrupt(CANA_BASE, CAN_GLOBAL_INT_CANINT0);

}

/***************************************************************************/
/**
* \brief codrvCanDisableInterrupt - disable the CAN interrupt
*
*/
void codrvCanDisableInterrupt(void)
{
	/* disable CAN interrupts */
    Interrupt_disable(INT_CANA0);
}

/***************************************************************************/
/**
* \brief codrvCanSetTxInterrupt - set pending bit of the Transmit interrupt
*
* This function set the interrupt pending bit. *
*/
void codrvCanSetTxInterrupt(void)
{

    /* not possible */
}



interrupt void codrvCan0Irq(void)
{
	CAN_IRQ_BEGIN

	codrvCanInterrupt();


    //
    // Clear the global interrupt flag for the CAN interrupt line
    //
    CAN_clearGlobalInterruptStatus(CANA_BASE, CAN_GLOBAL_INT_CANINT0);

    //
    // Acknowledge this interrupt located in group 9
    //
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);

	CAN_IRQ_END
}

/***************************************************************************/
/**
* \brief codrvTimerSetup - init and configure the hardware Timer
*
* This function starts a cyclic hardware timer to provide a timing interval
* for the CANopen library.
* Alternativly it can be derived from an other system timer
* with the timer interval given by the function parameter.
*
* \return RET_T
* \retval RET_OK
*	intialization of the timer was ok
*
*/

RET_T codrvTimerSetup(
		UNSIGNED32	timerInterval		/**< timer interval in usec */
	)
{

	/* start hardware timer */
	EALLOW;  // This is needed to write to EALLOW protected registers
	PieVectTable.TIMER0_INT = &codrvTimerISR;
	EDIS;    // This is needed to disable write to EALLOW protected registers

	// Configure CPU-Timer 0 to interrupt every 1ms:
	// 100MHz CPU Freq, x ms Period (in uSeconds)
	ConfigCpuTimer(&CpuTimer0, 100u, timerInterval);
	StartCpuTimer0();

	// Enable TINT0 in the PIE: Group 1 interrupt 7
	PieCtrlRegs.PIEIER1.bit.INTx7 = 1;

	Interrupt_enable(INT_TIMER0);
	return(RET_OK);
}


/***************************************************************************/
/**
* \brief codrvTimerISR - Timer interrupt service routine
*
* This function is normally called from timer interrupt
* or from an other system timer.
* It has to call the timer handling function at the library.
*
*
* \return void
*
*/
interrupt void codrvTimerISR(
		void
    )
{
	/* inform stack about new timer event */
	coTimerTick();

	// Acknowledge this interrupt to receive more interrupts from group 1
	PieCtrlRegs.PIEACK.all = PIE_ACK_ACK1;

	/* signal timer tick */
	CO_OS_SIGNAL_TIMER
#ifdef CO_RTOS_SUPPORTED
	Clock_tick();
	codrvSendSignal();
#endif /* CO_RTOS_SUPPORTED */
}



/***************************************************************************/
/* Sysbios semaphore for OD Lock functions */
/***************************************************************************/
#ifdef CO_RTOS_SUPPORTED
void codrvLockOd(
        void
    )
{
    /* Get access to resource */
    Semaphore_pend(coOdSemaphore, BIOS_WAIT_FOREVER);
}

void codrvUnlockOd(
        void
        )
{
    /* Release the resource */
    Semaphore_post(coOdSemaphore);
}

/***************************************************************************/
/* Sysbios semaphore for Event functions */
/***************************************************************************/

/***************************************************************************/
/**
* \internal
*
* \brief sendSignal - send a signal
*
* \return void
* \retVal void
*
*/
void codrvSendSignal(
   void
    )
{
    Event_post(coCanEvents, Event_Id_00);
}

/***************************************************************************/
/**
* \brief codrvWaitForCanOrTimerEvent  - wait for driver signal
* This Function waits for a driver signals:
* - message was sent
* - message was received
* - CAN state received
* - timer tick
*
* \return void
* \retVal void
*
*/
void codrvWaitForCanOrTimerEvent(
    UNSIGNED16      timeout     /**< timeout in ms */
    )
{

    if (timeout != 0) {
        Event_pend(coCanEvents, Event_Id_NONE, Event_Id_00, timeout);
    } else {
        Event_pend(coCanEvents, Event_Id_NONE, Event_Id_00, BIOS_WAIT_FOREVER);
    }
}
#endif /* CO_RTOS_SUPPORTED */

