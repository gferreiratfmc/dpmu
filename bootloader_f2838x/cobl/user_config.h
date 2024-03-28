/* user configuration
*
* Copyright (c) 2017-2020 emotas embedded communication GmbH
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
* \brief user configuration
*/


#ifndef USER_CONFIG_H
#define USER_CONFIG_H 1

/*--------------------------------------------------------------*/
/* Development settings                                         */
/*--------------------------------------------------------------*/
//#define NO_BOOT
//#define NO_APPL
//#define TEST_GENERATE_ECC_ERROR 1

/* Hardware Header
*---------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/* Type specific settings - before co_type.h                    */
/*--------------------------------------------------------------*/

/* Big Endian - set before include cobl_type.h */
//#define COBL_BIG_ENDIAN 1
#ifdef COBL_BIG_ENDIAN
#endif

/* Bootloader related Header
*---------------------------------------------------------------*/
#include <cobl_type.h>
#include <cobl_user.h>
/*--------------------------------------------------------------*/
/* Number of flash domains                                      */
/*--------------------------------------------------------------*/
//#define COBL_DOMAIN_COUNT 2

/*--------------------------------------------------------------*/
/* Backdoor                                                     */
/*--------------------------------------------------------------*/
// only in case a Backdoor is supported (default: no)
//#define CFG_CAN_BACKDOOR 1
//#define CFG_BACKDOOR_TIME 300 /* 300ms - 100ms tick!*/

/*--------------------------------------------------------------*/
/* SDO settings                                                 */
/*--------------------------------------------------------------*/

/* COBL_SDO_BLOCK_TRANSFER_SUPPORTED - default: on
 * enable the support of the SDO Block transfer
*/
#define COBL_SDO_BLOCK_TRANSFER_SUPPORTED 1

/* BL_BLOCK_SIZE - required for COBL_SDO_BLOCK_TRANSFER_SUPPORTED
 * number of messages per block
*/
#ifdef COBL_SDO_BLOCK_TRANSFER_SUPPORTED
#  define BL_BLOCK_SIZE 64u
#endif

/* COBL_SDO_SEG_READ_TRANSFER_SUPPORTED - default: off
* enable the support of the SDO segmented read transfer
*/
//#define COBL_SDO_SEG_READ_TRANSFER_SUPPORTED 1


#define CONFIG_DSP 1

/*--------------------------------------------------------------*/
/* Flash settings                                               */
/*--------------------------------------------------------------*/
   /* BL size in bytes */
#  define FLASH_BL_SIZE	(2 * 16 * 1024ul)


#ifdef COBL_DOMAIN_COUNT
extern U32 userGetDomainSize(U8 nbr);
extern U32 userGetDomainSizeByte(U8 nbr);
	#define CFG_MAX_DOMAINSIZE(x) 	userGetDomainSizeByte((x))
#else /* COBL_DOMAIN_COUNT */
 	/** Application Flash size (bytes) */
   #define CFG_MAX_DOMAINSIZE(x) (2 * ((256 * 1024ul) - FLASH_BL_SIZE))
#endif /* COBL_DOMAIN_COUNT */

/** flash write size (byte) */
	/* page 
	 * - must be greater than one CAN Telegram 
	 * - must be multiple of the real flash page 
	 * - 1 or multiple flash pages must be one erase page
	 */
#ifdef COBL_SDO_BLOCK_TRANSFER_SUPPORTED
#  define FLASH_WRITE_SIZE 	(BL_BLOCK_SIZE * 7u)
#else /* COBL_SDO_BLOCK_TRANSFER_SUPPORTED */
#  define FLASH_WRITE_SIZE 	16
#endif /* COBL_SDO_BLOCK_TRANSFER_SUPPORTED */

/** flash erase page size
 * without definition the flashdriver use different sizes (words)
 */
#define FLASH_ERASE_SIZE	(8ul*1024ul)

#define FLASH_EMPTY			0xFF

/**
 * Domain start address
 * Image 0 .. Applikation
 * Image 1..n additional images
 */
#ifdef COBL_DOMAIN_COUNT
    extern FlashAddr_t userGetApplStartAdr(U8 nbr);
    /* get the start addresses of each domain e.g. from a user function */
    #define FLASH_APPL_START(x) 	userGetApplStartAdr((x))
    #define FLASH_APPL_END(x)		((FLASH_APPL_START((x))) + ((userGetDomainSize((x))) - 1)) 	/* e.g. 128k == 0x8000 - 1 */
#else /* COBL_DOMAIN_COUNT */
    #define FLASH_APPL_START(x)		(0x084000)
    #define FLASH_APPL_END(x)		((FLASH_APPL_START((x))) + ((CFG_MAX_DOMAINSIZE((x)) / 2) - 1))
#endif /* COBL_DOMAIN_COUNT */



