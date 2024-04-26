/*
*
* Copyright (c) 2012-2019 emotas embedded communication GmbH
*-------------------------------------------------------------------
* SVN  $Id: cobl_od.c 31966 2020-05-06 11:09:20Z ro $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \file
* \brief object dictionary of the bootloader and OD callbacks
*
*
* callback return values
*  COBL_STATE_OK - OK, callback has answered
*  COBL_CALLBACK_OK - OK, sdo function should send OK answer
*  COBL_CALLBACK_ABORT - sdo should send general abort
*  other - error. callback has answered
*
*
*
*/

/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <stdio.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <user_config.h>

#include <cobl_type.h>
#include <cobl_debug.h>
#include <cobl_can.h>
#include <cobl_canopen.h>
#include <cobl_application.h>
#include <cobl_flash.h>
#include <cobl_call.h>
#include <cobl_user.h>

/* Definitions
---------------------------------------------------------------------------*/
#define RES {0,0}
#define OD_UNUSED {{0,0}}
#define MASK_ALLSET {{0xFFFFFFFFul,0xFFFFFFFFul}}
#define BOOT 0x424F4F54ul

#ifndef COBL_DOMAIN_COUNT
# define COBL_DOMAIN_COUNT 2u
#endif /* COBL_DOMAIN_COUNT */

/***************************************************************************/
/**
 * \brief send fix response
 *
 * \retval COBL_RET_OK
 * 		Response transmission OK
 *\retval COBL_RET_CAN_BUSY
 *		Transmission error
 */

static CoblRet_t odSdoSendReponse(
		const CanData_t * const pCanData /**< CAN data to transmit /little endian */
	)
{
static CanMsg_t canMsg;
CanState_t canState;

	canMsg.cobId.id = 0x580 + GET_NODEID();
	canMsg.dlc = 8u;
	canMsg.msg = *pCanData;

	canState = coblCanTransmit(&canMsg);

	if (canState != CAN_OK)  {
		return(COBL_RET_CAN_BUSY);
	}

	return(COBL_RET_OK);
}


/***************************************************************************/
/**
* \brief callback to read object by SDO segmented Upload
*
* \retval COBL_RET_CALLBACK_READY
* 		to avoid the generic canopen implentation to send a response
*/

#ifdef COBL_SDO_SEG_READ_TRANSFER_SUPPORTED
static CoblRet_t odSdoSegRead(
		const CanMsg_t * const pMsg
	)
{
UNSIGNED16		index;
UNSIGNED8		sunIndex;
CoblObjInfo_t	sObjInfo;
CanData_t		lCanData = {{ 0x00000000ul, 0x00000000ul}};

	index = (pMsg->msg.u16Data[1] & 0x00ff);
	index <<= 8;
	index |= ((pMsg->msg.u16Data[0] & 0xff00) >> 8u);

	sunIndex = ((pMsg->msg.u16Data[1] & 0xff00) >> 8u);

	sObjInfo = COBL_OD_SEG_READ_FCT(index, sunIndex);
	if ((sObjInfo.pObjData == NULL) || (sObjInfo.objSize == 0u))  {
		return(COBL_RET_ERROR);/// TODO
	}

	canopenSdoSegReadFirst(&sObjInfo);

	/* copy index and subIndex information */
	lCanData.u32Data[0] = pMsg->msg.u32Data[0];

	/* create response frame */
	lCanData.u16Data[0] &= 0xff00;
	lCanData.u16Data[0] |= 0x0041;
	lCanData.u32Data[1] = COBL_REVERSE_U32(sObjInfo.objSize);

	odSdoSendReponse(&lCanData);

	return(COBL_RET_CALLBACK_READY);
}
#endif /* COBL_SDO_SEG_READ_TRANSFER_SUPPORTED */

/***************************************************************************/
/**
 * answer for 1014:0 request (Emcy COB-ID)
 */
static CoblRet_t read1014(
		const CanMsg_t * const pMsg
	)
{
CanData_t data;
U32 cobid;

	cobid = 0x80 + GET_NODEID();
#ifdef COBL_BIG_ENDIAN
#error "adapt it to big endian"
#endif

	data.u32Data[0] = (pMsg->msg.u32Data[0] & 0xFFFFFF00) | 0x43;
	data.u32Data[1] = COBL_REVERSE_U32(cobid);

	odSdoSendReponse(&data);

	return(COBL_RET_CALLBACK_READY);
}

