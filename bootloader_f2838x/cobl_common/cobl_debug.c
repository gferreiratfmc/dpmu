/*
* cobl_debug.c
*
* Copyright (c) 2012-2019 emotas embedded communication GmbH
*-------------------------------------------------------------------
* SVN  $Id: cobl_debug.c 29606 2019-10-23 10:25:58Z hil $
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



/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>


/* header of project specific types
---------------------------------------------------------------------------*/
#include <user_config.h>

#ifdef COBL_DEBUG
#include <cobl_type.h>
#include <cobl_can.h>
#include <cobl_canopen.h>
//#include <cobl_debug.h>

/* constant definitions
---------------------------------------------------------------------------*/

/* local defined data types
---------------------------------------------------------------------------*/
//typedef unsigned long uintptr_t;


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
* \brief print a string
*
*/

void cobl_puts(
		const char * s
	)
{
	(void)s;
	printf("%s", s);
}

/***************************************************************************/
/**
* \brief print a 8bit hex
*
*/

void cobl_hex8(
		U8 c
	)
{
	(void)c;
	printf("%02x", (unsigned int)c);
}

/***************************************************************************/
/**
* \brief print a 16bit hex
*
*/

void cobl_hex16(
		U16 i
	)
{
	(void)i;
	printf("%04x", (unsigned int)i);
}

/***************************************************************************/
/**
* \brief print a 32bit hex
*
*/

void cobl_hex32(
		U32 l
	)
{
	(void)l;
	printf("%08lx", (unsigned long int)l);
}

/***************************************************************************/
/**
* \brief print a pointer hex
*
*/

void cobl_ptr(
		const void * const p
	)
{
	(void)p;
	printf("%p", p);
}


/***************************************************************************/
/**
* \brief print a memory dump
*
*/
void cobl_dump(
		const U8 * pStart, /**< start pointer */
		U16 iCnt /**< number of bytes */
	)
{
U16 i;
U8 fAddr = 1; /* print Address */

	for( i = 0; i < iCnt; i++)  {
		/* new line after every 16 Bytes */
		if (((uintptr_t)&pStart[i] & 0x0F) == 0)  {
			fAddr = 1;
		}

		if (fAddr != 0)  {
			cobl_puts("\n");
			cobl_ptr(&pStart[i]);
			cobl_puts(": ");
			fAddr = 0;
		}

		cobl_hex8(pStart[i]);
		cobl_puts(" ");
	}

	cobl_puts("\n");
}

/***************************************************************************/
/**
* \brief print size of datatypes
*
*/
void cobl_printDatatypes(void)
{
	printf("Test\n");
	printf("U8  - %d\n", (int)sizeof(U8));
	printf("U16 - %d\n", (int)sizeof(U16));
	printf("U32 - %d\n", (int)sizeof(U32));

	printf("CanData_t - %d\n", (int)sizeof(CanData_t));
	printf("CoOd_t - %d\n", (int)sizeof(CoOd_t));

	/* printTable();*/

}

#endif


