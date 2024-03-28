/*
* cobl_flash.h
*
* Copyright (c) 2012-2019 emotas embedded communication GmbH
*-------------------------------------------------------------------
* SVN  $Id: cobl_flash.h 30008 2019-12-02 13:34:13Z ro $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \file
* \brief cobl_flash.h
*
*/

#ifndef COBL_FLASH_H
#define COBL_FLASH_H 1

/** \brief Flash driver states */
typedef enum {

	FLASH_STATE_INIT, /**< ready for commands */
	FLASH_STATE_WAIT, /**< wait for new flash pages */
	FLASH_STATE_ERASE, /**< erase a page */
	FLASH_STATE_FLASH, /**< flash a page */
	FLASH_STATE_ERROR, /**< error during flash/erase */
	FLASH_STATE_END		/**< last page flashed */
} FlashState_t;

#ifdef COBL_DEBUG
static const char* FlashState_txt[] = {
		"FLASH_STATE_INIT",
		"FLASH_STATE_WAIT",
		"FLASH_STATE_ERASE",
		"FLASH_STATE_FLASH",
		"FLASH_STATE_ERROR",
		"FLASH_STATE_END"
};
#endif

/** \brief reduced Flash driver states for user */
typedef enum {

	FLASH_USERSTATE_OK, /**< OK, nothing to do */
	FLASH_USERSTATE_RUNNING, /**< doing anything (erase, program) */
	FLASH_USERSTATE_ERROR /**< error during flash/erase */
} FlashUserState_t;


/* flash page definition */
typedef struct {
	FlashAddr_t baseAddress;
	U32 pageSize;
	U16 pageMask;
} pageStructure_t;


#define FLASH_NONBLOCKING 0u
#define FLASH_BLOCK_FUNCTION 1u

/* Variables
* --------------------------------------------------------------------*/
extern Flag_t fFullEraseRequired;
extern Flag_t fFlashErased[];
extern U8 activeDomain;

/* Prototypes
* --------------------------------------------------------------------*/
CoblRet_t flashInit(U8 nbr);
CoblRet_t flashErase(U8 nbr);
CoblRet_t flashPage(const void * const pBuffer, U32 size, Flag_t fBlocking);
CoblRet_t flashCyclic(void);
CoblRet_t flashAbort(void);
CoblRet_t flashWaitEnd(void);
void flashEraseRequired(void);

FlashUserState_t flashGetState(void);


#endif /* COBL_FLASH_H_ */
