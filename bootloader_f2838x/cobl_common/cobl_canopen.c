/* cobl_canopen
 *
 * Copyright (c) 2012-2019 emotas embedded communication GmbH
 *-------------------------------------------------------------------
 * SVN  $Id: cobl_canopen.c 30807 2020-02-24 11:27:34Z hil $
 *
 *
 *-------------------------------------------------------------------
 *
 *
 */

/********************************************************************/
/**
 * \file
 * \brief canopen routine
 */

/* standard includes
 --------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>

/* header of project specific types
 ---------------------------------------------------------------------------*/
#include <user_config.h>

#include <cobl_type.h>
#include <cobl_can.h>
#include <cobl_canopen.h>
#include <cobl_flash.h>
#include <cobl_timer.h>
#include <cobl_debug.h>

/* constant definitions
 ---------------------------------------------------------------------------*/

/* local defined data types
 ---------------------------------------------------------------------------*/
typedef enum coblSdoState{
	CO_SDO_FREE,
	CO_SDO_SEG_READ,
	CO_SDO_SEG_WRITE
}coblSdoState_t;

/* list of external used functions, if not in headers
 ---------------------------------------------------------------------------*/

/* list of global defined functions
 ---------------------------------------------------------------------------*/

/* list of local defined functions
 ---------------------------------------------------------------------------*/
static CoblRet_t canopenNMT(const CanMsg_t * const pMsg);
static CoblRet_t canopenNmtResetComm(void);

static CoblRet_t canopenHeartbeat(void);
static CoblRet_t canopenSendHeartbeat(U8 nmtState);


static CoblRet_t canopenSDO(const CanMsg_t * const pMsg);
static const CoOd_t * canopenGetOdEntry(const CanMsg_t * const pMsg);
static CoblRet_t canopenSendSdoReponse(const CanData_t * const pCanData);

static CoblRet_t canopenSendSdoAbort(const CanData_t * const pCanData,
		U32 u32AbortCode);
static CoblRet_t canopenSdoSegmented(const CanMsg_t * const pMsg);
#ifdef COBL_SDO_SEG_READ_TRANSFER_SUPPORTED
static CoblRet_t canopenSdoSegRead(const CanMsg_t * const pMsg);
#endif /* COBL_SDO_SEG_READ_TRANSFER_SUPPORTED */

static void canopenSdoReset(void);

#ifdef COBL_SDO_BLOCK_TRANSFER_SUPPORTED
static void canopenSdoBlockConfirmation(CanData_t *pCanData, U8 blkSeqNr);
#endif /* COBL_SDO_BLOCK_TRANSFER_SUPPORTED */

/* external variables
 ---------------------------------------------------------------------------*/

/* global variables
 ---------------------------------------------------------------------------*/

/* local defined variables
 ---------------------------------------------------------------------------*/
static U16 nodeId; /**< saved Node Id */

static coblSdoState_t eSdoState; /**< segmented Transfer is running */
static CanMsg_t firstSdoMsg; /**< saved first segmented SDO message */
static U32 u32DomainSize; /**< Domains size */
static U32 u32ReceivedSize; /**< currently received size */
static U8 u8ToggleBit; /**< toggle bit state */

static U8 domainBuffer[FLASH_WRITE_SIZE]; /* 8bit-Byte Buffer - unpacked */

#ifdef COBL_SDO_BLOCK_TRANSFER_SUPPORTED
static Flag_t	fSdoBlockTransfer;	/**< block transfer in progress */
static U8		u8BlkSeqNr;
static Flag_t	fSdoBlockEnd;
static U8		u8BlkBuffer[7]; /**< save buffer - 8bit chars - unpacked */
#endif /* COBL_SDO_BLOCK_TRANSFER_SUPPORTED */

#ifdef COBL_SDO_SEG_READ_TRANSFER_SUPPORTED
static U32		u32SegReadTransferSize;
static U8		u8SegReadToggle;
static U8 *		pu8SegReadObj;
#endif /* COBL_SDO_SEG_READ_TRANSFER_SUPPORTED */

/***************************************************************************/
/**
 * \brief canopen initialization
 *
 * \retval
 *       COBL_OK OK
 */

CoblRet_t canopenInit(
		void
	)
{
CoblRet_t ret;

	/* reset services */
	ret = canopenNmtResetComm();

	return(ret);
}

