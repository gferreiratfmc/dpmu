/*
* cobl_flash_lowlvl.c
*
* Copyright (c) 2012-2020 emotas embedded communication GmbH
*-------------------------------------------------------------------
* SVN  $Id: cobl_flash_lowlvl.c 31983 2020-05-07 16:15:38Z ro $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \file
* \brief cobl_flash_lowlvl.c
* Low Level Flash functionality - direct access to Flash
* erase, flash
*
*/



/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <user_config.h>
#include <cobl_type.h>
#include <cobl_debug.h>
#include <cobl_flash.h>
#include <cobl_flash_lowlvl.h>
#include <cobl_call.h>

/* for LED support */
#include <cobl_hardware.h>
/* for EMCY support */
#include <cobl_canopen.h>

#include <device.h>
#include <F021_F2837xD_C28x.h>

#ifdef __TI_COMPILER_VERSION__
    #if __TI_COMPILER_VERSION__ >= 15009000
        #define ramFuncSection ".TI.ramfunc"
    #else
        #define ramFuncSection "ramfuncs"
    #endif
#endif

//#define CPUCLK_FREQUENCY 200 /* 200 MHz Systemfrequency*/
#define CPUCLK_FREQUENCY (DEVICE_SYSCLK_FREQ / 1000000u)

/* constant definitions
---------------------------------------------------------------------------*/
/* number of Byte that are flashed at the same time (16 bytes -> 8 words) */
#define FLASH_ONE_CALL_SIZE 16u

/* constant definitions
---------------------------------------------------------------------------*/

#ifdef DECRYPT_INIT
#else /* DECRYPT_INIT */
/* DECRYPT_INIT - reset decrypt functionality
 * \param nbr
 * domain number
 * \returns void
 */
#  define DECRYPT_INIT(nbr)
#endif /* DECRYPT_INIT */

#ifdef DECRYPT_MEMORY
#else /* DECRYPT_MEMCPY */
/* DECRYPT_MEMORY - decrypt a small buffer
 * The entire memory is decrypted by calling this function frequently.
 * \param addr
 *	later flash address location
 * \param pdest 
 * destination pointer (U8 *)
 * \param psrc
 * source pointer (const U8 *)
 * \param ssize
 * number of byte of the current source memory block
 * \returns void 
 */
#  define DECRYPT_MEMORY(addr, pdest, psrc, size) inline_decrypt_memory(addr, pdest, psrc, size)
static inline void inline_decrypt_memory(
		FlashAddr_t addr,
		U8 * pdest,
		const U8 * psrc,
		size_t size
	)
{		
	(void)addr;
	memcpy(pdest, psrc, size);
}
#endif /* DECRYPT_INIT */


/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
static U8 llcheckErasePage(FlashAddr_t address);
static CoblRet_t llcheckAddress(FlashAddr_t address);


/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/

#ifdef FLASH_ERASE_SIZE
#else /* FLASH_ERASE_SIZE */
/***************************************************************************/
/**
* \brief get pointer to the information of the current flash page
*
* \returns
*	pointer to the page information
*/
const FLASH_SECTOR_T * llsectorParams(
		FlashAddr_t address	/* address */
	)
{
static const U16 maxIndex = sizeof(sectorArray) / sizeof(FLASH_SECTOR_T);
U16 i;
FlashAddr_t saddr;
U32 ssize;

	for (i = 0; i < maxIndex; i++)  {
		saddr = sectorArray[i].baseAddress;
		ssize = sectorArray[i].sectorSize;

		/* for comatibility with old tables */
		if (ssize == 0)  {
			return(NULL);
		}
		/* check range */
		if ((address >= saddr) && (address < (saddr + ssize)))
		{
			return(&sectorArray[i]);
		}
	}

	return(NULL);
}

/***************************************************************************/
/**
* \brief word size of flash pages
*
* The different Flash pages have different size.
*
* \returns
*	page size in word
*/
U32 llsectorSize(
		FlashAddr_t address	/* address */
	)
{
const FLASH_SECTOR_T * pParam;
U32 size;

	pParam = llsectorParams(address);
	if (pParam == NULL)  {
		return(0u);
	}

	size = pParam->sectorSize;
	return(size);
}

