/*
* cobl_flashconfig.c
*
* Copyright (c) 2015-2020 emotas embedded communication GmbH
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
* \brief cobl_flashconfig.c
*
* Flash config, like different image regions
*/



/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <stddef.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <user_config.h>
#include <cobl_type.h>
#include <cobl_user.h>
#include <cobl_application.h>
#include <cobl_call.h>
#include <cobl_flash.h>



/* constant definitions
---------------------------------------------------------------------------*/
#if COBL_DOMAIN_COUNT > 2
# error "Only 2 regions defined!"
#endif

/* local defined data types
---------------------------------------------------------------------------*/
typedef struct {
	FlashAddr_t domainStartAddr;
	U32 domainSize;
	U32 configBlockSize;	
} flashImageConfiguration_t;

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
#ifdef COBL_DOMAIN_COUNT
/* for use with 2 domains only - words */
static const flashImageConfiguration_t flashImage[2] = {
		{ 0x084000, 0x1C000ul, 0x100 },  /* 192 K */
		{ 0x0A0000, 0x1C000ul, 0x08 }       /*64 K */
};



	#if COBL_DOMAIN_COUNT > 2
	#error "Only 2 domains adapted!"
	#endif

/***************************************************************************/
/**
 * \brief userGetApplStartAdr
 *
 * start address of a specific region
 */
FlashAddr_t userGetApplStartAdr(
		U8 nbr /**< region 0..n */
	)
{
FlashAddr_t retVal = 0;

	if (nbr < COBL_DOMAIN_COUNT) {
		retVal = flashImage[nbr].domainStartAddr;
	}
	return(retVal);
}

/***************************************************************************/
/**
 * \brief userGetDomainSize in word for internal use
 *
 * size of a specific region (word)
 *
 */
U32 userGetDomainSize(
        U8 nbr /**< region 0..n */
    )
{
U32 retVal = 0u;

    if (nbr < COBL_DOMAIN_COUNT) {
        retVal = flashImage[nbr].domainSize;
    }
    return(retVal);
}


/***************************************************************************/
/**
 * \brief userGetDomainSizeByte
 *
 * size of a specific region (Byte) for CANopen use
 *
 */
U32 userGetDomainSizeByte(
		U8 nbr /**< region 0..n */
	)
{
U32 retVal = 0u;

	if (nbr < COBL_DOMAIN_COUNT) {
		retVal = userGetDomainSize(nbr) * 2;
	}	
	return(retVal);
}

/***************************************************************************/
/**
 * \brief userGetConfigSize
 *
 * size of the CRC Config block of a specific region
 */
U32 userGetConfigSize(
		U8 nbr /**< region 0..n */
	)
{
U32 retVal = 0u;

	if (nbr < COBL_DOMAIN_COUNT) {
		retVal = flashImage[nbr].configBlockSize;
	}	
	return(retVal);
}

#endif /* COBL_DOMAIN_COUNT */
