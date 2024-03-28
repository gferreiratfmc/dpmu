/*
* cobl_debug.h
*
* Copyright (c) 2012-2019 emotas embedded communication GmbH
*-------------------------------------------------------------------
* SVN  $Id: cobl_debug.h 29606 2019-10-23 10:25:58Z hil $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \file
* \brief additional debug functionality
*
*/

#ifndef COBL_DEBUG_H
#define COBL_DEBUG_H 1

void cobl_puts(const char * s);
void cobl_hex8(U8 c);
void cobl_hex16(U16 i);
void cobl_hex32(U32 l);
void cobl_ptr(const void * const p);
void cobl_dump(const U8 * pStart, U16 iCnt);
void cobl_printDatatypes(void);

#ifdef PRINTF
# define PRINTF0(fmt) PRINTF(fmt)
# define PRINTF1(fmt, arg1) PRINTF(fmt, arg1)
# define PRINTF2(fmt, arg1, arg2) PRINTF(fmt, arg1, arg2)
# define PRINTF3(fmt, arg1, arg2, arg3) PRINTF(fmt, arg1, arg2, arg3)
#else /* PRINTF */
# define PRINTF0(fmt)
# define PRINTF1(fmt, arg1)
# define PRINTF2(fmt, arg1, arg2)
# define PRINTF3(fmt, arg1, arg2, arg3)
#endif /* PRINTF */

#endif /* COBL_DEBUG_H */
