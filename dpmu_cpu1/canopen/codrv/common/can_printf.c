/*
* can_printf - printf over CAN
*
* Copyright (c) 2013-2019 emotas embedded communication GmbH
*-------------------------------------------------------------------
* $Id: can_printf.c 33084 2020-08-12 14:40:12Z ro $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \brief printf over CAN
*
* \file can_printf.c 
*
* Requirements:
* 1) additional COB
* 2) more TX queue buffers
* 3) in gen_define.h
*      #define CAN_PRINTF
*      #define CO_CAN_DEBUG_SUPPORTED
*      #define printf can_printf
*      extern int can_printf( const char *_format, ...);
* 
*
*/


/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <gen_define.h>

#ifdef NO_PRINTF
#else

#ifdef CAN_PRINTF

#include <co_datatype.h>
#include <co_candebug.h>

# ifdef CO_CAN_DEBUG_SUPPORTED
# else /* CO_CAN_DEBUG_SUPPORTED */
#   error "CAN_PRINTF req. CO_CAN_DEBUG_SUPPORTED!"
# endif /* CO_CAN_DEBUG_SUPPORTED */


#ifndef PRINTF_COBID
# define PRINTF_COBID 0x580
#endif
/*
* can_printf - format like printf
*
* Currently fix for 8 Byte.
*
* Note: Required Stack size!
*
*/
int can_printf( const char *_format, ...)
{
va_list _ap;
int rval;
static char buf[80] = {0};
int idx = 0;

	va_start(_ap, _format);

	vsnprintf(buf, 79, _format, _ap);
	rval = strlen(buf);

	while (rval > 8) {
		coCanDebugPrint(PRINTF_COBID, 8, (const unsigned char *)&buf[idx]);
		rval -= 8;
		idx += 8;
	} 

	if (rval > 0) {
		coCanDebugPrint(PRINTF_COBID, rval, (const unsigned char *)&buf[idx]);
	}

	va_end(_ap);
	return (rval);
}

#endif
#endif
