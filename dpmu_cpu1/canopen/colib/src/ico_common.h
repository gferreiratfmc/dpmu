/*
* ico_common.h - contains internal defines
*
* Copyright (c) 2017-2021 emotas embedded communication GmbH
*
*-------------------------------------------------------------------
* $Id: ico_common.h 38093 2021-10-01 10:21:16Z boe $
*
*-------------------------------------------------------------------
*
*
*
*/

/**
* \file
* \brief common defines
*/

#ifndef ICO_COMMON_H
#define ICO_COMMON_H 1

/* datatypes */
#include "co_common.h"


/* function prototypes */

BOOL_T icoFdMode(void);
MP_PROTOCOL_TYPE_T icoProtocolType(void);

#endif /* ICO_COMMON_H */