/*--------------------------------------------------------------*/
/* Configuration Block                                          */
/*--------------------------------------------------------------*/
#ifdef COBL_DOMAIN_COUNT
extern U32 userGetConfigSize(U8 nbr);
	#define CFG_CONFIG_BLOCK_SIZE(x) 	userGetConfigSize((x))
#else
/** Config Block size (words) - e.g. depend of alignment of the IRQ table */
#define CFG_CONFIG_BLOCK_SIZE(x)	256u
#endif

/*--------------------------------------------------------------*/
/* CPU specific                                                 */
/*--------------------------------------------------------------*/
/** call a software reset */
void softwareReset(void);
#define USER_RESET() softwareReset()  /* software reset */

/*--------------------------------------------------------------*/
/* bootloader/application adaptations                           */
/*--------------------------------------------------------------*/
/** Bootloader Node ID */
#if 1
# define GET_NODEID()	126
#else
//# include "cobl_type.h"
//# include "cobl_user.h"
# define GET_NODEID()	userGetNodeId()
#endif

#if 1
#define CFG_BITRATE()		125
//#define CFG_BITRATE()		500
#else
//# include "cobl_type.h"
//# include "cobl_user.h"
# define CFG_BITRATE()	userGetBitrate()
#endif



//#define COBL_OD_1000_0_FCT userOd1000
#define COBL_OD_1018_1_FCT userOd1018_1
#define COBL_OD_1018_2_FCT userOd1018_2
#define COBL_OD_1018_3_FCT userOd1018_3
//#define COBL_OD_1018_4_FCT userOd1018_4
//#define COBL_OD_5F00_0_FCT userOd5F00_0

//#define COBL_OD_U32_READ_FCT userOdU32ReadFct

#ifdef COBL_SDO_SEG_READ_TRANSFER_SUPPORTED
#define COBL_OD_SEG_READ_FCT userSegReadFct
#endif

/* COBL_SDO_PROG_START - default: on
* SDO 1F51:1 = 1 call application enabled 
*/
#define COBL_SDO_PROG_START 1 

/* COBL_CHECK_PRODUCTID - default on
* check the vendor id and product code of image and bootloader 
* generate_firmware must be used with --od1018_1 and --od1018_2 
*/
#define COBL_CHECK_PRODUCTID 1

/* COBL_ECC_FLASH - default: off
* On ECC flash sometimes it is not possible to use the BL internal 
* empty check function, because it creates a new ECC Trap.
* This define activate functionality to force an erase.
* Additional device specific adaptations required (trap implementation).
*/
#define COBL_ECC_FLASH 1

/* COBL_ERASE_BEFORE_FLASH - default: off
* => erase command required before flash 
* Typical enabled for COBL_ECC_FLASH.
* Note: If you abort the erase you have to erase it again before flash 
* You can use SDO_RESPONSE_AFTER_ERASE to prevent an Erase abort
* created by the next command.
*/
#define COBL_ERASE_BEFORE_FLASH 1

/* COBL_DONT_ABORT_FLASH - default: off
* => do not abort flash activity 
* The firmware download returns an error, if erase is active.
* Without this definition the erase is aborted. 
* The definition can be disabled in case, that the Flash 
* is erasing during download.
*/
#define COBL_DONT_ABORT_FLASH 1

/* SDO_RESPONSE_AFTER_ERASE - default: off
* SDO response at the end of erase.
* Without this setting the SDO returns immediately.
* Check 0x1F57 about the current state.
*/
//#define SDO_RESPONSE_AFTER_ERASE 1



/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/



/** Debug code enabled */
//#define COBL_DEBUG 1

/** send additional Emergencies for debug */
#define COBL_DEBUG_EMCY 1
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
#ifdef COBL_DEBUG_EMCY
#  define COBL_EMCY_PRODUCER 1
#  define SEND_EMCY(code, arg1, arg2, arg3) canopenSendEmcy(code, arg1, arg2, arg3);
#else
#  define SEND_EMCY(code, arg1, arg2, arg3)
#endif


#define NO_PRINTF
/*
* all printf() should only used with COBL_DEBUG.
* But in case printf() is used in other way, the Define
* NO_PRINTF should disable the command printf().
*/
#ifdef NO_PRINTF
# define printf(...)
#else /* NO_PRINTF */
# define PRINTF printf
#endif /* NO_PRINTF */

#ifdef COBL_DEBUG
# include <stdio.h>
# include "cobl_debug.h"
#endif

#ifdef PRINTF0
#else
# define PRINTF0(fmt)
# define PRINTF1(fmt, arg1)
# define PRINTF2(fmt, arg1, arg2)
# define PRINTF3(fmt, arg1, arg2, arg3)
#endif

#endif /* USER_CONFIG_H */
