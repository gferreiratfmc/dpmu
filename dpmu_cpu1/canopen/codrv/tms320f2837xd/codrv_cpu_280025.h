/*
* codrv_cpu_280025.h
*
* Copyright (c) 2012-2020 emotas embedded communication GmbH
*-------------------------------------------------------------------
* SVN  $Date: 2018-11-09 15:03:17 +0100 (Fr, 09 Nov 2018) $
* SVN  $Rev: 25490 $
* SVN  $Author: hil $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \file
* \brief CPU driver part for the  driver
*
* demonstrate the use of the driverlib 
*
*/

#ifndef CODRV_CPU_28002x_H
#define CODRV_CPU_28002x_H 1

//#include "f28004x_device.h"
//#include "f28004x_cla_typedefs.h"   // f28004x CLA Type definitions
//#include "f28004x_device.h"         // f28004x Headerfile Include File
//#include "f28004x_examples.h"       // f28004x Examples Include File
#include "device.h"

/* general hardware initialization */
void codrvHardwareInit(void);

/* init CAN related hardware part */
void codrvHardwareCanInit(void);

/* signal definition for use with TI-RTOS kernel */
#ifndef TX_SIGNAL
# define TX_SIGNAL 0u       /**< transmit signal code */
#endif /* !TX_SIGNAL */

#ifndef RX_SIGNAL
# define RX_SIGNAL 1u       /**< receive signal code */
#endif /* !RX_SIGNAL */

#ifndef ST_SIGNAL
# define ST_SIGNAL 2u       /**< can state signal code */
#endif /* !ST_SIGNAL */

#ifndef TM_SIGNAL
# define TM_SIGNAL 3u       /**< time signal */
#endif /* !TM_SIGNAL */

#endif /* CODRV_CPU_28002x_H */
