/*
* cobl_user.c
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
* \brief cobl_user.c
*
* User specific functionality
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


/* constant definitions
---------------------------------------------------------------------------*/

/* local defined data types
---------------------------------------------------------------------------*/

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
 * \brief userGetNodeId - returns a valid nodeid
 *
 */
U8 userGetNodeId(void)
{

	return 126u;
}

/***************************************************************************/
/**
 * \brief userGetBitrate - returns a valid bitrate
 *
 * valid values are:
 * 		125 .. 125kbit/s
 *      250 .. 250kbit/s
 *      500 .. 500kbit/s
 *     1000 .. 1Mbit/s
 * \retval
 *  bitrate in kbit/s
 */
U16 userGetBitrate(void)
{
#ifdef CFG_CAN_BACKDOOR
CoblBackdoor_t backdoorState;

	backdoorState = getBackdoorCommand();
	if (   (backdoorState == COBL_BACKDOOR_ACTIVATE) /* backdoor active */
		|| (backdoorState == COBL_BACKDOOR_USED) ) /* backdoor was used */
	{
		return 250u; /* should be fix */
	}
#endif

	/* user bitrate, e.g. from eeprom */
	return 250u;
}


/***************************************************************************/
/* Object dictionary values                                                */
/***************************************************************************/
#ifdef COBL_OD_1000_0_FCT
U32 userOd1000(void)
{
	return 0x424F4F54ul;
}
#endif /* COBL_OD_1000_0_FCT */

/***************************************************************************/
/*
* 0x1018:1 vendor id
* Configure your vendor id. 
* It is not allowed to change the other 0x1018 parameter 
* as long as you do not configure your own vendor id.
* If you create customer adaptations remove the emtas vendor!
* Configure your own vendor or 0.
*/
#ifdef COBL_OD_1018_1_FCT
U32 userOd1018_1(void)
{
	return 0x319ul;
}
#endif /* COBL_OD_1018_1_FCT */



/***************************************************************************/
/* 
* 1018:2 productcode
* The productcode is vendorspecific - configure 0x1018:1, too.
* If the feature COBL_CHECK_PRODUCTID is used this value is used to compare 
* with the Config header information.
*/
#ifdef COBL_OD_1018_2_FCT
U32 userOd1018_2(void)
{
	return (1001ul << 16) | (63ul << 8) | 1 ;  /* e.g. 1001-63-1 */
}
#endif /* COBL_OD_1018_2_FCT */


/***************************************************************************/
/*
* 0x1018:3 revision 
*/
#ifdef COBL_OD_1018_3_FCT
U32 userOd1018_3(void)
{
	return (1ul << 16) | 42ul; /* e.g. version 1.42 */
}
#endif /* COBL_OD_1018_3_FCT */


/***************************************************************************/
/*
* 0x1018:4 serial number
*/
#ifdef COBL_OD_1018_4_FCT
U32 userOd1018_4(void)
{
	return 0;
}
#endif /* COBL_OD_1018_4_FCT */

/***************************************************************************/
/*
* example code for a customer specific object
* the example show the access to the Config header information
*/
#ifdef COBL_OD_5F00_0_FCT
U32 userOd5F00_0(void)
{
const ConfigBlock_t * pConfig;

	pConfig = applConfigBlock();

	return COBL_REVERSE_U32(pConfig->swVersion);
}
#endif /* COBL_OD_5F00_0_FCT */


#ifdef COBL_OD_U32_READ_FCT
/***************************************************************************/
/*
* read U32 objects
* function, that can be used for different U32 objects
*
*/
U32 userOdU32ReadFct(U16 index, U8 subIndex)
{
    if ((index == 0x1200u) && (subIndex == 1u))  {
        return 0x600u + GET_NODEID();
    } else
    if ((index == 0x1200u) && (subIndex == 2u))  {
        return 0x580u + GET_NODEID();
    }
    return 0ul;
}
#endif /* COBL_OD_U32_READ_FCT */

/***************************************************************************/
/*
* object callback for segmented read objects
*/
#ifdef COBL_SDO_SEG_READ_TRANSFER_SUPPORTED
CoblObjInfo_t userSegReadFct(U16 index, U8 subIndex)
{
static const U8 deviceName[] = "emotas Bootloader device";
CoblObjInfo_t retVal;

    retVal.objSize = 0u;
    retVal.pObjData = NULL;

    if ((index == 0x1008u) && (subIndex == 0u))  {
        retVal.objSize = 24ul;
        retVal.pObjData = (void*)&deviceName[0];
    }
    return(retVal);
}
#endif /* COBL_SDO_SEG_READ_TRANSFER_SUPPORTED */