/***************************************************************************/
/**
 * \brief cyclic canopen call
 *
 * This function check for received messages and timer events.
 * it implements the canopen functionality.
 *
 * \retval
 *       COBL_RET_OK OK
 */

CoblRet_t canopenCyclic(
		void
	)
{
static CanMsg_t canMsg; /* static for better debugging */
CanState_t canState;


	/* check for received messages */
	canState = coblCanReceive(&canMsg);
	if (canState == CAN_OK)  {

		/* fix IDs */
		switch(canMsg.cobId.id)  {
		case 0x000:
			canopenNMT(&canMsg);
			break;
		default:
			break;
		}

		/* node id depend IDs */
		if ((canMsg.cobId.id & 0x7F) == nodeId)  {
			switch(canMsg.cobId.id & 0x780)  {
			case 0x600:
				canopenSDO(&canMsg);
				break;
			default:
				break;
			}
		}
	}

	/* check for timer events */
	canopenHeartbeat();

	return(COBL_RET_OK);
}


/***************************************************************************/
/**
 * \brief NMT related functionality
 *
 * In case of a reset this function do not return.

 * \retval  COBL_RET_OK 
 *		OK, also used for ignored messages
 */

static CoblRet_t canopenNMT(
		const CanMsg_t * const pMsg /**< received CAN message */
	)
{
U8 lNode;
U8 lCommand;

#ifdef COBL_DEBUG
	cobl_puts("NMT\n");
#endif

	/* check correct command length */
	if (pMsg->dlc != 2)  {
		return COBL_RET_OK; /* ignore */
	}

	/* check correct node */
	lNode = (pMsg->msg.u16Data[0] >> 8) & 0x00FFu;
	if ((lNode != 0) && (lNode != nodeId))  {
		return COBL_RET_OK; /* ignore */
	}

	/* command */
	lCommand = pMsg->msg.u16Data[0] & 0x00FFu;

	switch (lCommand)  {
	case NMT_RESET_NODE:
#ifdef COBL_DEBUG
		cobl_puts("Software Reset - Restart!\n");
#endif
		USER_RESET();
		/* break; */ /* !!! no break in case, that USER_RESET() is empty */
	case NMT_RESET_COMM:
		canopenNmtResetComm();
		break;
	default:
		break;
	}

	return(COBL_RET_OK);
}


/***************************************************************************/
/**
 * \brief NMT communication reset of the device
 *
 *
 * \retval
 *       COBL_OK OK
 *
 */

static CoblRet_t canopenNmtResetComm(
		void
	)
{
CoblRet_t ret;

	/* get node id */
	nodeId = GET_NODEID();

	/* reconfigure driver */
	coblCanConfigureFilter(nodeId);

	/* possible point for Reset OD - anything to do? */

	/* reset CANopen services */
	canopenSdoReset();

	/* send bootup */
	ret = canopenSendHeartbeat(0x00);
	if (ret != COBL_RET_OK)  {
		return(ret);
	}

	timerInit();

	/* possible point for user callback */

	return(COBL_RET_OK);
}


/***************************************************************************/
/**
 * \brief send Heartbeat message
 *
 *
* \retval COBL_RET_OK
 * 		HB was transmitted
 * \retval COBL_RET_CAN_BUSY
 *		HB was not transmitted
 *       
 */

static CoblRet_t canopenSendHeartbeat(
		U8 nmtState /**< NMT state */
	)
{
CanMsg_t canMsg;
CanState_t canState;

	canMsg.cobId.id = 0x700 + nodeId;
	canMsg.dlc = 1;
	canMsg.msg.u16Data[0] = (U16)nmtState;

	canState = coblCanTransmit(&canMsg);

	if (canState != CAN_OK)  {
		return(COBL_RET_CAN_BUSY);
	}

	return(COBL_RET_OK);
}


/***************************************************************************/
/**
 * \brief check and send Heartbeat message
 *
 *
 * \retval COBL_RET_OK
 *		no Error
 * \retval other
 * 		Error during HB generation
 */

static CoblRet_t canopenHeartbeat(
		void
	)
{
CoblRet_t ret;

	if (timerTimeExpired(COBL_HB_TIME) != 0)  {
		ret = canopenSendHeartbeat(0x7F);
		if (ret != COBL_RET_OK)  {
			return(ret);
		}
	}

	return(COBL_RET_OK);
}