/***************************************************************************/
/**
* \brief answer for 1F50:x request (read domain)
*
* \retval COBL_RET_CALLBACK_READY
*		callback function created the answer itself
*/
static CoblRet_t read1F50(
		const CanMsg_t * const pMsg /**< SDO request */
	)
{
CanData_t data;
U16 subIndex;

	data.u32Data[0] = pMsg->msg.u32Data[0];
	subIndex = (data.u16Data[1] >> 8);
	/* abort */
	data.u16Data[0] = (data.u16Data[0] & 0xFF00) | 0x80;

	if ((subIndex <= COBL_DOMAIN_COUNT) &&
		(subIndex > 0u))  {
		data.u32Data[1] = COBL_REVERSE_U32(0x06010001); /* no read perm */
	} else {
		data.u32Data[1] = COBL_REVERSE_U32(0x06090011);
	}

	odSdoSendReponse(&data);

	return(COBL_RET_CALLBACK_READY);
}

/***************************************************************************/
/**
 * \brief answer for 1F56:x request (CRC sum)
 *
 * \retval COBL_RET_CALLBACK_READY
 *		callback function created the answer itself
 */
static CoblRet_t read1F56(
		const CanMsg_t * const pMsg /**< SDO request /little endian */
	)
{
CanData_t data;
U16 subIndex;
U16 chksum;
FlashUserState_t lFlashState;

	data.u32Data[0] = pMsg->msg.u32Data[0];
	subIndex = (data.u16Data[1] >> 8);

	if ((subIndex <= COBL_DOMAIN_COUNT) &&
		(subIndex > 0u))  {
		data.u16Data[0] = (data.u16Data[0] & 0xFF00) | 0x43; /* OK */

		lFlashState = flashGetState();
		if (lFlashState == FLASH_USERSTATE_RUNNING)  {
			/* e.g. erase running */
			return(COBL_RET_BUSY);
		} else
		if (applCheckChecksum((U8)(subIndex - 1u)) != COBL_RET_OK)  {
			/* wrong checksum */
			data.u16Data[2] = 0;
		} else {
			chksum = applChecksum((U8)(subIndex - 1u));
			data.u16Data[2] = COBL_REVERSE_U16(chksum);
		}
		data.u16Data[3] = 0;
	} else {
		data.u16Data[0] = (data.u16Data[0] & 0xFF00) | 0x80; /* abort */
		data.u32Data[1] = COBL_REVERSE_U32(0x06090011);
	}

	odSdoSendReponse(&data);

	return(COBL_RET_CALLBACK_READY);
}

/***************************************************************************/
/**
 * \brief answer for 1F57:x request (flash state)
 *
 * \retval COBL_RET_CALLBACK_READY
 *		callback function created the answer itself
 */
static CoblRet_t read1F57(
		const CanMsg_t * const pMsg /**< SDO request /little endian */
	)
{
CanData_t data;
U32 u32State;
FlashUserState_t lFlashState;
U16 subIndex;

	data.u32Data[0] = pMsg->msg.u32Data[0];
	subIndex = (data.u16Data[1] >> 8);

	if ((subIndex <= COBL_DOMAIN_COUNT) &&
		(subIndex > 0u))  {

		u32State = 0u;
		lFlashState = flashGetState();

#ifdef COBL_ECC_FLASH
		if (cobl_command[ECC_ERROR_IDX] == ECC_ERROR_VAL)  {
			u32State |= 65 << 1; /* manuf.specified - ECC error */
		} else
#endif
		{
			switch (lFlashState)  {
			case FLASH_USERSTATE_OK:
				if (applCheckChecksum((U8)(subIndex - 1u)) != COBL_RET_OK)  {
					u32State |= 1 << 1; /* no valid program */
				}
				break;
			case FLASH_USERSTATE_RUNNING:
				u32State |= 1 << 1; /* no valid program */
				break;
			case FLASH_USERSTATE_ERROR:
				u32State |= 63 << 1; /* unspecified */
				break;
			default:
				u32State |= 64 << 1; /* manuf.specified */
				break;
			}
		}

		if (lFlashState == FLASH_USERSTATE_RUNNING)  {
			u32State |= 0x01;
		}

		data.u16Data[0] = (data.u16Data[0] & 0xFF00) | 0x43; /* OK */
		data.u32Data[1] = COBL_REVERSE_U32(u32State);

	} else {
		data.u16Data[0] = (data.u16Data[0] & 0xFF00) | 0x80; /* abort */
		data.u32Data[1] = COBL_REVERSE_U32(0x06090011);
	}

	odSdoSendReponse(&data);

	return(COBL_RET_CALLBACK_READY);
}


