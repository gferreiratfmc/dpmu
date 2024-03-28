/*
* cobl_flash.c
*
* Copyright (c) 2012-2019 emotas embedded communication GmbH
*-------------------------------------------------------------------
* SVN  $Id: cobl_flash.c 30042 2019-12-03 16:32:16Z ro $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \file
* \brief cobl_flash.c
* *
* Implementation of the basic functionality of erasing and programming
* the application FLASH area of the CANopen bootloader.
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
//#include <cobl_hardware.h>
/* for EMCY support */
#include <cobl_canopen.h>

#ifdef SIMULATION_FLASH
  #include <cobl_simulation.h>
#endif


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
#ifdef COBL_DEBUG
static void stateCheck(FlashState_t new);
#endif

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/
Flag_t fFullEraseRequired = 0; /**< erase without check is required (1 required) */
#ifdef COBL_DOMAIN_COUNT
Flag_t fFlashErased[COBL_DOMAIN_COUNT] = { 0 }; /**< flash was erased */
#else
Flag_t fFlashErased[1] = { 0 }; /**< flash was erased */
#endif
U8 activeDomain;

/* local defined variables
---------------------------------------------------------------------------*/
static Flag_t fEraseActive; /**< erase is active (0 inactive, 1 active) */
static FlashAddr_t nextEraseAddress; /**< next erase page address */

/*
 * fFlashBufferFull = 1 ... valid data in buffer
 * fFlashBufferFull = 0, but flashBufferValidSize > 0 .. old not flashed data in buffer!
 */
static Flag_t fFlashBufferFull; /**< flash buffer is full */
static FlashAddr_t nextFlashAddress; /**< next flash page address */
static U32 flashBufferValidSize; /**< number of valid bytes in Flashbuffer - DSP: word size  */
static U32 flashBufferWriteIdx; /* current position of flashing  - DSP: word size */

static Flag_t fFlashAbort; /**< abort signaled */
static Flag_t fFlashEnd; /**< wait for end of flash activity */

static FlashState_t flashState; /**< current state */

__inline static void setFlashState(FlashState_t newState);

/* buffer for flash data - also for data from the last call, that could not be flashed */
#ifdef CONFIG_DSP
static U16 flashBuffer[(FLASH_ONE_CALL_SIZE + FLASH_WRITE_SIZE)/2+1]; /**< packed data */
#else /*  CONFIG_DSP */
static U8 flashBuffer[FLASH_ONE_CALL_SIZE + FLASH_WRITE_SIZE]; /* word boundary */
#endif /*  CONFIG_DSP */

/***************************************************************************/
/**
 * \brief change flashstate
 *
 *
 * \returns
 *	OK
 */
__inline static void setFlashState(
		FlashState_t newState
	)
{
	flashState = newState;

#ifdef COBL_DEBUG
	stateCheck(newState);
#endif

}

#ifdef COBL_DEBUG
/***************************************************************************/
/**
 * \brief debug flashstate changes
 *
* print new flash state in case of a change
 *
 * \returns
 *	OK
 */
static void stateCheck(
		FlashState_t newState   /**< flash driver state */
	)
{
static FlashState_t old = (FlashState_t)0xff;

	if (old != newState)  {
		PRINTF2("State %d nextFlashAddress 0x%lx ",
				(int)flashState, nextFlashAddress);
		if (fEraseActive != 0)  {
			PRINTF2("fEraseActive %d nextEraseAddress 0x%lx\n ",
					(int)fEraseActive, nextEraseAddress);
		} else {
			PRINTF0("\n");
		}

		//		canopenSendEmcy(0xffff, (U16)new);
		old = newState;
	}
}
#endif


/***************************************************************************/
/**
 * \brief initialize flash functionality
 *
 *
 * \retval COBL_RET_OK
 *	OK
 */
CoblRet_t flashInit(
		U8 nbr
	)
{
	PRINTF0("flashInit\n");

	fEraseActive = 0u;
	fFlashBufferFull = 0u;
	flashBufferValidSize = 0u;
	flashBufferWriteIdx = 0u;
	fFlashAbort = 0u;
	fFlashEnd = 0u;
	activeDomain = nbr;

	(void)llflashInit(nbr);

	setFlashState(FLASH_STATE_INIT);

	return(COBL_RET_OK);
}


/***************************************************************************/
/**
 * \brief erase flash
 *
 * Start the erase and return immediately (default)
 * In case SDO_RESPONSE_AFTER_ERASE is set, the function returns at
 * the end of erase.
 *
 *
 * \retval COBL_RET_OK
 *	OK
 */