/***************************************************************************/
/**
 * \brief central SDO related functionality
 *
 *
 * \retval COBL_RET_OK
 *       OK
 * \retval other
 * 		Error
 *
 */

static CoblRet_t canopenSDO(
		const CanMsg_t * const pMsg /**< received CAN message */
	)
{
const CoOd_t * pOD = NULL;
CoblRet_t ret;
Flag_t fSendAbort = 0;
Flag_t fSendResponse = 0;

#ifdef COBL_DEBUG
	printf("SDO\n");
#endif

	ret = COBL_RET_OK;

	/** Receive SDO Abort Message */
	if ((pMsg->msg.u16Data[0] & 0x00FFu) == 0x0080u)  {
		canopenSdoReset();
		return(ret);
	}

	/* exp. or segm. transfer */
	if (eSdoState == CO_SDO_SEG_WRITE)  {
		ret = canopenSdoSegmented(pMsg);
	}
#ifdef COBL_SDO_SEG_READ_TRANSFER_SUPPORTED
	else if (eSdoState == CO_SDO_SEG_READ)  {
		ret = canopenSdoSegRead(pMsg);
	}
#endif /* COBL_SDO_SEG_READ_TRANSFER_SUPPORTED */
	else {

		/* init */
		pOD = NULL;

		/* search entry only for the correct length */
		if (pMsg->dlc == 8u)  {
			/* check object dictionary */
			pOD = canopenGetOdEntry(pMsg);
		}

		/* what is to do */
		if (pOD == NULL)  {
			fSendAbort = 1;
		} else {
			/* callback? */
			if (pOD->bCallFunction != 0)  {
				ret = COBL_RET_OK;
				/* without function is the default used, too */
				if (pOD->ptrCallFunction != NULL)  {
					ret = pOD->ptrCallFunction(pMsg);
				}
				if (ret == COBL_RET_OK)  {
					fSendResponse = 1u;
				} else if (ret == COBL_RET_CALLBACK_READY)  {
					/* callback has answered */
				} else if (ret == COBL_RET_BUSY)  {
					/* do not abort internal states */
					ret = canopenSendSdoAbort(&pMsg->msg, SDO_ABORT_STATE);
				} else {
					fSendAbort = 1u;
				}
			} else {
				fSendResponse = 1u;
			}
		}
	}


	if (fSendResponse != 0u)  {
		ret = canopenSendSdoReponse(&pOD->resp);
	}

	if (fSendAbort != 0u)  {
		ret = canopenSendSdoAbort(&pMsg->msg, SDO_ABORT_GENERAL);
	}

	return(ret);
}

/***************************************************************************/
/**
 * \brief search object dictionary entry
 *
 *
 * \returns
 *       pointer to the OD entry or NULL
 */

static const CoOd_t * canopenGetOdEntry(
		const CanMsg_t * const pMsg /**< received CAN message */
	)
{
const CoOd_t * pOD = NULL; /* OD entry */
U16 i;
Flag_t fFound; /* OD entry found */

	fFound = 0;

	for (i = 0; i < odTableSize; i++)  {
		pOD = &coOd[i];
		if (pOD->req.u32Data[0] == (COBL_REVERSE_U32(pMsg->msg.u32Data[0]) & pOD->mask.u32Data[0]))  {
			if (pOD->bCompareSecond != 0)  {
				if (pOD->req.u32Data[1] == (COBL_REVERSE_U32(pMsg->msg.u32Data[1]) & pOD->mask.u32Data[1]))  {
					fFound = 1;
					break;
				}
			} else {
				fFound = 1u;
				break;
			}
		}
	}

	/* return the found entry */
	if (fFound != 0)  {
		return(pOD);
	}

	return(NULL);

}


/***************************************************************************/
/**
 * \brief reset all SDO states
 *
 * Additional to the SDO communication reset also the Flash driver
 * functionality is stopped.
 *
 * \returns
 *       nothing
 */

static void canopenSdoReset(
		void
	)
{
	/* reset segm. transfer flag */
	eSdoState = CO_SDO_FREE;
	/* stop erase/flash */
	if (flashAbort() != COBL_RET_CAN_BUSY)  {
		flashInit(0u);
	}
}


