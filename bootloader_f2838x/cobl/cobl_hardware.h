/*
* cobl_hardware.h
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
* \brief hardware adaptations
*
*/



#ifndef COBL_HARDWARE_H
#define COBL_HARDWARE_H 1

#include "F2837xD_device.h"
#include "device.h"
#include "F2837xD_GlobalPrototypes.h"
#include "F2837xD_cputimervars.h"
#include "F2837xD_Ipc_defines.h"
#include "F2837xD_Ipc_drivers.h"


/* Prototypes
*---------------------------------------------------------------------------*/
void initBaseHardware(void);
void initHardware(void);
void toggleWD(void);

/* Prototypes from the additional files
*---------------------------------------------------------------------------*/




#endif /* COBL_HARDWARE_H */ 