CoblRet_t flashErase(
		U8 nbr
	)
{
	PRINTF1("flashErase image %d\n", nbr);

	/* abort old flash activities */
	if (flashAbort() == COBL_RET_CAN_BUSY)  {
		return(COBL_RET_ERROR);
	}
	/* reinitialize flash */
	flashInit(nbr);

	PRINTF2("flashErase 0x%lx - 0x%lx\n", FLASH_APPL_START(nbr), FLASH_APPL_END(nbr));
	/* start erase */
	fEraseActive = 1;
#ifdef SDO_RESPONSE_AFTER_ERASE
	/* erase was started - wait for end */
	while(fEraseActive != 0)  {
	CoblRet_t ret;
		ret = flashCyclic();
		if ((ret != COBL_RET_OK) && (ret != COBL_RET_NOTHING))  {
			return(COBL_RET_ERROR);
		}
	}
#endif

	return(COBL_RET_OK);
}


/***************************************************************************/
/**
 * \brief flash a page
 *
 * The pBuffer data are copied to the flash function internal data.
 * The function is blocking during the last saved buffer is not ready.
 *
 * DSP:
 * pBuffer is a pointer to an 8bit-Byte Buffer
 * FLASH_PAGE_SIZE is the size in 8bit-Bytes.
 *
 * In case of a decryption of the data, this function is the right one.
 *
 * \retval COBL_RET_OK 
 * 		flash buffer was filled
 * \retval COBL_RET_ERROR
 * 		page can not be flashed
 *
 *
 */
CoblRet_t flashPage(
		const void * const pBuffer, /**< input data to flash - DSP:8bit char unpacked*/
		U32 size, /* number of valid data  - DSP: 8bit byte size */
		Flag_t fBlocking /* return after end of flashing */
	)
{
CoblRet_t ret;
#ifdef CONFIG_DSP
U16 i;
const U8 * const pCoBuffer = pBuffer; /**< local CANopen buffer pointer */
#endif /* CONFIG_DSP */

	PRINTF0("flashPage\n");

#ifdef COBL_DEBUG
	cobl_dump(pBuffer, FLASH_WRITE_SIZE);
#endif

#ifdef COBL_ERASE_BEFORE_FLASH
	if ((nextFlashAddress == FLASH_APPL_START(activeDomain))
	     && (fFlashErased[activeDomain] != 1u))
	{
		/* not erase command called or completed */
		return(COBL_RET_ERROR);
	}
#endif

	if ((flashState == FLASH_STATE_INIT)
			|| (flashState == FLASH_STATE_END)
			|| (flashState == FLASH_STATE_ERROR) )
	{
		/* wrong states - erase command req. */
		return(COBL_RET_ERROR);
	}

	/* buffer is full - wait for freeing */
	while(fFlashBufferFull != 0)  {
		ret = flashCyclic();
		if (ret != COBL_RET_OK)  {
			return(COBL_RET_ERROR);
		}
	}

	/* this page would be flash outside of it's range */
	if (nextFlashAddress >= FLASH_APPL_END(activeDomain))  {
		flashAbort(); /* Flash cycle abort */
		return(COBL_RET_ERROR);
	}

#ifdef CONFIG_DSP
	if ((size & 1) != 0) {
		/* german: gerade Anzahl - 2 8bit required for 1 word */
		return COBL_RET_ERROR;
	}
	/* ------------------------------------------------ */

	/* fill flash buffer - pack CANopen data - little endian - to the flash buffer
	 * Byte counts
	 */
	for(i = flashBufferValidSize * 2; i < (flashBufferValidSize * 2) + size; i += 2 ) {
		flashBuffer[i/2] = (pCoBuffer[i] & 0x00FFu) | (((U16)pCoBuffer[i+1] << 8) & 0xFF00u);
	}
	flashBufferValidSize += size/2; /* words */
#else /* CONFIG_DSP */
	/* ------------------------------------------------ */
	/* fill buffer */
	memcpy(&flashBuffer[flashBufferValidSize], pBuffer, size);
	flashBufferValidSize += size;
#endif /* CONFIG_DSP */

	fFlashBufferFull = 1;

	if (fBlocking == FLASH_BLOCK_FUNCTION)  {
		/* buffer is full - wait for freeing */
		while(fFlashBufferFull != 0)  {
			ret = flashCyclic();
			if (ret != COBL_RET_OK)  {
				return(COBL_RET_ERROR);
			}
		}

	}

	return(COBL_RET_OK);
}


/***************************************************************************/
/**
* \brief mark, that a complete erase is required
* Hint: never reset as long the bootloader runs to avoid cyclic problems
*       A reset of the Bootloader would reset this flag as long no problems found 
*       an this function is called again.
*/

void flashEraseRequired(
		void
	)
{
	fFullEraseRequired = 1u;
}