/***************************************************************************/
/**
 * \brief send fix response
 *
 *
 * \retval COBL_RET_OK
 * 		SDO response was transmitted
 * \retval COBL_RET_CAN_BUSY
 *		error during the transmission of the SDO Response message
 *     
 */

static CoblRet_t canopenSendSdoReponse(
		const CanData_t * const pCanData
	)
{
static CanMsg_t canMsg;
CanState_t canState;

#ifdef COBL_DEBUG
	printf("send Response %lx\n", (unsigned long)pCanData->u32Data[0]);
#endif

	canMsg.cobId.id = 0x580u + nodeId;
	canMsg.dlc = 8u;
	canMsg.msg.u32Data[0] = COBL_REVERSE_U32(pCanData->u32Data[0]);
	canMsg.msg.u32Data[1] = COBL_REVERSE_U32(pCanData->u32Data[1]);

	canState = coblCanTransmit(&canMsg);

	if (canState != CAN_OK)  {
		return(COBL_RET_CAN_BUSY);
	}

	return(COBL_RET_OK);
}

/***************************************************************************/
/**
 * \brief send SDO Abort Message
 *
 * This function is especially calling, if the SDO message in unknown.
 *
 *
 * \retval COBL_RET_OK
 * 		SDO response was transmitted
 * \retval other
 * 		Error
 *
 */

static CoblRet_t canopenSendSdoAbort(
		const CanData_t * const pCanData, /**< request message / little endian */
		U32 u32AbortCode /**< abort code / big endian */
	)
{
CanData_t lCanData;
CoblRet_t retval;

	lCanData = *pCanData;
	lCanData.u16Data[0] &= 0xFF00u;
	lCanData.u16Data[0] |= 0x0080u;
	/* switch to big endian for canopenSendSdoReponse() */
	lCanData.u32Data[0] = COBL_REVERSE_U32(lCanData.u32Data[0]);
	lCanData.u32Data[1] = u32AbortCode;

	/* SDO_ABORT_STATE means internal work active - do not abort */
	if (u32AbortCode != SDO_ABORT_STATE)  {
		canopenSdoReset();
	}

	retval = canopenSendSdoReponse(&lCanData);
	return(retval);
}

/***************************************************************************/
/**
 * \brief receive the first segment of the SDO transfer
 *
 * This function is especially calling from the object dictionary structure.
 *
 *
 * \returns
 *	CAN transmission state
 * \retval COBL_RET_OK
 * 		SDO response was transmitted
 * \retval other
 * 		Error in transmission
 */

CoblRet_t canopenSdoSegmentedFirst(
		const CanMsg_t * const pMsg /**< received CAN message / little endian */
	)
{
CoblRet_t ret;
U32 lDomainSize = 0;
U32 lAbortCode = 0;
CanData_t lCanData;
U16 subIndex;
U16 cmd;

#ifdef COBL_DEBUG
	cobl_puts("canopenSdoSegmentedFirst\n");
#endif

	if (eSdoState != CO_SDO_FREE)  {
		lAbortCode = SDO_ABORT_GENERAL;
	} else {
		subIndex = (pMsg->msg.u16Data[1] & 0xFF00) >> 8;
		/* stop/restart flash activity */
		flashAbort();
		flashInit((U8)(subIndex - 1u));

		firstSdoMsg = *pMsg;

		lDomainSize = COBL_REVERSE_U32(pMsg->msg.u32Data[1]);
		if (lDomainSize > CFG_MAX_DOMAINSIZE((U8)(subIndex - 1u)))  {
			lAbortCode = SDO_ABORT_LARGE;
		}
	}

	if (lAbortCode != 0)  {
		ret = canopenSendSdoAbort(&firstSdoMsg.msg, lAbortCode);
		return(ret);
	}

	/* init buffer */
	memset(&domainBuffer[0], FLASH_EMPTY, sizeof(domainBuffer));

	/* send ok */
	eSdoState = CO_SDO_SEG_WRITE;
	u32DomainSize = lDomainSize;
	u32ReceivedSize = 0;
	u8ToggleBit = 0x00;

	lCanData.u32Data[0] = pMsg->msg.u32Data[0];
	cmd = lCanData.u16Data[0] & 0x00FF;
	lCanData.u16Data[0] &= 0xFF00;

#ifdef COBL_SDO_BLOCK_TRANSFER_SUPPORTED
	if ((cmd & 0xe0) == 0xc0)  {
		fSdoBlockTransfer = 1;
		u8BlkSeqNr = 1;
		fSdoBlockEnd = 0;

		lCanData.u16Data[0] |= 0x00a0;
		lCanData.u32Data[1] = 0;
		lCanData.u16Data[2] = BL_BLOCK_SIZE;
	} else
#endif /* COBL_SDO_BLOCK_TRANSFER_SUPPORTED */
	{
#ifdef COBL_SDO_BLOCK_TRANSFER_SUPPORTED
		fSdoBlockTransfer = 0;
#endif /* COBL_SDO_BLOCK_TRANSFER_SUPPORTED */
		lCanData.u16Data[0] |= 0x60;
		lCanData.u32Data[1] = 0;
	}
	lCanData.u32Data[0] = COBL_REVERSE_U32(lCanData.u32Data[0]);
	lCanData.u32Data[1] = COBL_REVERSE_U32(lCanData.u32Data[1]);
	ret = canopenSendSdoReponse(&lCanData);

	return(ret);

}

