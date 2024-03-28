/* global types
*
* Copyright (c) 2012-2019 emotas embedded communication GmbH
*-------------------------------------------------------------------
* SVN  $Id: cobl_type.h 29606 2019-10-23 10:25:58Z hil $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \file
* \brief general type definitions
*
*/

#include <stdint.h>

#ifndef COBL_TYPE_H
#define COBL_TYPE_H 1

typedef unsigned char U8;	/**< 8bit type - DSP 16bit (only 8bit used) */
typedef unsigned short U16; /**< 16bit type */
//typedef unsigned int U32; /**< 32bit type on 64bit linux */
typedef unsigned long U32; /**< 32bit type  */

/* default definitions for Big Endian CPUs */
#ifndef COBL_REVERSE_U32
  #ifdef COBL_BIG_ENDIAN
    #define COBL_REVERSE_U32(x)	((U32)(((U32)(x) >> 24) | (((x) & 0x00ff0000ul) >> 8) | (((x) & 0x0000ff00ul) << 8) | (((x) & 0x000000fful) << 24))) 		
  #else /* COBL_BIG_ENDIAN */
    #define COBL_REVERSE_U32(x) (x)
  #endif /* COBL_BIG_ENDIAN */
#endif /*  !COBL_REVERSE_U32 */

#ifndef COBL_REVERSE_U16
  #ifdef COBL_BIG_ENDIAN
    #define COBL_REVERSE_U16(x)	((((U16)(x) & 0xff00) >> 8) | (((x) & 0x00ff) << 8)) 
  #else /* COBL_BIG_ENDIAN */
    #define COBL_REVERSE_U16(x) (x)
  #endif /* COBL_BIG_ENDIAN */
#endif /* !COBL_REVERSE_U16 */

typedef U8 Flag_t; /**< small numbers, but sometime are greater types better */
#if defined(__linux__) || defined(_WIN32)
typedef uintptr_t FlashAddr_t; /**< integer value of the flash address */
#else
typedef unsigned long FlashAddr_t; /**< integer value of the flash address */
#endif
/** general return value of the bootloader */
typedef enum {
	COBL_RET_OK=0,	/**< OK */
	COBL_RET_CAN_BUSY, /**< CAN is busy, e.g. cannot transmit */

	COBL_RET_BUSY, /**< functionality is busy/already running */
	COBL_RET_NOTHING, /**< nothing is required to do */

	COBL_RET_CRC_WRONG, /**< wrong CRC sum */

	COBL_RET_FLASH_END, /**< action was stopped */
	COBL_RET_FLASH_ERROR, /**< general error */

	COBL_RET_CALLBACK_READY, /**< callback has answer sent */

	COBL_RET_USB_WRONG_FORMAT, /**< format error */
	COBL_RET_IMAGE_WRONG,/* flash image has incorrect parameter */
	COBL_RET_ERROR /**< general error */
} CoblRet_t;

/* Support CANopen Types */
typedef U8 UNSIGNED8;
typedef U16 UNSIGNED16;
typedef U32 UNSIGNED32;

#define COBL_COMMAND_START 	0u 		// call application
#define COBL_COMMAND_BL 	1u		// stay in bootloader
#define COBL_COMMAND_BACKDOOR 	2u	// activate backdoor

typedef enum {
	COBL_BACKDOOR_DEACTIVATE, /* backdoor inactive */
	COBL_BACKDOOR_ACTIVATE, /* activate the backdoor */
	COBL_BACKDOOR_USED /* backdoor is using by the user */
} CoblBackdoor_t;

typedef struct CoblObjInfo  {
	UNSIGNED32	objSize;
	void *		pObjData;
} CoblObjInfo_t;


#endif /* COBL_TYPE_H */