#endif  /* FLASH_ERASE_SIZE */


/***************************************************************************/
/**
 * \brief init the low level modul
 * e.g. copy the RAM functions from Flash to RAM
 *
*/
CoblRet_t llflashInit(
		U8 nbr
	)
{
    Fapi_StatusType fret;
    CoblRet_t ret = COBL_RET_OK;

	(void)nbr; /* avoid warning */
	DECRYPT_INIT(nbr);

	/* initialize flash API */
	EALLOW;
    fret = Fapi_initializeAPI((Fapi_FmcRegistersType *)F021_CPU0_REGISTER_ADDRESS, CPUCLK_FREQUENCY);
    if (fret != Fapi_Status_Success) {
        ret = COBL_RET_ERROR;
    }

    fret = Fapi_setActiveFlashBank(Fapi_FlashBank0);
    if (fret != Fapi_Status_Success) {
        ret = COBL_RET_ERROR;
    }

//    SeizeFlashPump();
    EDIS;

	return(ret);
}


/***************************************************************************/
/**
 * \brief check for an empty flash page
 *
 * An empty flash page must not be erased.
 *
 * \retval 0
 *	empty and error
 *
 *	\retval 1
 *	not empty
 */
#if defined(USE_COMMON_VARIANT)
/***************************************************************************/
static U8 llcheckErasePage(
		FlashAddr_t address /**< flash address */
	)
{
U8 * pFlash;
U16 i;

	/* printf("checkErasePage %p %lx\n", dummyFlash, address); */
	if (fFullEraseRequired != 0u)  {
		return(1u);
	}

	/* calc page start address */
	pFlash = (U8*)(address & ~(FlashAddr_t)(FLASH_ERASE_SIZE - 1));

	for( i = 0; i < FLASH_ERASE_SIZE; i++)  {
		if (*pFlash != FLASH_EMPTY)  {
			return(1);
		}
		pFlash ++;
	}

	return(0u);
}
#elif defined(FLASH_ERASE_SIZE)
/***************************************************************************/
static U8 llcheckErasePage(
		FlashAddr_t address /**< flash address */
	)
{
	/* optimization for U32 - constant page size */
#ifdef COBL_ECC_FLASH
#else
U32 * pFlash;
U16 i;

static const U32 empty = (U32)FLASH_EMPTY << 24
					| (U32)FLASH_EMPTY << 16
					| (U32)FLASH_EMPTY << 8
					| (U32)FLASH_EMPTY;
#endif /* COBL_ECC_FLASH */
	PRINTF1("checkErasePage %08lx\n", address);

	if (fFullEraseRequired != 0)  {
		PRINTF0("fFullEraseRequired set\n");
		return(1u);
	}

#ifdef COBL_ECC_FLASH
	if (fFlashErased[activeDomain] != 1u)  {
		PRINTF0("fFlashErased not set\n");
		return(1u); //erase required
	}
#else /* COBL_ECC_FLASH */

	/* calc page start address */
	pFlash = (U32*)(address & ~(FlashAddr_t)(FLASH_ERASE_SIZE - 1));

	for( i = 0; i < FLASH_ERASE_SIZE; i += 4)  {
		if (*pFlash != empty)  {
			return(1u);
		}
		pFlash ++;
	}
#endif /* COBL_ECC_FLASH */

	return(0u);
}
#else
/***************************************************************************/
static U8 llcheckErasePage(
		FlashAddr_t address /**< flash address */
	)
{
	/* optimization for U32 - constant page size */
U32 * pFlash;
U32 i;
static const U32 empty = (U32)FLASH_EMPTY << 24
					| (U32)FLASH_EMPTY << 16
					| (U32)FLASH_EMPTY << 8
					| (U32)FLASH_EMPTY;
const FLASH_SECTOR_T * pParam;
U32 eraseSize;

	if (fFullEraseRequired != 0)
	{
		return(1u);
	}

#ifdef COBL_ECC_FLASH
	if (fFlashErased[activeDomain] != 1u)
	{
		return(1u); //erase required
	}

#else /* COBL_ECC_FLASH */

	pParam = llsectorParams(address);
	if (pParam == NULL)  {
		return(0u); /* error */
	}

	eraseSize = pParam->sectorSize;
	pFlash = (U32 *)(pParam->baseAddress);

	for( i = 0u; i < eraseSize; i += 4u)  {
		if (*pFlash != empty)  {
			return(1u);
		}
		pFlash ++;
	}
#endif /* COBL_ECC_FLASH */

	return(0u);
}
#endif