/***************************************************************************/
/**
 * \brief received one segment - part of the domain transfer
 *
 * Check the correct command byte, save the received domain bytes and answer.
 *
 *
 * \returns
 *	CAN transmission state
 * \retval COBL_RET_OK
 * 		SDO response was transmitted
 * \retval other
 * 		Error in transmission
 */

static CoblRet_t canopenSdoSegmented(
		const CanMsg_t * const pMsg /**< received CAN message / little endian */
	)
{
CoblRet_t ret = COBL_RET_ERROR;
U8 lCommand = 0u;
U32 lAbortCode = 0u; /**< abort code */
Flag_t fLast = 0u; 	/**< last packet */
U8 lSize = 0u; 			/**< SDO command received size */
U32 lPageFullCnt;
U32 lPageFreeCnt;
U32 lCopyCnt; /**< count of copied data */
Flag_t fPageFull = 0; /**< domain buffer full, ready to flash */
Flag_t fBlocking = FLASH_NONBLOCKING;
CanData_t lCanData = {{ 0x00000000ul, 0x00000000ul}};
U8 lU8Data[8];

	lU8Data[0] = pMsg->msg.u16Data[0] & 0x00FFu;
	lU8Data[1] = (pMsg->msg.u16Data[0] >> 8) & 0x00FFu;
	lU8Data[2] = pMsg->msg.u16Data[1] & 0x00FFu;
	lU8Data[3] = (pMsg->msg.u16Data[1] >> 8) & 0x00FFu;
	lU8Data[4] = pMsg->msg.u16Data[2] & 0x00FFu;
	lU8Data[5] = (pMsg->msg.u16Data[2] >> 8) & 0x00FFu;
	lU8Data[6] = pMsg->msg.u16Data[3] & 0x00FFu;
	lU8Data[7] = (pMsg->msg.u16Data[3] >> 8) & 0x00FFu;

#ifdef CO_DRV_FILTER
	/* increase SDO segmented speed */
	fBlocking = FLASH_NONBLOCKING;
#else /*  CO_DRV_FILTER */
	fBlocking = FLASH_BLOCK_FUNCTION;
#endif /*  CO_DRV_FILTER */

	lCanData.u16Data[0] = 0x0020;

	if(pMsg->dlc != 8)  {
		lAbortCode = SDO_ABORT_GENERAL;
	} else {

		lCommand = pMsg->msg.u16Data[0] & 0x00FFu;
#ifdef COBL_SDO_BLOCK_TRANSFER_SUPPORTED
		if (fSdoBlockTransfer == 1u)  {
			ret = COBL_RET_OK;
			fBlocking = FLASH_BLOCK_FUNCTION;

			if (fSdoBlockEnd == 0)  {
				/* block dowload */
				if ((lCommand & 0x7f) == u8BlkSeqNr)  {
					u8BlkSeqNr++;

					/* last transfer ? */
					if ((lCommand & 0x80) != 0)  {
						/* save buffer, unused bytes are indicated by 
						 * end block transfer message */
						memcpy(&u8BlkBuffer[0], &lU8Data[1], 7);

						fSdoBlockEnd = 1u;

						/* send confirmation */
						canopenSdoBlockConfirmation(&lCanData, u8BlkSeqNr);

						lCanData.u32Data[0] = COBL_REVERSE_U32(lCanData.u32Data[0]);
						lCanData.u32Data[1] = COBL_REVERSE_U32(lCanData.u32Data[1]);
						ret = canopenSendSdoReponse(&lCanData);

						return(ret);
					}

					lSize = 7;

				} else {
					lSize = 0;
					SEND_EMCY(0xff20u, 0x01u, (U16)lCommand, (U16)u8BlkSeqNr);
				}
			} else {
				/* block end */
				lSize = 7 - ((lCommand & 0x1c) >> 2);
				fLast = 1u;
			}
		} else
#endif /* COBL_SDO_BLOCK_TRANSFER_SUPPORTED */
		{
			/* check received message */
			if ((lCommand & SDO_TOGGLE_BIT) != u8ToggleBit)  {
				lAbortCode = SDO_ABORT_TOGGLE;
			}

			if ((lCommand & SDO_LAST_BIT) != 0)  {
				fLast = 1u;
			}

			if ((lCommand & SDO_CCS_MASK) != SDO_CCS_DL_SEGMENT)  {
				lAbortCode = SDO_ABORT_GENERAL; /* wrong command */
			}
		}
	}

	if (lAbortCode == 0)  {
#ifdef COBL_SDO_BLOCK_TRANSFER_SUPPORTED
		if (fSdoBlockTransfer == 0)
#endif /* COBL_SDO_BLOCK_TRANSFER_SUPPORTED */
		{
			lSize = (lCommand & SDO_N_MASK) >> 1;
			lSize = 7u - lSize;
		}

		if ((u32ReceivedSize + lSize) > u32DomainSize)  {
			lAbortCode = SDO_ABORT_LARGE;
		}
	}

	/* return on error */
	if (lAbortCode != 0)  {
		ret = canopenSendSdoAbort(&firstSdoMsg.msg, lAbortCode);
		return(ret);
	}

	/* fill domain buffer */
	lPageFullCnt = u32ReceivedSize % FLASH_WRITE_SIZE;
	lPageFreeCnt = FLASH_WRITE_SIZE - lPageFullCnt;

	PRINTF2("full: %d free: %d\n", (int)lPageFullCnt, (int)lPageFreeCnt);

	/* how many data can be copied in the current buffer? */
	if (lPageFreeCnt <= lSize)  {
		lCopyCnt = lPageFreeCnt;
		fPageFull = 1u;
	} else {
		lCopyCnt = lSize;
	}

	PRINTF1("(1) copy %d byte\n", (int)lCopyCnt);

#ifdef COBL_SDO_BLOCK_TRANSFER_SUPPORTED
	if ((fSdoBlockTransfer != 0) && (fSdoBlockEnd != 0))
	{
		memcpy(&domainBuffer[lPageFullCnt], &u8BlkBuffer[0], lCopyCnt);
	} else
#endif /* COBL_SDO_BLOCK_TRANSFER_SUPPORTED */
	{
		memcpy(&domainBuffer[lPageFullCnt], &lU8Data[1], lCopyCnt);
	}
	lPageFullCnt += lCopyCnt;

	if (fPageFull != 0)  {
		PRINTF0("page is full\n");

		ret = flashPage(&domainBuffer[0], lPageFullCnt, fBlocking);
		if (ret != COBL_RET_OK)  {
			lAbortCode = SDO_ABORT_FLASH; /* error on flash */
		}
		lPageFullCnt = 0;
		/* reset domain Buffer */
		memset(&domainBuffer[0], FLASH_EMPTY, sizeof(domainBuffer));
	}

	/* copy the nonflashed bytes */
	if(lCopyCnt < lSize)  {
		PRINTF1("(2) copy %d byte\n", (int)(lSize - lCopyCnt));

#ifdef COBL_SDO_BLOCK_TRANSFER_SUPPORTED
		if (fSdoBlockEnd != 0)  {
			memcpy(&domainBuffer[0], &u8BlkBuffer[lCopyCnt + 1 - 1], lSize - lCopyCnt);
		} else
#endif /* COBL_SDO_BLOCK_TRANSFER_SUPPORTED */
		{
			memcpy(&domainBuffer[0], &lU8Data[lCopyCnt + 1], lSize - lCopyCnt);
		}

		lPageFullCnt = lSize - lCopyCnt;
	}

	/* flash the last page */
	if ((fLast != 0) && (lPageFullCnt > 0))  {
		ret = flashPage(&domainBuffer[0], lPageFullCnt /*FLASH_WRITE_SIZE */, fBlocking);
		if (ret != COBL_RET_OK)  {
			lAbortCode = SDO_ABORT_FLASH; /* error on flash */
		}
	}

	/* save receive counter */
	u32ReceivedSize += lSize;

	if (fLast != 0)  {
		/* wait for the end of flash cycle */
		ret = flashWaitEnd();
		if (ret != COBL_RET_OK)  {
			lAbortCode = SDO_ABORT_FLASH; /* error on flash */
		}

		/* reset segmented transfer */
		eSdoState = CO_SDO_FREE;
		/* check received data */
		if(u32ReceivedSize != u32DomainSize)  {
			lAbortCode = SDO_ABORT_GENERAL; /* wrong size */
		}
	}


	/* send response */
	if (lAbortCode == 0)  {
#ifdef COBL_SDO_BLOCK_TRANSFER_SUPPORTED
		if (fSdoBlockTransfer == 1)  {
			if (fSdoBlockEnd == 0)  {
				/* answer only after blocksize is reached */
				if ((lCommand & 0x7f) >= (BL_BLOCK_SIZE))  {
					canopenSdoBlockConfirmation(&lCanData, u8BlkSeqNr);

					lCanData.u32Data[0] = COBL_REVERSE_U32(lCanData.u32Data[0]);
					lCanData.u32Data[1] = COBL_REVERSE_U32(lCanData.u32Data[1]);
					ret = canopenSendSdoReponse(&lCanData);

					/* reset blk counter */
					if (u8BlkSeqNr >= BL_BLOCK_SIZE)  {
						u8BlkSeqNr = 1;
					}
				}
			} else {
				/* send block end */
				lCanData.u16Data[0] = (lCanData.u16Data[0] & 0xFF00) | 0xa1;

				lCanData.u32Data[0] = COBL_REVERSE_U32(lCanData.u32Data[0]);
				lCanData.u32Data[1] = COBL_REVERSE_U32(lCanData.u32Data[1]);
				ret = canopenSendSdoReponse(&lCanData);
			}
		} else
#endif /* COBL_SDO_BLOCK_TRANSFER_SUPPORTED */
		{
			/* correct answer toggle bit */
			lCanData.u16Data[0] |= u8ToggleBit;

			lCanData.u32Data[0] = COBL_REVERSE_U32(lCanData.u32Data[0]);
			lCanData.u32Data[1] = COBL_REVERSE_U32(lCanData.u32Data[1]);
			ret = canopenSendSdoReponse(&lCanData);
		}
	} else {
		ret = canopenSendSdoAbort(&firstSdoMsg.msg, lAbortCode);
	}

	/* toggle bit of the next transfer */
	u8ToggleBit ^= SDO_TOGGLE_BIT;

	return(ret);
}

