/*
* cobl_flash_lowlvl.h
*
* Copyright (c) 2018-2020 emotas embedded communication GmbH
*-------------------------------------------------------------------
* SVN  $Id: cobl_flash_lowlvl.h 29606 2019-10-23 10:25:58Z hil $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \file
* \brief Low Level functionality of the Flash 
*
*/

#ifndef COBL_FLASH_LOWLVL_H
#define COBL_FLASH_LOWLVL_H 1

/* Macros
* --------------------------------------------------------------------*/
/* number of Byte that are flashed at the same time */
#define FLASH_ONE_CALL_SIZE 16u

/* Type definitions
* --------------------------------------------------------------------*/

/* flash sector definition */
typedef struct {
	FlashAddr_t baseAddress;
	U32 sectorSize;
	//Fapi_FlashSectorType sectorNumber;
	//Fapi_FlashBankType bankNumber;
} FLASH_SECTOR_T;


/* Constants
* --------------------------------------------------------------------*/

/* Variables
* --------------------------------------------------------------------*/

/* Prototypes
* --------------------------------------------------------------------*/
#ifdef FLASH_ERASE_SIZE
#else /* FLASH_ERASE_SIZE */
const FLASH_SECTOR_T * llsectorParams(FlashAddr_t address);
U32 llsectorSize(FlashAddr_t address);
#endif /* FLASH_ERASE_SIZE */

CoblRet_t llflashInit(U8 nbr);
CoblRet_t lleraseOnePage(FlashAddr_t address);
CoblRet_t llflashOnePage(FlashAddr_t address, const U16 * pSrc);
U16 llflashOneCallSize(void);


#endif /* COBL_FLASH_H_ */