/***************************************************************************/
/**
 * \brief flash state machine
 *
 *
 * \retval COBL_RET_OK
 * state machine without errors
 *
 * \retval COBL_RET_ERROR
 * error in the current step
 *
 * \retval COBL_RET_FLASH_ERROR
 * stay in error state
 *
 * \retval COBL_RET_FLASH_END
 * end of flash activity
 *
 */

CoblRet_t flashCyclic(
		void
	)
{
CoblRet_t ret = COBL_RET_OK;
Flag_t fEraseRequired = 0;

	switch (flashState)  {
	case FLASH_STATE_INIT:
		/* init erase */
		nextEraseAddress = FLASH_APPL_START(activeDomain);
		/* init flash */
		nextFlashAddress = FLASH_APPL_START(activeDomain);
		/* init state */
		setFlashState(FLASH_STATE_WAIT);

		break;
	case FLASH_STATE_WAIT:
		/* stop activity */
		if (fFlashAbort == 1u)  {
			/* abort flashing */
			fFlashAbort = 0;
			setFlashState(FLASH_STATE_END);
			break;
		}

#ifdef COBL_ERASE_BEFORE_FLASH
#else /* COBL_ERASE_BEFORE_FLASH */
	#ifdef FLASH_OVERWRITE_ALLOWED
		if (fEraseActive != 0)
	#endif
		{
			if (nextFlashAddress == nextEraseAddress)  {
				fEraseRequired = 1u;
			}
		}
#endif /* COBL_ERASE_BEFORE_FLASH */

		if ((fFlashEnd == 1u) && (fFlashBufferFull == 0))  {
			/* flash last block */
			if (flashBufferValidSize > 0)  {
				fFlashBufferFull = 1u;
			}
		}

		/* flashing has higher priority - but only if flash is already erased */
		if ((fFlashBufferFull != 0) && (fEraseRequired == 0))  {
			/* flash page */
			fFlashErased[activeDomain] = 0u;
			ret = llflashOnePage(nextFlashAddress, &flashBuffer[flashBufferWriteIdx]);
			if (ret == COBL_RET_OK)  {
				setFlashState(FLASH_STATE_FLASH);
			} else {
				setFlashState(FLASH_STATE_ERROR);
			}
		} else {
			if ((fEraseActive != 0) || ((fFlashBufferFull != 0) && (fEraseRequired != 0)))  {
				/* erase next page,
				 * - erase command active
				 * - new buffer want to write without an extra erase command
				 */
				ret = lleraseOnePage(nextEraseAddress);
				if (ret == COBL_RET_OK)  {
					setFlashState(FLASH_STATE_ERASE);
				} else
				if (ret == COBL_RET_NOTHING)  {
					/* erase not required - calculate the next address 
					 * -> state changed to Erase to do this */
					setFlashState(FLASH_STATE_ERASE);
					ret = COBL_RET_OK;
				} else {
					/* different error cases */
					setFlashState(FLASH_STATE_ERROR);
				}
			} else {
				/* no flash active, no erase active */
				if (fFlashEnd != 0)  {
					fFlashEnd = 0;
					setFlashState(FLASH_STATE_END);
				}
			}
		}

		break;
	case FLASH_STATE_ERASE:
		/* TODO: check flash register for active erasing 
		* - during erase stay in state erase
		*   -> additional check for errors!
		* - but only at the end calculate the next address
		*   and go to the Wait state
		*
		* For this environment not required, because the bootloader
		* and the application run on the same memory controller.
		* The CPU stop work during erase/flash.
		*/
		PRINTF1("erase ok: 0x%lx\n", nextEraseAddress);

		/* at the moment nothing to do */
#ifdef FLASH_ERASE_SIZE
		nextEraseAddress += FLASH_ERASE_SIZE;
		/* in case, that the first address is not at the start of a section */
		nextEraseAddress -= (nextEraseAddress % FLASH_ERASE_SIZE);

#else /* FLASH_ERASE_SIZE */
		{
		U32 i;
			i = llsectorSize(nextEraseAddress);
			nextEraseAddress += i;
		}
#endif /* FLASH_ERASE_SIZE */

		PRINTF2("next erase: 0x%lx/0x%lx\n", nextEraseAddress, FLASH_APPL_END(activeDomain));
		/* check for the end of erase */
		if (nextEraseAddress >= FLASH_APPL_END(activeDomain))  {
			PRINTF0("erase stopped\n");
			/* no more erase */
			fEraseActive = 0;
			fFlashErased[activeDomain] = 1u;
			SEND_EMCY(0xff00u, 0x1Fu, 0u, 0u);
#ifdef COBL_ECC_FLASH
			/* clear old ECC error */
			cobl_command[ECC_ERROR_IDX] = 0;
#endif
		}

		setFlashState(FLASH_STATE_WAIT);
		break;
	case FLASH_STATE_FLASH:
		/* check flash register for active flashing
		 * if required
		 * (only req. if Bootloader in RAM)
		 */
		PRINTF1("flash ok: 0x%lx\n", nextFlashAddress);

		/* flash only some bytes of the flash at the same time */
#ifdef CONFIG_DSP
		nextFlashAddress += FLASH_ONE_CALL_SIZE/2;
		flashBufferWriteIdx += FLASH_ONE_CALL_SIZE/2;
#else /* CONFIG_DSP */
		nextFlashAddress += FLASH_ONE_CALL_SIZE;
		flashBufferWriteIdx += FLASH_ONE_CALL_SIZE;
#endif /* CONFIG_DSP */

		/* last block flashed (with additional not used bytes
		 * or complete block flashed
		 */
		if (flashBufferValidSize <= flashBufferWriteIdx)  {
			flashBufferValidSize = 0;
			flashBufferWriteIdx = 0;
			fFlashBufferFull = 0; /* reset buffer */
		} else {
#ifdef CONFIG_DSP
			if ((flashBufferValidSize - flashBufferWriteIdx) < FLASH_ONE_CALL_SIZE/2)  
#else /* CONFIG_DSP */
			if ((flashBufferValidSize - flashBufferWriteIdx) < FLASH_ONE_CALL_SIZE)
#endif /* CONFIG_DSP */
            {
				memmove(&flashBuffer[0], &flashBuffer[flashBufferWriteIdx], (flashBufferValidSize - flashBufferWriteIdx));
				flashBufferValidSize -= flashBufferWriteIdx;
				flashBufferWriteIdx = 0;
				fFlashBufferFull = 0; /* reset buffer */
			}
		}

		if (nextFlashAddress >= FLASH_APPL_END(activeDomain))  {
			setFlashState(FLASH_STATE_END); /* last flash page */
		} else {
			setFlashState(FLASH_STATE_WAIT);
		}

		break;
	case FLASH_STATE_ERROR:
		ret = COBL_RET_FLASH_ERROR;
		break;
	case FLASH_STATE_END:
		ret = COBL_RET_FLASH_END;
		break;
	default:
		break;
	}

	return(ret);
}

