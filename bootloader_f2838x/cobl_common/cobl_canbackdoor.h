/*
* cobl_canbackdoor.h
*
* Copyright (c) 2015-2019 emotas embedded communication GmbH
*-------------------------------------------------------------------
* SVN  $Id: cobl_canbackdoor.h 29606 2019-10-23 10:25:58Z hil $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \file
* \brief Bootloader backdoor using CAN
*
* The Bootloader activate the CAN during startup and wait a short time
* for a backdoor message. Without this message the application is calling.
*
*/



#ifndef COBL_CANBACKDOOR_H
#define COBL_CANBACKDOOR_H 1

void backdoorCheck(void);


#endif /* COBL_CALL_H */