/***************************************************************************/
static CoblRet_t readU32(
		const CanMsg_t * const pMsg
	)
{
CanData_t data;
U32 u32Value;
U32 u32MsgData;

/* Note: case works not with U32 on s12z */
	u32MsgData = COBL_REVERSE_U32(pMsg->msg.u32Data[0]);

#ifdef COBL_OD_1000_0_FCT
	if (u32MsgData == 0x00100040ul)  {
		u32Value = COBL_OD_1000_0_FCT();
	} else
#endif

#ifdef COBL_OD_1018_1_FCT
	if (u32MsgData == 0x01101840ul)  {
		u32Value = COBL_OD_1018_1_FCT();
	} else
#endif
#ifdef COBL_OD_1018_2_FCT
	if (u32MsgData == 0x02101840ul)  {
		u32Value = COBL_OD_1018_2_FCT();
	} else
#endif
#ifdef COBL_OD_1018_3_FCT
	if (u32MsgData == 0x03101840ul)  {
		u32Value = COBL_OD_1018_3_FCT();
	} else
#endif
#ifdef COBL_OD_1018_4_FCT
	if (u32MsgData == 0x04101840ul)  {
		u32Value = COBL_OD_1018_4_FCT();
	} else
#endif

#ifdef COBL_OD_5F00_0_FCT
	if (u32MsgData == 0x005F0040ul)  {
		u32Value = COBL_OD_5F00_0_FCT();
	} else
#endif

#ifdef COBL_OD_U32_READ_FCT
	{
	U16 index;
	U8 subIndex;
	
		index = (pMsg->msg.u16Data[1] & 0x00ff);
		index <<= 8;
		index |= ((pMsg->msg.u16Data[0] & 0xff00) >> 8u);

		subIndex = ((pMsg->msg.u16Data[1] & 0xff00) >> 8u);

		u32Value = COBL_OD_U32_READ_FCT(index, subIndex);
	}
#else /* COBL_OD_U32_READ_FCT */
	{
		u32Value = 0xFFFFFFFFul;
	}
#endif /* COBL_OD_U32_READ_FCT */

	data.u32Data[0] = pMsg->msg.u32Data[0];
	data.u16Data[0] = (data.u16Data[0] & 0xFF00) |  0x43; /* OK */
	data.u32Data[1] = COBL_REVERSE_U32(u32Value);

	odSdoSendReponse(&data);

	return(COBL_RET_CALLBACK_READY);
}

/***************************************************************************/

/***************************************************************************/
/**
* \brief Domain initial request
 *  -> write data to flash
 *
 * \retval COBL_RET_CALLBACK_READY
 *		callback function created the answer itself
 */
static CoblRet_t write1F50(
		const CanMsg_t * const pMsg /**< SDO initial request */
	)
{
CanData_t data;
#ifdef COBL_ECC_FLASH
FlashUserState_t lFlashState;
#endif /* COBL_ECC_FLASH */
U16 subIndex;

	subIndex = (pMsg->msg.u16Data[1] >> 8);

	if ((subIndex <= COBL_DOMAIN_COUNT) &&
		(subIndex > 0u))  {
		/* idea: Erase only after command on 1F51, at this point check for erased flash */
		//	flashErase();

#ifdef COBL_ECC_FLASH
		/* do not abort an erase cycle */
		lFlashState = flashGetState();
		if (lFlashState == FLASH_USERSTATE_RUNNING)  {
			/* e.g. erase running */
			return(COBL_RET_BUSY);
		}
#endif /* COBL_ECC_FLASH */

		canopenSdoSegmentedFirst(pMsg);
	} else {
		data.u16Data[0] = (pMsg->msg.u16Data[0] & 0xFF00) | 0x80; /* abort */
		data.u16Data[1] = pMsg->msg.u16Data[1];
		data.u32Data[1] = COBL_REVERSE_U32(0x06090011);
		odSdoSendReponse(&data);
	}

	return(COBL_RET_CALLBACK_READY);
}