/***************************************************************************/
/**
 * \brief abort flashing
 *
 * Stop erase and flash state machine.
 *
 * \retval COBL_RET_OK
 *	OK
 */

CoblRet_t flashAbort(
		void
	)
{
	PRINTF0("flashAbort\n");

	if ((flashState == FLASH_STATE_INIT)
			|| (flashState == FLASH_STATE_END)
			|| (flashState == FLASH_STATE_ERROR) )
	{
		/* nothing to do */
	} else {
#ifdef COBL_DONT_ABORT_FLASH
		if (fEraseActive != 0)  {
			return(COBL_RET_CAN_BUSY);
		}
#else
		/* command - stop flash activity */
		fFlashAbort = 1;
		flashWaitEnd();
#endif
	}

	return(COBL_RET_OK);
}

/***************************************************************************/
/**
 * \brief wait for the end of flashing
 *
 * wait for the end of the flash state machine
 *
 * \retval COBL_RET_OK
 *	OK
 * \retval other
 *  Error signaled by the flash state machine
 */

CoblRet_t flashWaitEnd(
		void
	)
{
CoblRet_t ret;

	/* mark end of flash cycle */
	fFlashEnd = 1;

	SEND_EMCY(0xff00u, 0u, 0u, fFlashAbort);
	do {
		ret = flashCyclic();
	} while ((ret != COBL_RET_FLASH_ERROR) && (ret != COBL_RET_FLASH_END));

	SEND_EMCY(0xff00u, 0x88u, 0u, 0u);

	PRINTF0("Flash flashed completely\n");

	if (ret == COBL_RET_FLASH_END)  {
		return(COBL_RET_OK);
	}

	return(ret);
}


/***************************************************************************/
/**
 * \brief get current flash driver state
 *
 * \retval FLASH_USERSTATE_RUNNING
 *		Flash process running
 * \retval FLASH_USERSTATE_OK
 *		nothing to do, ready for new commands
 * \retval FLASH_USERSTATE_ERROR
 * 		Error state
*/
FlashUserState_t flashGetState(
		void
	)
{
	switch (flashState)  {
	case FLASH_STATE_INIT:
	case FLASH_STATE_WAIT:
	case FLASH_STATE_END:
		if ((fEraseActive != 0) || (fFlashBufferFull != 0))  {
			return(FLASH_USERSTATE_RUNNING);
		}
		return(FLASH_USERSTATE_OK);
	case FLASH_STATE_ERASE:
	case FLASH_STATE_FLASH:
		return(FLASH_USERSTATE_RUNNING);
	case FLASH_STATE_ERROR:
		return(FLASH_USERSTATE_ERROR);
	default:
		return(FLASH_USERSTATE_RUNNING);
	}

}