/***************************************************************************/
#ifdef COBL_SDO_BLOCK_TRANSFER_SUPPORTED
static void canopenSdoBlockConfirmation(
		CanData_t *pCanData,
		U8 blkSeqNr /**< block sequence number */
	)
{
	pCanData->u16Data[0] = ((blkSeqNr - 1) << 8) | 0xa2;
	pCanData->u16Data[1] = (pCanData->u16Data[1] & 0xFF00) | BL_BLOCK_SIZE;
}
#endif /* COBL_SDO_BLOCK_TRANSFER_SUPPORTED */

#ifdef COBL_SDO_SEG_READ_TRANSFER_SUPPORTED
/***************************************************************************/
/**
* \brief receive the first segment of the SDO read transfer
*
* This function is especially calling from the object dictionary structure.
*
*
* \returns
*	CAN transmission state
* \retval COBL_RET_OK
* \retval other
* 		Not implemented yet
*/
CoblRet_t canopenSdoSegReadFirst(
		CoblObjInfo_t * const pObjInfo /**< received CAN message / little endian */
	)
{
CoblRet_t ret = COBL_RET_OK;

	u32SegReadTransferSize = pObjInfo->objSize;
	pu8SegReadObj = pObjInfo->pObjData;

	u8SegReadToggle = 0;

	eSdoState = CO_SDO_SEG_READ;

	return(ret);
}