/***************************************************************************/
/**
* \brief answer for 1F51:x request (program)
*
* \retval COBL_RET_CALLBACK_READY
*		callback function created the answer itself
*/
static CoblRet_t read1F51(
		const CanMsg_t * const pMsg /**< SDO request */
	)
{
CanData_t data;
U16 subIndex;

	data.u32Data[0] = pMsg->msg.u32Data[0];
	subIndex = (data.u16Data[1] >> 8);

	if ((subIndex <= COBL_DOMAIN_COUNT) &&
		(subIndex > 0u))  {
		data.u16Data[0] = (data.u16Data[0] & 0xFF00) | 0x4f;
		data.u32Data[1] = 0u;
	} else {
		data.u16Data[0] = (data.u16Data[0] & 0xFF00) | 0x80; /* abort */
		data.u32Data[1] = COBL_REVERSE_U32(0x06090011);
	}

	odSdoSendReponse(&data);

	return(COBL_RET_CALLBACK_READY);
}

/***************************************************************************/
/**
 * \brief erase command
 * 		start erase command
 *
 * \returns flash driver answer
 * \retval COBL_RET_OK
 *		OK
 * \retval other
 *		 erase error
 */
static CoblRet_t write1F51Erase(
		const CanMsg_t * const pMsg /**< SDO initial request */
	)
{
CoblRet_t retVal;
CanData_t data;
U16 subIndex;

	subIndex = (pMsg->msg.u16Data[1] >> 8);

	if ((subIndex > COBL_DOMAIN_COUNT) &&
		(subIndex < 1u))
	{
		return(COBL_RET_ERROR);
	}

	retVal = flashErase((U8)(subIndex - 1u));
	if (retVal == COBL_RET_OK)  {
		data.u16Data[0] = (pMsg->msg.u16Data[0] & 0xFF00) | 0x60; /* OK */
		data.u16Data[1] = pMsg->msg.u16Data[1];
		data.u32Data[1] = 0u;

		odSdoSendReponse(&data);

		return(COBL_RET_CALLBACK_READY);
	}
	return(retVal);
}

#ifdef COBL_SDO_PROG_START
/***************************************************************************/
/**
* \brief start program
* 		call the start program by SDO 
*		this will call the Reset
*       Everytime the Sub1 (application) must be used.
*       
* \returns never
*/
static CoblRet_t write1F51Start(
		const CanMsg_t * const pMsg /**< SDO initial request */
	)
{
CanData_t data;

	data.u32Data[0] = pMsg->msg.u32Data[0];
	data.u16Data[0] = (pMsg->msg.u16Data[0] & 0xFF00) | 0x60; /* OK */
	data.u32Data[1] = 0u;

	odSdoSendReponse(&data);

#ifdef COBL_DEBUG
	cobl_puts("Software Reset - Restart!\n");
#endif
	USER_RESET();

	return(COBL_RET_CALLBACK_READY);
}
#endif /* COBL_SDO_PROG_START */

/***************************************************************************/
/** object dictionary
 *
 */
