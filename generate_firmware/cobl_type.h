/* global types
*
* Copyright 2012 emtas GmbH
*
* $ID: $
*
*
* Changelog:
*
*
*/

/**
* \file
* \brief general type definitions
*/

#ifndef COBL_TYPE_H
#define COBL_TYPE_H 1

typedef unsigned char U8;	/**< 8bit type */
typedef unsigned short U16; /**< 16bit type */
typedef unsigned int U32; /**< 32bit type on 64bit linux */

typedef U8 Flag_t; /**< small numbers, but sometime are greater types better */
typedef unsigned long FlashAddr_t; /**< integer value of the flash address */

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

	COBL_RET_ERROR /**< general error */
} CoblRet_t;

#endif /* COBL_TYPE_H */