/***************************************************************************/
/**
 * \brief erase one page
 *
 * Before a flash page is erased, it is checked if it is already erased.
 * If it is already erased, all bytes have the FLASH_EMPTY value,
 * the function returns without action.
 *
 *
 * \retval COBL_STATE_NOTHING
 * 		page is empty - no erase is required
 * \retval COBL_STATE_OK
 * 		erase was started correctly
 */
#pragma CODE_SECTION(lleraseOnePage,ramFuncSection);
CoblRet_t lleraseOnePage(
		FlashAddr_t address /**< flash address */
	)
{
CoblRet_t ret;
Fapi_StatusType fret;

	ret = llcheckAddress(address);
	if (ret != COBL_RET_OK)  {
		return(ret);
	}

	/* check, if erase is required */
	if (llcheckErasePage(address) == 1)  {
		/* start flash action */
		PRINTF1("erase 0x%lx\n", address);

		EALLOW;
        fret = Fapi_issueAsyncCommandWithAddress(Fapi_EraseSector, (uint32_t *)address);
        // // Wait until the erase operation is over
        while (Fapi_checkFsmForReady() != Fapi_Status_FsmReady){}
        EDIS;

        /* inform about erase progress */
        SEND_EMCY(0xff00u, 0x18u, (U16)(address & 0xFFFFu), (U16)(address >> 16));

        /* in case of error */
        if (fret != Fapi_Status_Success) {
            SEND_EMCY(0xff00u, 0x19u, (U16)(address & 0xFFFFu), 0u);
            return COBL_RET_FLASH_ERROR;
        }

		return(COBL_RET_OK);
	}

	return(COBL_RET_NOTHING);
}

/***************************************************************************/
/**
 * \brief flash one page
 *
 *
 */
#pragma CODE_SECTION(llflashOnePage,ramFuncSection);
CoblRet_t llflashOnePage(
		FlashAddr_t address, /**< flash address */
		const U16 * pSrc /**< data to flash */
	)
{
CoblRet_t ret;
Flag_t error = 0;
Fapi_StatusType fret;
U8 dest[FLASH_ONE_CALL_SIZE];

	ret = llcheckAddress(address);
	if (ret != COBL_RET_OK)  {
		return(ret);
	}

	DECRYPT_MEMORY(address,  &dest[0], (const U8*) pSrc, FLASH_ONE_CALL_SIZE);
	
	/* start flash action */
	PRINTF1("flash 0x%lx\n", address);

    EALLOW;
    fret = Fapi_issueProgrammingCommand( (uint32 *) address, (uint16 *) dest, FLASH_ONE_CALL_SIZE / 2,
                                         NULL, 0,  Fapi_AutoEccGeneration);

    // Wait until the Flash program operation is over
    while (Fapi_checkFsmForReady() != Fapi_Status_FsmReady){}

    if (fret != Fapi_Status_Success) {
        error = 1;
    }
    EDIS;


    /* send Emergency with flash error */
	if (error != 0u)  {
	    SEND_EMCY(0xff00u, 0x29u, (U16)(address & 0xFFFFu), error);
		return(COBL_RET_FLASH_ERROR);
	}

	return(COBL_RET_OK);
}

/***************************************************************************/
/**
 * \brief check if the address is within the FLASH application area
 *
 * This function checks, if address is inside of Application Flash start
 * and end.
 * 
 * \retval COBL_RET_ERROR
 * address is wrong
 * \retval COBL_RET_OK
 * address is correct
 *
 */

static CoblRet_t llcheckAddress(
		FlashAddr_t address /**< address to check */
	)
{
	if (address > FLASH_APPL_END(activeDomain))  {
		return(COBL_RET_ERROR);
	}

	if (address < FLASH_APPL_START(activeDomain))  {
		return(COBL_RET_ERROR);
	}

	return(COBL_RET_OK);
}