const CoOd_t coOd[] = {
	/* comp 2., call, res, {{req}} */
	{ 0u, 1u, RES, 	{{ 0x00100040ul, 0x00000000ul}},
			/* {{mask}} */
			MASK_ALLSET,
			/* {{resp}} */
			{{0x00100043ul, BOOT}},
			/* call */
#ifdef COBL_OD_1000_0_FCT
			readU32
#else
			NULL
#endif
	},
	{ 0u, 0u, RES, 	{{ 0x00100140ul, 0x00000000ul}},
			MASK_ALLSET,
			{{0x0010014Ful, 0x00000000ul}},
			NULL
	},
#ifdef COBL_SDO_SEG_READ_TRANSFER_SUPPORTED
	{ 0u, 1u, RES,{ { 0x00100840ul, 0x00000000ul } },
			MASK_ALLSET,
			{{0ul, 0ul}}, /* 0x1008 - man. device name */
			odSdoSegRead
	},
#endif /* COBL_SDO_SEG_READ_TRANSFER_SUPPORTED */
#ifdef COBL_EMCY_PRODUCER
	{ 0u, 1u, RES, 	{{ 0x00101440ul, 0x00000000ul}},
			MASK_ALLSET,
			{{ 0x00101443ul, 0x81ul}}, /* Emcy COB-ID */
			read1014 //NULL
	},
#endif
	{ 0u, 0u, RES, 	{{ 0x00101740ul, 0x00000000ul}},
			MASK_ALLSET,
			{{ 0x0010174Bul, COBL_HB_TIME /*2000ul*/}}, /* HB Prod. time */
			NULL
	},
	{ 0u, 0u, RES, 	{{ 0x0010172Bul, 0x00000000ul}},
			MASK_ALLSET,
			{{ 0x00101780ul, 0x06010002ul}}, /* no write perm -HB Prod. time */
			NULL
	},
	{ 0u, 0u, RES, 	{{ 0x00101840ul, 0x00000000ul}},
			MASK_ALLSET,
			{{ 0x0010184Ful, 4ul}}, /* Identity */
			NULL
	},
	{ 0u, 1u, RES, 	{{ 0x01101840ul, 0x00000000ul}},
			MASK_ALLSET,
			{{ 0x01101843ul, 0ul}}, /* Identity - Vendor id*/
#ifdef COBL_OD_1018_1_FCT
			readU32
#else
			NULL
#endif
	},
	{ 0u, 1u, RES, 	{{ 0x02101840ul, 0x00000000ul}},
			MASK_ALLSET,
			{{ 0x02101843ul, 0ul}}, /* Identity - product code */
#ifdef COBL_OD_1018_2_FCT
			readU32
#else
			NULL
#endif
	},
	{ 0u, 1u, RES, 	{{ 0x03101840ul, 0x00000000ul}},
			MASK_ALLSET,
			{{ 0x03101843ul, 0ul}}, /* Identity - revision */
#ifdef COBL_OD_1018_3_FCT
			readU32
#else
			NULL
#endif
	},
	{ 0u, 1u, RES, 	{{ 0x04101840ul, 0x00000000ul}},
			MASK_ALLSET,
			{{ 0x04101843ul, 0ul}}, /* Identity - serial */
#ifdef COBL_OD_1018_4_FCT
			readU32
#else
			NULL
#endif
	},
#ifdef COBL_OD_U32_READ_FCT
	{ 0u, 0u, RES, 	{{ 0x00120040ul, 0x00000000ul}},
			MASK_ALLSET,
			{{ 0x0012004Ful, 2ul}}, /* 0x1200:0=2 SDO Server */
			NULL
	},
	{ 0u, 1u, RES, 	{{ 0x01120040ul, 0x00000000ul}},
			MASK_ALLSET,
			{{ 0x01120043ul, 0ul}}, /* 0x1200:1= 0x600+Nodeid */
			readU32
	},
	{ 0u, 1u, RES, 	{{ 0x02120040ul, 0x00000000ul}},
			MASK_ALLSET,
			{{ 0x02120043ul, 0ul}}, /* 0x1200:2=0x580+Nodeid */
			readU32
	},
#endif /* COBL_OD_U32_READ_FCT */
	{ 0u, 0u, RES, 	{{ 0x001F5040ul, 0x00000000ul}},
			MASK_ALLSET,
			{ { 0x001F504Ful, COBL_DOMAIN_COUNT } }, /* number of domains */
			NULL
	},
	{ 0u, 1u, RES, 	{{ 0x001F5040ul, 0x00000000ul}},
			{ { 0x00FFFFFFul, 0ul } },
			{ { 0x011F5080ul, 0x06010001ul } }, /* no read perm - domains */
			read1F50
	},
	/* for 0x1f50 subindex 0 */
	{ 0u, 0u, RES, { { 0x001F5021ul, 0x00000000ul } },
			MASK_ALLSET,
			{ { 0x001F5080ul, 0x06010002ul } }, /* write: program erase */
			NULL
	},
	/* comp 2., call, res, {{req}} */
	{ 0u, 1u, RES, 	{{ 0x001F5021ul, 0x00000000ul}}, /* Domain write */
			/* {{mask}} */
			{ { 0x00FFFFFFul, 0xffFFffFFul } },
			/* {{resp}} */
			OD_UNUSED,
			/* call */
			write1F50
	},
#ifdef COBL_SDO_BLOCK_TRANSFER_SUPPORTED
	{ 0u, 1u, RES, 	{{ 0x001F50c2ul, 0x00000000ul}}, /* Domain write block */
			/* {{mask}} */
			{ { 0x00FFFFFFul, 0xffFFffFFul } },
			/* {{resp}} */
			OD_UNUSED,
			/* call */
			write1F50
	},
#endif /* COBL_SDO_BLOCK_TRANSFER_SUPPORTED */
	{ 0u, 0u, RES, 	{{ 0x001F5140ul, 0x00000000ul}},
			MASK_ALLSET,
			{ { 0x001F514Ful, COBL_DOMAIN_COUNT } }, /* number of domains */
			NULL
	},
	{ 0u, 1u, RES, 	{{ 0x001F5140ul, 0x00000000ul}},
			{ { 0x00FFFFFFul, 0xffFFffFFul } },
			{ { 0x011F514Ful, 0ul } }, /* program stopped */
			read1F51
	},

	/* for 0x1f51 subindex 0 */
	{ 0u, 0u, RES, { { 0x001F512Ful, 0x00000000ul } },
			MASK_ALLSET,
			{ { 0x001F5180ul, 0x06010002ul } }, /* write: program erase */
			NULL
	},
	/* for 0x1f51 comp 2., call, res, {{req}} */
	{ 1u, 1u, RES, 	{{ 0x001F512Ful, 0x00000003ul}},
			/* {{mask}} */
			{{0x00FFFFFFul, 0x000000FFul}},
			/* {{resp}} */
			{{ 0x011F5160ul, 0ul}}, /* write: program erase */
			/* call */
			write1F51Erase
	},
#ifdef COBL_SDO_PROG_START
	{ 1u, 1u, RES,{ { 0x011F512Ful, 0x00000001ul } },// only sub 1
				MASK_ALLSET,
				{ { 0x011F5160ul, 0ul } }, /* write: program start */
				write1F51Start
	},
#endif /* COBL_SDO_PROG_START				*/
	{ 0u, 0u, RES, 	{{ 0x001F5640ul, 0x00000000ul}},
			MASK_ALLSET,
			{ { 0x001F564Ful, COBL_DOMAIN_COUNT } }, /* number of domains */
			NULL
	},
	{ 0u, 1u, RES, 	{{ 0x001F5640ul, 0x00000000ul}},
			{ { 0x00FFFFFFul, 0xffFFffFFul } },
			{ { 0x011F564Ful, 0ul } }, /* crc check? */
			read1F56
	},
	{ 0u, 0u, RES, 	{{ 0x001F5740ul, 0x00000000ul}},
			MASK_ALLSET,
			{ { 0x001F574Ful, COBL_DOMAIN_COUNT } }, /* number of domains */
			NULL
	},
	{ 0u, 1u, RES, 	{{ 0x001F5740ul, 0x00000000ul}},
			{ { 0x00FFFFFFul, 0xffFFffFFul } },
			{{ 0x011F574Ful, 0ul}}, /* program valid */
			read1F57
	},
#ifdef COBL_OD_5F00_0_FCT
	{ 0, 1, RES, 	{{ 0x005F0040ul, 0x00000000ul}},
			MASK_ALLSET,
			{{ 0x005F0043ul, 0ul}}, /* application version */
			readU32
	},
#endif

};