/***************************************************************************/
static CoblRet_t canopenSdoSegRead(
		const CanMsg_t * const pMsg /**< received CAN message / little endian */
	)
{
U8			lCommand;
U32			lAbortCode = 0u; /**< abort code */
CoblRet_t	ret = COBL_RET_OK;
U8			lU8Data[8] = {0u,0u,0u,0u,0u,0u,0u,0u};
U8			lu8byteCnt;
CanData_t	lCanData = {{ 0x00000000ul, 0x00000000ul}};

	lCommand = pMsg->msg.u16Data[0] & 0x00FFu;

	/* test for right command spcifier */
	if ((lCommand & SDO_CCS_MASK) != SDO_CCS_UL_SEGMENT)  {
		lAbortCode = SDO_ABORT_GENERAL; /* wrong command */
	}
	/* test for right toggle bit value */
	if ((lCommand & SDO_TOGGLE_BIT) != u8SegReadToggle)  {
		lAbortCode = SDO_ABORT_TOGGLE;
	}

	if (lAbortCode != 0u)  {
		canopenSendSdoAbort(&pMsg->msg, lAbortCode);
		eSdoState = CO_SDO_FREE;
		return(COBL_RET_ERROR);
	}

	lU8Data[0] = u8SegReadToggle;

	if (u32SegReadTransferSize > 6)  {
		memcpy(&lU8Data[1], pu8SegReadObj, 7);
		u32SegReadTransferSize = u32SegReadTransferSize - 7u;
		pu8SegReadObj += 7;
		lu8byteCnt = 7u;
	} else {
		memcpy(&lU8Data[1], pu8SegReadObj, u32SegReadTransferSize);
		lu8byteCnt = (U8)u32SegReadTransferSize;
		u32SegReadTransferSize = 0u;
	}
	/* last ? */
	if (u32SegReadTransferSize == 0u)  {
		lU8Data[0] |= (UNSIGNED8)(0x01 | ((7u - lu8byteCnt) << 1));
		eSdoState = CO_SDO_FREE;
	}

	//memcpy(&lCanData, lU8Data, 8);
	lCanData.u16Data[0] = ((U16)lU8Data[1] << 8) | lU8Data[0];
	lCanData.u16Data[1] = ((U16)lU8Data[3] << 8) | lU8Data[2];
	lCanData.u16Data[2] = ((U16)lU8Data[5] << 8) | lU8Data[4];
	lCanData.u16Data[3] = ((U16)lU8Data[7] << 8) | lU8Data[6];

	canopenSendSdoReponse(&lCanData);

	/* toggle bit of the next transfer */
	u8SegReadToggle ^= SDO_TOGGLE_BIT;

	return(ret);
}
#endif /* COBL_SDO_SEG_READ_TRANSFER_SUPPORTED */

