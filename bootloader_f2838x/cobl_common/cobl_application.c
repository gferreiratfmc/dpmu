/*
* cobl_application.c
*
* Copyright (c) 2012-2019 emotas embedded communication GmbH
*-------------------------------------------------------------------
* SVN  $Id: cobl_application.c 31961 2020-05-06 08:42:41Z ro $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \file
* \brief cobl_application.c - application related parts
*
* application related parts
*/



/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <stdio.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <user_config.h>
#include <cobl_type.h>
#include <cobl_application.h>
#include <cobl_crc.h>
#include <cobl_flash.h>
#include <cobl_call.h>


/* constant definitions
---------------------------------------------------------------------------*/

/* local defined data types
---------------------------------------------------------------------------*/
/** Config Block address */
#define CONFIG_BLOCK_ADDRESS(x) FLASH_APPL_START((x))
/* Config Block size - moved to user_config.h */
//#define CFG_CONFIG_BLOCK_SIZE	256

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

/********************************************************************/
/**
 * \brief first flash address of the application
 * Often it is the address after the configuration block.
 *
 * \retval
 * first address of the application within the flash (Begin of the Vector table)
 */
FlashAddr_t applFlashStart(U8 nbr)
{
FlashAddr_t address;

	address = FLASH_APPL_START(nbr) + CFG_CONFIG_BLOCK_SIZE(nbr);
	return(address);
}

/********************************************************************/
/**
 * \brief first flash address after the application
 *
 * \retval
 * address after last address of the application within the Flash
 */
FlashAddr_t applFlashEnd(U8 nbr)
{
const ConfigBlock_t * pConfig = applConfigBlock(nbr);
FlashAddr_t address;

	address = applFlashStart(nbr) + COBL_REVERSE_U32(pConfig->applSize);
	return(address);
}

/********************************************************************/
/**
 * \brief checksum saved in flash
 *
 * \retval
 * 	saved checksum
 */
U16 applChecksum(U8 nbr)
{
const ConfigBlock_t * pConfig = applConfigBlock(nbr);
U16 retval;

	retval = COBL_REVERSE_U16(pConfig->crcSum);
	return(retval);
}

/********************************************************************/
/**
* \brief check some general setting
 *
 * Check for a correct flashed application.
 * The function check also that the bootloader configuration make sense.
 *
 * \retval COBL_STATE_OK
 * configuration seems to be ok
 * \retval COBL_RET_ERROR
 * wrong configuration in application flash or bootloader configuration
 *
 */
CoblRet_t applCheckConfiguration(U8 nbr)
{
FlashAddr_t flashStart = FLASH_APPL_START(nbr);
FlashAddr_t flashEnd   = FLASH_APPL_END(nbr);
FlashAddr_t configBlockStart = CONFIG_BLOCK_ADDRESS(nbr);
U32 maxApplSize = FLASH_APPL_END(nbr) - FLASH_APPL_START(nbr) + 1;
const ConfigBlock_t * pConfig = applConfigBlock(nbr);

#ifdef COBL_ECC_FLASH
	if (cobl_command[ECC_ERROR_IDX] == ECC_ERROR_VAL)  {
		return(COBL_RET_ERROR);
	}
#endif

	if (configBlockStart < flashStart)  {
		return(COBL_RET_ERROR);
	}

	if (configBlockStart > (flashEnd - CFG_CONFIG_BLOCK_SIZE(nbr)) )  {
		return(COBL_RET_ERROR);
	}

	if (COBL_REVERSE_U32(pConfig->applSize) > maxApplSize)  {
		return(COBL_RET_ERROR);
	}

	if (COBL_REVERSE_U32(pConfig->applSize) == 0)  {
		return(COBL_RET_ERROR);
	}

	if (COBL_REVERSE_U16(pConfig->crcSum) == 0)  {
		return(COBL_RET_ERROR);
	}

	return(COBL_RET_OK);
}


/********************************************************************/
/**
* \brief check the CRC sum of the application in flash
 *
 * \retval COBL_RET_OK
 * checksum correct
 *
 * \retval COBL_RET_ERROR
 * configuration wrong, e.g. no application
 *
 * \retval COBL_RET_CRC_WRONG
 * error in flash
 */
CoblRet_t applCheckChecksum(U8 nbr)
{
CoblRet_t ret;
FlashAddr_t applStart;
U16 crcVal;
const ConfigBlock_t * pConfig = applConfigBlock(nbr);

	ret = applCheckConfiguration(nbr);
	if (ret != COBL_RET_OK)  {
		return(ret);
	}

	applStart = applFlashStart(nbr);
	crcVal = crcCalculation((const U8 *)applStart, CRC_START_VALUE, COBL_REVERSE_U32(pConfig->applSize));
	if (crcVal != COBL_REVERSE_U16(pConfig->crcSum))  {
		return(COBL_RET_CRC_WRONG);
	}

	return(COBL_RET_OK);
}

#ifdef COBL_CHECK_PRODUCTID
#  ifdef COBL_OD_1018_1_FCT
#  else
#error "user function required - COBL_OD_1018_1_FCT"
#  endif
#  ifdef COBL_OD_1018_2_FCT
#  else
#error "user function required - COBL_OD_1018_2_FCT"
#  endif

/********************************************************************/
/**
* \brief check 0x1018 parameter, if available
*
* \retval COBL_RET_OK
* Image parameter ok
*
* \retval COBL_RET_ERROR
* configuration wrong, e.g. no application
*
* \retval COBL_RET_IMAGE_WRONG
* wrong image parameter
*/
CoblRet_t applCheckImage(void)
{
CoblRet_t ret;
const ConfigBlock_t * pConfig;
U32 val1018_1;
U32 val1018_2;

	ret = applCheckConfiguration(0);
	if (ret != COBL_RET_OK)  {
		return(ret);
	}

	//ret = COBL_RET_OK;
	val1018_1 = userOd1018_1();
	val1018_2 = userOd1018_2();

	pConfig = applConfigBlock(0);
	if ((COBL_REVERSE_U32(pConfig->od1018Vendor) != val1018_1)
		|| (COBL_REVERSE_U32(pConfig->od1018Product) != val1018_2))
	{
		ret = COBL_RET_IMAGE_WRONG;
	}

	return(ret);
}

#endif /* COBL_CHECK_PRODUCTID */

/********************************************************************/
const ConfigBlock_t * applConfigBlock(U8 nbr)
{
ConfigBlock_t * pConfig = (ConfigBlock_t *)CONFIG_BLOCK_ADDRESS(nbr);

	return(pConfig);
}