/** number of entries */
const U16 odTableSize = sizeof(coOd)/sizeof(CoOd_t);


#ifdef COBL_DEBUG
/***************************************************************************/
/**
* \brief Debug output - object dictionary
*
* print the object dictionary table to stdout
*/
void printTable(void)
{
int i;
const CoOd_t * ptrOd;

	printf("Table entries: %d\n", (int)odTableSize);
	printf("---------------------------------------------\n");

	for (i = 0; i < odTableSize; i++)  {
		ptrOd = &coOd[i];
		if (ptrOd->bCompareSecond != 0)  {
			printf("Request: 0x%08lx 0x%08lx\n",
					(long)ptrOd->req.u32Data[0],
					(long)ptrOd->req.u32Data[1]);
			printf("Mask   : 0x%08lx 0x%08lx\n",
					(long)ptrOd->mask.u32Data[0],
					(long)ptrOd->mask.u32Data[1]);
		} else {
			printf("Request : 0x%08lx ----\n",
					(long)ptrOd->req.u32Data[0]);
			printf("Mask    : 0x%08lx ----\n",
					(long)ptrOd->mask.u32Data[0]);
		}
		if (ptrOd->bCallFunction != 0)  {
			printf("Function: %p\n", ptrOd->ptrCallFunction);
		} else {
			printf("Response: 0x%08lx 0x%08lx\n",
					(long)ptrOd->resp.u32Data[0],
					(long)ptrOd->resp.u32Data[1]);
		}
		printf("---------------------------------------------\n");
	}


}
#endif /* COBL_DEBUG */
