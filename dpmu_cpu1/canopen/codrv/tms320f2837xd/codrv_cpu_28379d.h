/*
* codrv_cpu_28379d.h
*
* Copyright (c) 2012-2016 emotas embedded communication GmbH
*-------------------------------------------------------------------
* SVN  $Date: 2019-08-30 17:38:11 +0200 (Fr, 30. Aug 2019) $
* SVN  $Rev: 29114 $
* SVN  $Author: phi $
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
*
*
*/

#ifndef CODRV_CPU_28379_H
#define CODRV_CPU_28379_H 1

#include "F2837xD_device.h"
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

#endif /* CODRV_CPU_23379_H */