#ifdef COBL_EMCY_PRODUCER
/***************************************************************************/
/**
* \brief send an Emcy message
 *
 * \retval COBL_RET_OK
 * 		Emcy was transmitted
 * \retval COBL_RET_CAN_BUSY
 *		error during the transmission of the Emcy message
 */

CoblRet_t canopenSendEmcy(
		U16 errorcode,	/**< EMCY error code */
		U8 add0,		/**< Byte 0 - additional information */
		U16 add1,		/**< Byte 1,2 - additional information / Big Endian*/
		U16 add2		/**< Byte 3,4 - additional information */
	)
{
CanMsg_t canMsg;
CanState_t canState;

	canMsg.cobId.id = 0x80 + nodeId;
	canMsg.dlc = 8;
	canMsg.msg.u16Data[0] = COBL_REVERSE_U16(errorcode);

	canMsg.msg.u16Data[1] = 0x01u | (((U16)add0) << 8) ; /* generic */
	canMsg.msg.u16Data[2] = COBL_REVERSE_U16(add1);
	canMsg.msg.u16Data[3] = COBL_REVERSE_U16(add2);

	canState = coblCanTransmit(&canMsg);

	if (canState != CAN_OK)  {
		return(COBL_RET_CAN_BUSY);
	}

	return(COBL_RET_OK);
}
#endif
