/*
* codrv_dcan.c - D-CAN driver 
*
* Copyright (c) 2018-2019 emotas embedded communication GmbH
*-------------------------------------------------------------------
* $Id: codrv_dcan.c 37191 2021-07-16 12:17:09Z ro $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \brief Bosch D-CAN driver
*
* \file 
* \author emtas GmbH
*
* This module contains the D_CAN driver package.
* The driver base on the Bosch C_CAN driver. That's why the
* naming start with CCAN.
* It was adapted to TMS320F28379D.
*
* State:
* - Transmit and receive data frames.
* - Receive Remote Frames (in Filter mode only)
* - Use both polling and interrupt.
* - Check error states.
*
*/

/* header of standard C - libraries
---------------------------------------------------------------------------*/
#pragma CHECK_MISRA("none")
#include <stddef.h>
#pragma RESET_MISRA("all")



/* header of project specific types
---------------------------------------------------------------------------*/
#include <gen_define.h>

#include <co_datatype.h>
#include <co_drv.h>
#include <co_commtask.h>

#include <codrv_error.h>
#include <codrv_dcan.h>
#include <stdio.h>

/* driver require pointer arithmetic
 * check 12.8 is incorrect
 * check 5.6, 5.7 - same name for same functionality
 * check 11.3 - pointer to periphery address
 * check 17.4 - pointer arithmetic for CAN register address
 * check 19.7 - function like macro
 * ---------------------------------------------------------------------------*/
#pragma CHECK_MISRA("-5.6,-5.7,-11.3,-12.8,-17.4,-19.7")

/* unsure solution - disable for better check of different hints
 * ---------------------------------------------------------------------------*/
#pragma CHECK_MISRA("-10.1")


/* constant definitions
---------------------------------------------------------------------------*/
#define CCAN_TX_OBJ 1u
#define CCAN_RX_OBJ 2u
#define CCAN_RX_RTR_OBJ 3u

/* OS related macros - default definition */
#ifdef CO_OS_SIGNAL_CAN_TRANSMIT
#else
#  define CO_OS_SIGNAL_CAN_TRANSMIT
#endif

#ifdef CO_OS_SIGNAL_CAN_RECEIVE
#else
#  define CO_OS_SIGNAL_CAN_RECEIVE
#endif

#ifdef CO_OS_SIGNAL_CAN_STATE
#else
#  define CO_OS_SIGNAL_CAN_STATE
#endif


/**
* \define CODRV_DEBUG
* UART Debug output
*/

/* #define CODRV_DEBUG 1 */

/* additional supported Setting
* CO_DRV_FILTER - activate acceptance filter
* CO_DRV_GROUP_FILTER - receive all HB Consumer and EMCY Consumer
*                       messages independent of its configuration.
*
* Note: This settings should be part of gen_define.h!
*/
/* #define CO_DRV_FILTER 1 */
/* #define CO_DRV_GROUP_FILTER 1 */


/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/
#ifdef CODRV_DEBUG
# ifdef CO_DRV_FILTER
void printFilter(void);
# endif
#endif

/* list of local defined functions
---------------------------------------------------------------------------*/
static RET_T codrvCanTransmit(CO_CONST CO_CAN_TR_MSG_T * pBuf,
		UNSIGNED32 mnum);
static CO_CONST CODRV_BTR_T * codrvCanGetBtrSettings(UNSIGNED16 bitRate);


static void codrvCanTransmitInterrupt(UNSIGNED32 mnum);
static void codrvCanReceiveInterrupt(UNSIGNED32 mnum);
static void codrvCanErrorInterrupt(void);
static void codrvCanErrorHandler(void);

#ifdef DRIVER_TEST
#include <co_canopen.h>
#include <../src/ico_cobhandler.h>
#include <../src/ico_queue.h>

RET_T transmitFrames(void);
#endif /* DRIVER_TEST */


#ifdef CO_DRV_FILTER
#  ifdef CO_DRV_GROUP_FILTER
static BOOL_T codrvCanCheckGroupFilter( CO_CONST CO_CAN_COB_T * CO_CONST pCob);
#  endif /* CO_DRV_GROUP_FILTER  */
static UNSIGNED16 codrvCanGetChan( CO_CAN_COB_T * CO_CONST pCob);
static void codrvCanRxBufferFilterInit( UNSIGNED32 mnum, UNSIGNED32 id,
		UNSIGNED32 mask);
static void codrvCanExtRxBufferFilterInit( UNSIGNED32 mnum, UNSIGNED32 id,
		UNSIGNED32 mask);
#else /* CO_DRV_FILTER */
static void codrvCanRxBufferInit( UNSIGNED32 mnum);
static void codrvCanRxRTRBufferInit( UNSIGNED32 mnum);
#endif /* CO_DRV_FILTER */

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
static BOOL_T canEnabled = CO_FALSE; /**< CAN bus on */
static BOOL_T transmissionIsActive = CO_FALSE; /**< TX transmission active */


/** currently TX message */
static CO_CAN_TR_MSG_T *pTxBuf = NULL;

/** CAN Controller address (see codrv_dcan.h) */
static volatile UNSIGNED32 *  CO_CONST pCan = (UNSIGNED32*) CAN_BASE_ADDRESS;


#ifdef CODRV_BIT_TABLE_EXTERN
extern CO_CONST CODRV_BTR_T codrvCanBittimingTable[];
#else /* CODRV_BIT_TABLE_EXTERN */

/** can bit timing table */
static CO_CONST CODRV_BTR_T codrvCanBittimingTable[5] = {
		/* 100MHz table, pre-scaler 6bit (max 64) + BRPE 4bit == 1024 */
		{   125u,  50u, 0u, 13u, 2u }, /* ! 87.5% */
		{   250u,  25u, 0u, 13u, 2u }, /* ! 87.5% */
		{   500u,  20u, 0u, 8u, 1u },  /* ! 90% */
		{  1000u,  10u, 0u, 8u, 1u }, /*  ! 90% */
		{0u,0u,0u,0u,0u} /* last */
	};
#endif /* CODRV_BIT_TABLE_EXTERN */


/* internal definitions
 * DEBUG_SEND_TESTMESSAGE - send a CAN message after initialization 
 *                          without using interrupts
 * CODRV_CAN_POLLING - use the driver in polling mode without interrupts
 */


#ifdef CO_DRV_FILTER
/**
* nextMsgObj counts the used acceptance filter.
*/
static UNSIGNED16 nextMsgObj;
#endif /* CO_DRV_FILTER */

#ifdef DEBUG_SEND_TESTMESSAGE
/***************************************************************************/
/**
* \brief codrvSendTestMessage - send a test message
*
* Send a CAN message without interrupts.
* This will test the access to the CAN controller 
* an can be used to measure the CAN bit timing.
*
* Test message:
* 0x555:8:0102030405060708
*/
static void codrvSendTestMessage(void)
{
	/* Bus on */
	pCan[CCAN_CNTL] = 0u;

	/* only base ids (IDbit 28..18) /Data frames */
	pCan[CCAN_IF_ARB(1u)] = ((UNSIGNED32)0x555ul << 18)
						| CCAN_IFARB_DIR_TX
						| CCAN_IFARB_MSGVAL;
	pCan[CCAN_IF_DA(1u)] = 0x03040201u;
	pCan[CCAN_IF_DB(1u)] = 0x08070605u;


	pCan[CCAN_IF_MCTRL(1u)] = 8u
						| CCAN_IFMCTRL_EOB
						| CCAN_IFMCTRL_TXRQST
						| CCAN_IFMCTRL_NEWDAT;

	pCan[CCAN_IF_CMDMSK(1u)] = CCAN_CMDMSK_WR | CCAN_CMDMSK_ARB
						| CCAN_CMDMSK_CTRL | CCAN_CMDMSK_DATA_A
						| CCAN_CMDMSK_DATA_B | CCAN_CMDMSK_WR_TXRQST
						| CCAN_TX_OBJ;

	while ( (pCan[CCAN_IF_CMDREQ(1u)] & CCAN_CMDREQ_BUSY) != 0u)  {};

}

#endif /* DEBUG_SEND_TESTMESSAGE */
/*---------------------------------------------------------------------------*/


#ifdef CO_DRV_FILTER
/***************************************************************************/
/**
* \brief codrvCanRxBufferFilterInit - initialize receive buffer of the CAN controller
*
* This function initialize the Receive Message box of the CAN controller
* for Full CAN. This means, the Receive Filter are configured to receive 
* only the configured messages.
*
* Currently only receive base frames.
*/
static void codrvCanRxBufferFilterInit(
		UNSIGNED32 mnum, /**< C_CAN Message buffer number */
		UNSIGNED32 id, /* AccFilter */
		UNSIGNED32 mask /* Acceptance mask */
	)
{
	/* only base ids (IDbit 28..18) /Data & RTR frames */
	pCan[CCAN_IF_MASK(1u)] = (mask << 18) /* masked ID bits */
							| CCAN_IFMASK_MXTD;

	/* only base ids (IDbit 28..18) /Data frames */
	pCan[CCAN_IF_ARB(1u)] = (id << 18) /* ID parts */
						| CCAN_IFARB_DIR_RX /* data frame */
						| CCAN_IFARB_MSGVAL;
						/* + base identifier only */

	/* enable interrupts and acceptance filter */
	pCan[CCAN_IF_MCTRL(1u)] = CCAN_IFMCTRL_EOB
						| CCAN_IFMCTRL_RXIE
						| CCAN_IFMCTRL_UMASK;

	pCan[CCAN_IF_CMDMSK(1u)] = CCAN_CMDMSK_WR | CCAN_CMDMSK_ARB
	                        | CCAN_CMDMSK_CTRL | CCAN_CMDMSK_MASK
	                        | mnum;

	while ( (pCan[CCAN_IF_CMDREQ(1u)] & CCAN_CMDREQ_BUSY) != 0u)  {};
}

/***************************************************************************/
/**
* \brief codrvCanExtRxBufferFilterInit - initialize receive buffer of the CAN controller
*
* This function initialize the Receive Message box of the CAN controller
* for Full CAN. This means, the Receive Filter are configured to receive
* only the configured messages.
*
* Currently only receive extended frames.
*/
static void codrvCanExtRxBufferFilterInit(
		UNSIGNED32 mnum, /**< C_CAN Message buffer number */
		UNSIGNED32 id, /* AccFilter */
		UNSIGNED32 mask /* Acceptance mask */
	)
{
	/* only ext. ids /Data & RTR frames */
	pCan[CCAN_IF_MASK(1u)] = (mask & 0x1FFFFFFFul)/* masked ID bits */
							| CCAN_IFMASK_MXTD;

	/* only base ids (IDbit 28..18) /Data frames */
	pCan[CCAN_IF_ARB(1u)] = (id & 0x1FFFFFFFul) /* ID parts */
						| CCAN_IFARB_DIR_RX /* data frame */
						| CCAN_IFARB_MSGVAL
						| CCAN_IFARB_XTD;
						/* + base identifier only */

	/* enable interrupts and acceptance filter */
	pCan[CCAN_IF_MCTRL(1u)] = CCAN_IFMCTRL_EOB
						| CCAN_IFMCTRL_RXIE
						| CCAN_IFMCTRL_UMASK;

	pCan[CCAN_IF_CMDMSK(1u)] = CCAN_CMDMSK_WR | CCAN_CMDMSK_ARB
	                        | CCAN_CMDMSK_CTRL | CCAN_CMDMSK_MASK
	                        | mnum;

	while ( (pCan[CCAN_IF_CMDREQ(1u)] & CCAN_CMDREQ_BUSY) != 0u)  {};
}


#else /* CO_DRV_FILTER */
/***************************************************************************/
/**
* \brief codrvRxBufferInit - initialize receive buffer of the CAN controller
*
* This function initialize the Receive Message box of the CAN controller
* for Basic CAN. This means, the Receive Filter are configured to receive 
* all messages.
*
* Currently only receive base data frames.
*/
static void codrvCanRxBufferInit(
		UNSIGNED32 mnum /**< C_CAN Message buffer number */
	)
{
	pCan[CCAN_IF_MASK(1u)] = 0x0000u; /* receive all */
	                                   /* all IDs, ext. RTR */

	/* only base ids (IDbit 28..18) /Data frames */
	pCan[CCAN_IF_ARB(1u)] = 0x0000u /* all IDs - value irrelevant */
						| CCAN_IFARB_DIR_RX /* data frame */
						| CCAN_IFARB_MSGVAL;
						/* + base identifier only */

	/* enable interrupts and acceptance filter */
	pCan[CCAN_IF_MCTRL(1u)] = CCAN_IFMCTRL_EOB
						| CCAN_IFMCTRL_RXIE
						| CCAN_IFMCTRL_UMASK;

	pCan[CCAN_IF_CMDMSK(1u)] = CCAN_CMDMSK_WR | CCAN_CMDMSK_ARB
	                        | CCAN_CMDMSK_CTRL | CCAN_CMDMSK_MASK | mnum;
	while ( (pCan[CCAN_IF_CMDREQ(1u)] & CCAN_CMDREQ_BUSY) != 0u)  {};
}

/***************************************************************************/
/**
* \brief codrvRxBufferInit - initialize receive buffer of the CAN controller
*
* This function initialize the Receive Message box of the CAN controller
* for Basic CAN. This means, the Receive Filter are configured to receive
* all messages.
*
* Currently only receive base data frames.
*/
static void codrvCanRxRTRBufferInit(
		UNSIGNED32 mnum /**< C_CAN Message buffer number */
	)
{
	pCan[CCAN_IF_MASK(1u)] = 0x0000u; /* receive all */
	                                   /* all IDs, ext. RTR */

	/* only base ids (IDbit 28..18) /Data frames */
	pCan[CCAN_IF_ARB(1u)] = 0x0000u /* all IDs - value irrelevant */
						| CCAN_IFARB_DIR_TX /* rtr frame */
						| CCAN_IFARB_MSGVAL;
						/* + base identifier only */

	/* enable interrupts and acceptance filter */
	pCan[CCAN_IF_MCTRL(1u)] = CCAN_IFMCTRL_EOB
						| CCAN_IFMCTRL_RXIE
						| CCAN_IFMCTRL_UMASK;


	pCan[CCAN_IF_CMDMSK(1u)] = CCAN_CMDMSK_WR | CCAN_CMDMSK_ARB
	                        | CCAN_CMDMSK_CTRL | CCAN_CMDMSK_MASK | mnum;

	while ( (pCan[CCAN_IF_CMDREQ(1u)] & CCAN_CMDREQ_BUSY) != 0u)  {};
}

#endif /* CO_DRV_FILTER */


/***************************************************************************/
/**
* \brief codrvCanInit - initialize CAN controller
*
* This function initializes the CAN controller and configures the bit rate.
* At the end of the function, the CAN controller should be in state disabled.
*
* \return RET_T
* \retval RET_OK
*	initialization was ok
*
*/
RET_T codrvCanInit(
		UNSIGNED16	bitRate		/**< CAN bitrate */
	)
{
RET_T	retVal = RET_OK;
UNSIGNED8 mn;

	/* init req. variables */
	canEnabled = CO_FALSE;
	transmissionIsActive = CO_FALSE;

	pTxBuf = NULL;

	/* error states */
	codrvCanErrorInit();

	/* initialize CAN controller, setup timing, pin description, CAN mode ...*/
	pCan[CCAN_CNTL] = CCAN_CNTL_INIT | CCAN_CNTL_CCE;

	/* reset CAN and RAM */
	__eallow();
	while((pCan[CCAN_CNTL] & CCAN_CNTL_INIT) == 0);
	pCan[CCAN_CNTL] = CCAN_CNTL_INIT | CCAN_CNTL_CCE | CCAN_CNTL_SWR;
	while((pCan[CCAN_CNTL] & CCAN_CNTL_SWR) == CCAN_CNTL_SWR);
	pCan[CCAN_RAM_INIT] = CCAN_RAM_INIT_START | 0xA;
	while((pCan[CCAN_RAM_INIT] & CCAN_RAM_INIT_DONE) == 0);
	__edis();

	/* set bitrate */
	retVal = codrvCanSetBitRate(bitRate);
	if (retVal != RET_OK)  {
		return(retVal);
	}

	for ( mn = (UNSIGNED8)1u; mn <= (UNSIGNED8)CCAN_MSGOBJ_NUM; mn++) {
		pCan[CCAN_IF_ARB(1u)] = 0u; /* Msg invalid */

		/* write to nm message object */
		pCan[CCAN_IF_CMDMSK(1u)] = (CCAN_CMDMSK_WR | CCAN_CMDMSK_ARB | mn);

		while ( (pCan[CCAN_IF_CMDMSK(1u)] & CCAN_CMDREQ_BUSY) != 0u)  {};
	}

	/* configure the receive buffer */
#ifdef CO_DRV_FILTER

	nextMsgObj = CCAN_RX_OBJ;

#  ifdef CO_DRV_GROUP_FILTER
#    ifdef CO_HB_CONSUMER_CNT
	/* create RX Buffer for all heart beat messages */
	codrvCanRxBufferFilterInit(nextMsgObj, 0x700ul /* ID */, 0x780ul /* mask */ );
	nextMsgObj++;
#    endif /* CO_HB_CONSUMER_CNT */
#    ifdef CO_EMCY_CONSUMER_CNT
	/* create RX Buffer for all Emergency's */
	codrvCanRxBufferFilterInit(nextMsgObj, 0x80ul /* ID */, 0x780ul /* mask */ );
	nextMsgObj++;
#	 endif /* CO_EMCY_CONSUMER_CNT */
#  endif /* CO_DRV_GROUP_FILTER */

#else /* CO_DRV_FILTER */

	/* create RX Buffer */
	codrvCanRxBufferInit(CCAN_RX_OBJ);
	codrvCanRxRTRBufferInit(CCAN_RX_RTR_OBJ);

#endif /* CO_DRV_FILTER */


#ifdef DEBUG_SEND_TESTMESSAGE
	codrvCanEnable();
	codrvSendTestMessage();
#endif /* DEBUG_SEND_TESTMESSAGE */

	return(retVal);
}

/***********************************************************************/
/**
* codrvCanGetBtrSettings - get pointer to the btr value structure
*
* \internal
*
* \returns
*	pointer to an btr table entry
*/

static CO_CONST CODRV_BTR_T * codrvCanGetBtrSettings(
		UNSIGNED16 bitRate	/**< required bitrate */
	)
{
CO_CONST CODRV_BTR_T * pBtrEntry = NULL;
UNSIGNED8 i;

	i = 0u;
	while (codrvCanBittimingTable[i].bitRate != 0u)  {
		if (codrvCanBittimingTable[i].bitRate == bitRate)  {
			pBtrEntry = &codrvCanBittimingTable[i];
			break;
		}
		i++;
	}

	return(pBtrEntry);
}


/***********************************************************************/
/**
* \brief codrvCanSetBitRate - set CAN bitrate
*
* This function sets the CAN bitrate to the given value.
* Changing the Bitrate is only allowed, if the CAN controller is in reset.
* The state at the start of the function is unknown, 
* so the CAN controller should be switch to state reset.
*
* At the end of the function the CAN controller should be stay in state reset.
*
* \return RET_T
* \retval RET_OK
*	setup bitrate was ok
*
*/
RET_T codrvCanSetBitRate(
		UNSIGNED16	bitRate		/**< CAN bitrate in kbit/s */
	)
{
UNSIGNED32 pre;
UNSIGNED32 seg1;
UNSIGNED32 seg2;
UNSIGNED32 brpe = 0u;
CO_CONST CODRV_BTR_T * pBtrEntry;

	/* stop CAN controller */
	codrvCanDisable();

	pBtrEntry = codrvCanGetBtrSettings(bitRate);

	if (pBtrEntry == NULL)  {
		/* if bitrate not supported */
		return(RET_DRV_WRONG_BITRATE);
	}

	/* set configuration bit */
	pCan[CCAN_CNTL] = CCAN_CNTL_INIT | CCAN_CNTL_CCE;

	pre = pBtrEntry->pre;
	seg1 = pBtrEntry->seg1 + pBtrEntry->prop;
	seg2 = pBtrEntry->seg2;

	pre -= 1ul; /* now register value! */
	
	if (pre > 64)  {
		brpe = pre/64;
		pre %= 64;
	}
	
	
	pCan[CCAN_BIT] =  (brpe << 16)
					| ((seg2 - 1ul) << 12)
					| ((seg1 - 1ul) << 8)
					| (pre & 0x3Ful);  /* SJW = 1tq */

	return(RET_OK);
}


/***********************************************************************/
/**
* \brief codrvCanEnable - enable CAN controller
*
* This function enables the CAN controller.
*
* \return RET_T
* \retval RET_OK
*	CAN controller is enabled
*
*/
RET_T codrvCanEnable(
		void
	)
{
RET_T	retVal = RET_OK;

	/* enable CAN controller */
	pCan[CCAN_CNTL] = 0u; /* Bus on */

	/* Error active is later checked */

	/* enable the interrupts, Auto buson */
	codrvCanEnableInterrupt();
	pCan[CCAN_CNTL] = (CCAN_CNTL_IE | CCAN_CNTL_EIE | CCAN_CNTL_ABO);

#ifdef CODRV_DEBUG
# ifdef CO_DRV_FILTER
	printFilter();
# endif /* CO_DRV_FILTER */
#endif /* CODRV_DEBUG */

	return(retVal);
}


/***********************************************************************/
/**
* \brief codrvCanDisable - disable CAN controller
*
* This function disables the CAN controller.
*
* \return RET_T
* \retval RET_OK
*	CAN controller is disabled
*
*/
RET_T codrvCanDisable(
		void
	)
{
RET_T	retVal = RET_OK;

	/* set CAN in init state */
	pCan[CCAN_CNTL] = CCAN_CNTL_INIT;

	/* disable CAN controller */
	canEnabled = CO_FALSE;

	return(retVal);
}

#ifdef CO_DRV_FILTER
#  ifdef CO_DRV_GROUP_FILTER

/***********************************************************************/
/**
* \brief codrvCheckGroupFilter - check, if the canId part of the group filter
*
* Depend of some settings the group filter are for the IDs
*	0x700..0x77F - Heartbeat
*	0x80..0xFF   - Emergency (and default Sync)
*
* \return BOOL_T
* \retval CO_TRUE
*	The ID is part of the group filter.
* \retval CO_FALSE
*	The ID is not part of the group filter.
*
*/
static BOOL_T codrvCanCheckGroupFilter(
		CO_CONST CO_CAN_COB_T * CO_CONST pCob
	)
{
BOOL_T retval;
UNSIGNED32 canId = pCob->canId;

	retval = CO_FALSE;

	if ((pCob->flags & CO_COBFLAG_RTR)  != 0)
	{
#    ifdef CO_HB_CONSUMER_CNT
		if ((canId & 0x0780u) == 0x700u)  {
			/* part of the group filter */
			retval = CO_TRUE;
		}
#    endif /* CO_HB_CONSUMER_CNT */
#    ifdef CO_EMCY_CONSUMER_CNT
		if ((canId & 0x0780u) == 0x80u)  {
			/* part of the group filter */
			retval = CO_TRUE;
		}
#    endif /* CO_EMCY_CONSUMER_CNT */
	}

	return(retval);
}
#  endif /* CO_DRV_GROUP_FILTER */

/***********************************************************************/
/**
* codrvCanGetChan - get or calculate the filter entry 
*
* Check, if the COB require a receive filter entry (Chan = channel).
* If required, calculate a new entry.
*
* The new calculate channel number is written within the COB entry.
*
* \returns
* channel == message object
*
* \retval 0xFFFF
*	no filter required or no free entry
* 
*/
static UNSIGNED16 codrvCanGetChan(
		CO_CAN_COB_T * CO_CONST pCob
	)
{
const UNSIGNED16 maxFilter = CCAN_MSGOBJ_NUM;
BOOL_T reserveChan = CO_FALSE;

#  ifdef CO_DRV_GROUP_FILTER
	if (codrvCanCheckGroupFilter(pCob) == CO_TRUE)  {
		/* nothing */
	} else
#  endif /* CO_DRV_GROUP_FILTER */

	if ((pCob->flags & CO_COBFLAG_ENABLED) == 0)  {
		/* no channel/filter reservation req.  
		* But in case the COB has a filter assigned, 
		* this filter has to be disabled
		*/
	} else
	if ((pCob->flags & CO_COBFLAG_RTR) == 0)
	{
		/* Receive Data frame */
		reserveChan = CO_TRUE;
	} else
	if ((pCob->flags & CO_COBFLAG_RTR) != 0)
	{
		/* Receive RTR frame */
		reserveChan = CO_TRUE;
	} else {
		/* nothing */
	}

	if ((reserveChan == CO_TRUE) && (pCob->canChan == 0xFFFFu))
	{
		if (nextMsgObj > maxFilter)  {
			/* no free filter */
		} else {
			pCob->canChan = nextMsgObj;
			nextMsgObj++;
		}
	}

	return(pCob->canChan); /* no Channel == 0xFFFFu */
}


/***********************************************************************/
/**
* codrvCanSetFilter - activate and configure the receive filter
*
* Depend of the COB entry's the driver specific filter will 
* be configured. 
*
*
* \retval RET_OK
*	OK
* \retval RET_INVALID_PARAMETER
*	invalid COB reference
* \retval RET_DRV_ERROR
*	filter cannot be set, e.g. no free entry
*
*/

RET_T codrvCanSetFilter(
		CO_CAN_COB_T * pCanCob /**< COB reference */
	)
{
UNSIGNED16 canChan;
BOOL_T error = CO_FALSE;

#ifdef CODRV_DEBUG
	printf("codrvCanSetFilter: 0x%04x rtr: %d enabled: %d\n", pCanCob->canId, pCanCob->rtr, pCanCob->enabled);
#endif

	/* get a filter entry in case of:
	 * - COB has a filter entry from older setting or
	 * - COB is enabled and
	 * - COB is a Receive Data Frame or
	 * - COB is a Transmit Data Frame, but can be query by RTR
	 */

	canChan = codrvCanGetChan(pCanCob);

	if ((pCanCob->flags & CO_COBFLAG_ENABLED) != 0)
	{
		if ((pCanCob->flags & CO_COBFLAG_RTR) == 0)
		{
			/* Receive Data frame */
			if (canChan == 0xFFFFu)  {
				error = CO_TRUE;
			} else {
				if ((pCanCob->flags & CO_COBFLAG_EXTENDED) == 0)  {
					codrvCanRxBufferFilterInit(canChan, pCanCob->canId, 0x7FFul /* 11bit */ );
				} else {
					codrvCanExtRxBufferFilterInit(canChan, pCanCob->canId, 0x1FFFFFFFul /* 29bit */ );
				}
			}
		} else {

			/* Receive RTR frame */
			if (canChan == 0xFFFFu)  {
				error = CO_TRUE;
			} else {
				if ((pCanCob->flags & CO_COBFLAG_EXTENDED) == 0)  {
					codrvCanRxBufferFilterInit(canChan, pCanCob->canId, 0x7FFul /* 11bit */ );
				} else {
					codrvCanExtRxBufferFilterInit(canChan, pCanCob->canId, 0x1FFFFFFFul /* 29bit */ );
				}
			}
		}

	} else {
		/* deactivate filter */
		if (canChan == 0xFFFFu)  {
			/* nothing to do */	
		} else
#  ifdef CO_DRV_GROUP_FILTER
		if (codrvCanCheckGroupFilter(pCanCob) == CO_TRUE)  {
			/* nothing */
		} else
#  endif
		{
			pCan[CCAN_IF_ARB(1u)] = 0u; /* Msg invalid */

			pCan[CCAN_IF_CMDMSK(1u)] = CCAN_CMDMSK_WR | CCAN_CMDMSK_ARB |
										canChan; /* write to message object */

			while ( (pCan[CCAN_IF_CMDREQ(1u)] & CCAN_CMDREQ_BUSY) != 0u)  {};
		}
	}

	if (error == CO_TRUE)  {
		return(RET_DRV_ERROR);
	}

#  ifdef CODRV_DEBUG
	printFilter();
#  endif /* CODRV_DEBUG */

	return(RET_OK);
}
#endif /* CO_DRV_FILTER */


/***********************************************************************/
/**
* \brief codrvCanStartTransmission - start can transmission if not active
*
* Transmission of CAN messages should be interrupt driven.
* If a message was sent, the Transmit Interrupt is called
* and the next message can be transmitted.
* To start the transmission of the first message,
* this function is called from the CANopen stack.
*
* The easiest way to implement this function is
* to trigger the transmit interrupt, 
* but only of the tranmission is not already active.
*
* \return RET_T
* \retval RET_OK
*	start transmission was succesful
*
*/
RET_T codrvCanStartTransmission(
		void
	)
{
#ifdef CODRV_CAN_POLLING
#else
UNSIGNED32 canCtrlReg;
#endif
	/* if can is not enabled, return with error */
	if (canEnabled != CO_TRUE)  {
		return(RET_DRV_ERROR);
	}

	if (transmissionIsActive == CO_FALSE)  {
		/* trigger transmit interrupt */
#ifdef CODRV_CAN_POLLING
		codrvCanInterrupt();
#else
		/* TX IRQ call with disabled interrupt */
		canCtrlReg = pCan[CCAN_CNTL];
		canCtrlReg &= (~CCAN_CNTL_IE);
		pCan[CCAN_CNTL] = canCtrlReg;


		/* transmit CAN message */
		codrvCanTransmitInterrupt(CCAN_TX_OBJ);

		canCtrlReg |= CCAN_CNTL_IE;
		pCan[CCAN_CNTL] = canCtrlReg;
#endif

	}

	return(RET_OK);
}


/***********************************************************************/
/**
* \brief codrvCanTransmit - transmit can message
*
* This function writes a new message to the CAN controller and transmits it.
* Normally called from Transmit Interrupt
*
* \return RET_T
* \retval RET_OK
*	Transmission was ok
*
*/
static RET_T codrvCanTransmit(
		CO_CONST CO_CAN_TR_MSG_T * pBuf,		/**< pointer to data */
		UNSIGNED32 mnum		/**< CCAN message buffer number */
	)
{
RET_T	retVal = RET_OK;
UNSIGNED32 dir;

	/* set invalid */
	pCan[CCAN_IF_ARB(1u)] =  CCAN_IFARB_DIR_TX; /* not valid */
	pCan[CCAN_IF_CMDMSK(1u)] = CCAN_CMDMSK_WR | CCAN_CMDMSK_ARB | mnum;
	while ( (pCan[CCAN_IF_CMDREQ(1u)] & CCAN_CMDREQ_BUSY) != 0u)  {};

	/* configure new message */

	/* write message to the CAN controller */
	dir = CCAN_IFARB_DIR_TX;
	if ((pBuf->flags & CO_COBFLAG_RTR) != 0)  {
	    /* RTR message */
		dir = CCAN_IFARB_DIR_RX;
	}

	/* ID */
	if ((pBuf->flags & CO_COBFLAG_EXTENDED) == 0)  {
		/* only base ids (IDbit 28..18) /Data frames */
		pCan[CCAN_IF_ARB(1u)] = ((UNSIGNED32)pBuf->canId << 18)
						| (UNSIGNED32)CCAN_IFARB_MSGVAL
						| dir;
	} else {
	    pCan[CCAN_IF_ARB(1u)] = pBuf->canId
	                        | (UNSIGNED32)CCAN_IFARB_MSGVAL
	                        | (UNSIGNED32)CCAN_IFARB_XTD
	                        | dir;
	}

	if ((pBuf->flags & CO_COBFLAG_RTR) == 0)  {
		pCan[CCAN_IF_DA(1u)] = ((UNSIGNED32)pBuf->data[0]
							 | ((UNSIGNED32)pBuf->data[1] << 8)
		                     | ((UNSIGNED32)pBuf->data[2] << 16)
							 | ((UNSIGNED32)pBuf->data[3] << 24));
		pCan[CCAN_IF_DB(1u)] = ((UNSIGNED32)pBuf->data[4]
							 | ((UNSIGNED32)pBuf->data[5] << 8)
		                     | ((UNSIGNED32)pBuf->data[6] << 16)
							 | ((UNSIGNED32)pBuf->data[7] << 24));
	}

	pCan[CCAN_IF_MCTRL(1u)] = (UNSIGNED32)pBuf->len
						| CCAN_IFMCTRL_EOB
						| CCAN_IFMCTRL_TXRQST
						| CCAN_IFMCTRL_TXIE
						| CCAN_IFMCTRL_NEWDAT;

	/* transmit it */
	pCan[CCAN_IF_CMDMSK(1u)] = CCAN_CMDMSK_WR | CCAN_CMDMSK_ARB
						| CCAN_CMDMSK_CTRL | CCAN_CMDMSK_DATA_A
						| CCAN_CMDMSK_DATA_B | CCAN_CMDMSK_WR_TXRQST
						| mnum;
	/* wait some clocks */
	while ( (pCan[CCAN_IF_CMDREQ(1u)] & CCAN_CMDREQ_BUSY) != 0u)  {};

	transmissionIsActive = CO_TRUE;

	return(retVal);
}


/***********************************************************************/
/**
* \brief codrvCanDriverTransmitInterrupt - can driver transmit interrupt
*
* This function is called, after a message was transmitted.
*
* As first, inform stack about message transmission.
* Get the next message from the transmit buffer,
* write it to the CAN controller
* and transmit it.
*
* \return void
*
*/
static void codrvCanTransmitInterrupt(
		UNSIGNED32 mnum /**< CCAN message buffer number */
	)
{
UNSIGNED32 txreq;

	if (mnum == 0)  {
		return; /* internal error - never call with 0 */
	}

	/* more for polling - software interrupt, but TX object was sended */
	txreq = pCan[CCAN_TXRQ21];

	if ((txreq & (1ul << (mnum - 1u))) == 0ul) {
		transmissionIsActive = CO_FALSE;

		/* set msgobj invalid */
		pCan[CCAN_IF_ARB(1u)] = 0u; /* Msg invalid */
		pCan[CCAN_IF_CMDMSK(1u)] = CCAN_CMDMSK_WR | CCAN_CMDMSK_ARB | mnum;
		while ( (pCan[CCAN_IF_CMDREQ(1u)] & CCAN_CMDREQ_BUSY) != 0u)  {};
	}

	if (transmissionIsActive == CO_FALSE)  {

		/* inform stack about transmitted message */
		if (pTxBuf != NULL)  {
			coQueueMsgTransmitted(pTxBuf);
			pTxBuf = NULL;
		}

		/* get next message from transmit queue */
		pTxBuf = coQueueGetNextTransmitMessage();
		if (pTxBuf != NULL)  {
			/* and transmit it */
			(void)codrvCanTransmit(pTxBuf, mnum);
		}
	}
}


/***********************************************************************/
/**
* \brief codrvCanReceiveInterrupt - can driver receive interrupt
*
* This function is called, if a new message was received.
* As first get the pointer to the receive buffer
* and save the message there.
* Then set the buffer as filled and inform the lib about new data.
*
* \return void
*
*/
static void codrvCanReceiveInterrupt(
		UNSIGNED32 mnum /**< CCAN message buffer number */
	)
{
UNSIGNED8 *pRecDataBuf;
UNSIGNED32 id;
UNSIGNED8 len;
UNSIGNED32 data;
CAN_ERROR_FLAGS_T * pError;
UNSIGNED8 flags = CO_COBFLAG_NONE;
UNSIGNED8 tempBuffer[8u];

	if ((pCan[CCAN_IF_MCTRL(1u)] & CCAN_IFMCTRL_NEWDAT) == 0u)
	{
		/* no new data ?? */
		return;
	}

	pError = codrvCanErrorGetFlags();

	if ((pCan[CCAN_IF_MCTRL(1u)] & CCAN_IFMCTRL_MSGLST) != 0u)  {
		pError->canErrorRxOverrun = CO_TRUE;

		/* reset message lost bit */
		pCan[CCAN_IF_MCTRL(1u)] &= ~ (CCAN_IFMCTRL_MSGLST
								| CCAN_IFMCTRL_NEWDAT
								| CCAN_IFMCTRL_INPND);

		pCan[CCAN_IF_CMDMSK(1u)] = CCAN_CMDMSK_WR
		    					| CCAN_CMDMSK_CTRL
								| mnum;

		while ( (pCan[CCAN_IF_CMDREQ(1u)] & CCAN_CMDREQ_BUSY) != 0u)  {};
	}

	id = pCan[CCAN_IF_ARB(1u)];
	if ((id & CCAN_IFARB_DIR_MSK) != 0u)  {
		/* TX - means RTR */
		flags |= CO_COBFLAG_RTR;
	}

	if ((id & CCAN_IFARB_XTD) != 0u)  {
		/* 29bit ID */
	    flags |= CO_COBFLAG_EXTENDED;
	}

	if ((pCan[CCAN_IF_MCTRL(1u)] & CCAN_IFMCTRL_TXRQST) != 0u)  {
		/* RTR - ignore */
		return;
	}

	if ((id & CCAN_IFARB_XTD) == 0u)  {
		/* only base frames supported */
		id = (id >> 18) & 0x7FFul;
	} else {
		/* 29bit ID */
		id &= 0x1FFFFFFFul;
	}

	/* set length of data */
	len = (UNSIGNED8)pCan[CCAN_IF_MCTRL(1u)] & (UNSIGNED8)0x0Fu;
	if (len > (UNSIGNED8)8u)  {
		len = (UNSIGNED8)8u;
	}

	/* get receiveBuffer */
	pRecDataBuf = coQueueGetReceiveBuffer(id, len, flags);
	if (pRecDataBuf != NULL)  {
		/* save message temporary buffer */
		data = (UNSIGNED32)pCan[CCAN_IF_DA(1u)];
		tempBuffer[0] = data & 0xff;
		tempBuffer[1] = (data >> 8) & 0xff;
		tempBuffer[2] = (data >> 16) & 0xff;
		tempBuffer[3] = (data >> 24) & 0xff;

		data = (UNSIGNED32)pCan[CCAN_IF_DB(1u)];
		tempBuffer[4] = data & 0xff;
		tempBuffer[5] = (data >> 8) & 0xff;
		tempBuffer[6] = (data >> 16) & 0xff;
		tempBuffer[7] = (data >> 24) & 0xff;

		/* copy data to message buffer that may be shorter than 8 byte */
		memcpy(pRecDataBuf, tempBuffer, len);

		/* set buffer filled */
		coQueueReceiveBufferIsFilled();
	}


	/* signal received message */
	CO_OS_SIGNAL_CAN_RECEIVE
}

/***********************************************************************/
/**
* \brief codrvCanErrorInterrupt - Error interrupt handler
*
* This function check for Error state changes.
*/
static void codrvCanErrorInterrupt(
		void
	)
{
UNSIGNED16 cansts;
CAN_ERROR_FLAGS_T * pError;

	pError = codrvCanErrorGetFlags();

	cansts = (UNSIGNED16)pCan[CCAN_STAT];

	if ((pCan[CCAN_CNTL] & CCAN_CNTL_INIT) != 0u)  {
		pError->canErrorBusoff = CO_TRUE;
	} else
	if ((cansts & CCAN_STAT_BOFF) != 0u)  {
		pError->canErrorBusoff = CO_TRUE;
	} else
	if ((cansts & CCAN_STAT_EPASS) != 0u)  {
		pError->canErrorPassive = CO_TRUE;
	} else {
		pError->canErrorActive = CO_TRUE;
	}

	/* signal changed CAN state */
	CO_OS_SIGNAL_CAN_STATE
}


/***********************************************************************/
/**
* \brief Interrupt Entry
*
* C_CAN use only one interrupt entry for TX, RX and Error.
* This function is the interrupt entry. It's check the interrupt
* source and call it.
*
* This function has also to consider that a software interrupt
* can an additional interrupt source.
*
*/
void codrvCanInterrupt(void)
{
UNSIGNED32 intid;
UNSIGNED32 mnum = 0ul;

	intid = pCan[CCAN_INT];
	intid &= 0x0000FFFF; /* INT0 */

	if (intid == 0x8000ul)  {
		codrvCanErrorInterrupt();
	} else
	if (intid == 0ul)  {
		/* software interrupt - no interrupt pending */
		mnum = CCAN_TX_OBJ;
	} else
	if ((intid >= 1ul) && (intid <= CCAN_MSGOBJ_NUM))  {
		mnum = intid;
	} else
	{
		/* not valid */
	}

	if (mnum == 0ul)  {
		return; /* Problem? */
	}

	pCan[CCAN_IF_CMDMSK(1u)] = CCAN_CMDMSK_RD | CCAN_CMDMSK_ARB
						| CCAN_CMDMSK_CTRL | CCAN_CMDMSK_DATA_A
						| CCAN_CMDMSK_DATA_B
						| CCAN_CMDMSK_CLRINTPND
						| CCAN_CMDMSK_RD_CLRNEWDAT
	                    | mnum;
	while ( (pCan[CCAN_IF_CMDREQ(1u)] & CCAN_CMDREQ_BUSY) != 0ul)  {};

	/* at this point the DIR bit could be checked */
	if (mnum == CCAN_TX_OBJ)  {
		codrvCanTransmitInterrupt(mnum);

#ifdef	CODRV_CAN_POLLING
#else
		/* signal transmitted message from ISR only */
		CO_OS_SIGNAL_CAN_TRANSMIT
#endif /* CODRV_CAN_POLLING */
	} else {
		codrvCanReceiveInterrupt(mnum);
	}
}

/***********************************************************************/
/**
* \brief codrvCanErrorHandler - local Error handler
*
* This function polls the current state of the CAN controller
* and checks explicitly all situation that are not signalled
* within the interrupts.
*
* To be called outside of interrupts!
* Typically called in codrvCanDriverHandler().
*
* \return void
*
*/
static void codrvCanErrorHandler(void)
{
UNSIGNED16 cansts;
UNSIGNED32 txreq;
CAN_ERROR_FLAGS_T * pError;
BOOL_T fStartTransmission = CO_FALSE;
static UNSIGNED16 errorWaitCounter = {0u};

	pError = codrvCanErrorGetFlags();

	cansts = (UNSIGNED16)pCan[CCAN_STAT];

	if ((cansts & CCAN_STAT_BOFF) != 0u)  {
		/* busoff - init */
		pError->canNewState = Error_Busoff; /* current state busoff */
	} else
	if ((pCan[CCAN_CNTL] & CCAN_CNTL_INIT) != 0u)  {
		/* busoff - init */
			pError->canNewState = Error_Offline; /* current state offline */
	} else
	if ((cansts & CCAN_STAT_EPASS) != 0u)  {
		/* error passive */
		pError->canNewState = Error_Passive; /* current state is passive */
	} else {
		/* error active */
		pError->canNewState = Error_Active; /* current state is active */
	}

	/* more for polling - software interrupt, but TX object was sended */
	txreq = pCan[CCAN_TXRQ21];

	if (txreq == 0u)  {
		/* correct possible Errors -> CAN has deactivated the transmission */
		transmissionIsActive = CO_FALSE;
	}

#ifdef not_used /* auto buson enabled */
	/* auto buson */
	if ((pError->canNewState == Error_Busoff)
		&& (canEnabled == CO_TRUE))
	{
		canEnabled = CO_FALSE;
		pCan[CCAN_CNTL] = 0u; /* bus on */
	}
#endif

	if (canEnabled == CO_FALSE)  {
		if ((pCan[CCAN_CNTL] & CCAN_CNTL_INIT) == 0u)  {
			/* CAN is active, now */
			/* disabled -> enabled */
			canEnabled = CO_TRUE;
			fStartTransmission = CO_TRUE;

		}
	}

	if (canEnabled == CO_TRUE)  {
		/* check for stopped transmissions */
		if ((transmissionIsActive == CO_FALSE) && (pTxBuf != NULL))  {
			/* transmission aborted, e.g. busoff, 
		     * discard message -> is done within the tx interrupt
			*/
			errorWaitCounter++;
			if (errorWaitCounter > 20)  {
				fStartTransmission = CO_TRUE;
				errorWaitCounter = 0;
			}
		} else {
			errorWaitCounter = 0;
		}
	} else {
		errorWaitCounter = 0;
	}

	if (fStartTransmission == CO_TRUE)  {
		(void)codrvCanStartTransmission(); /* -> call Interrupt at this point */
	}
}

/***********************************************************************/
/**
* \brief codrvCanDriverHandler - can driver handler
*
* This function is cyclically called from the CANopen stack
* to get the current CAN state
* (BUS_OFF, PASSIVE, ACTIVE).
*
* If a bus off was occured,
* this function should try to get to bus on again.
*
* \return void
*
*/
void codrvCanDriverHandler(
		void
	)
{
	/* send a few test message one time */
#ifdef DRIVER_TEST
	static BOOL_T sent = CO_FALSE;
	if ((sent == CO_FALSE) && (canEnabled == CO_TRUE))  {
		transmitFrames();
		sent = CO_TRUE;
	}
#endif /* DRIVER_TEST */

	/* check current state */
	codrvCanErrorHandler();

	/* inform stack about the state changes during two handler calls */
	(void)codrvCanErrorInformStack();

#ifdef CODRV_CAN_POLLING
	codrvCanInterrupt();
#endif

	return;
}


#ifdef CODRV_DEBUG
# ifdef CO_DRV_FILTER

/*#include <stdio.h>*/
void printFilter(void)
{
UNSIGNED16 msgObj;
UNSIGNED32 accCode;
UNSIGNED32 accMask;

	for (msgObj = 1u; (msgObj <= 32u) && (msgObj < nextMsgObj); msgObj++)
	{
		pCan[CCAN_IF_CMDMSK(1u)] = CCAN_CMDMSK_RD | CCAN_CMDMSK_ARB
							| CCAN_CMDMSK_MASK
							| CCAN_CMDMSK_CTRL | CCAN_CMDMSK_DATA_A
							| CCAN_CMDMSK_DATA_B | CCAN_CMDMSK_RD_NEWDAT
							| CCAN_CMDMSK_CLRINTPND
							| msgObj;
		while ( (pCan[CCAN_IF_CMDREQ(1u)] & CCAN_CMDREQ_BUSY) != 0u)  {};


		printf("MsgObj %2d - ", msgObj);
		if ((pCan[CCAN_IF_ARB2(1u)] & CCAN_IFARB2_MSGVAL ) == 0u)  {
			printf("disabled\n");
		} else {

			/* 11bit ID, no check of XTD, DIR */
			accCode = (pCan[CCAN_IF_ARB2(1u)] >> 2) & 0x7FFul;
			printf("ID 0x%04x ", (unsigned int)accCode);

			accMask = (pCan[CCAN_IF_MASK2(1u)] >> 2) & 0x7FFul;
			printf("mask 0x%04x\n", (unsigned int)accMask);

		}

	}

}
# endif /* CO_DRV_FILTER */
#endif


#ifdef DRIVER_TEST
/***********************************************************************/
/* Transmit Tests
* req. 1 additional COB -> CANopen Device Designer!
*
* 11bit Dataframe id 0x123
* 11bit RTR id 0x234
* 29bit dataframe in 0x345
* 29bit RTR id 0x456
*/
/***********************************************************************/
RET_T transmitFrames(
	 	void
	 )
{
static volatile RET_T           retVal;
static COB_REFERENZ_T   drvCob = 0xffffu;
static UNSIGNED8 trData[8] = {1,2,3,4,5,6,7,8};

    if (drvCob == 0xffffu)  {
        drvCob = icoCobCreate(CO_COB_TYPE_TRANSMIT, CO_SERVICE_CAN_DEBUG, 0u);
        if (drvCob == 0xffffu)  {
            return(RET_EVENT_NO_RESSOURCE);
        }
    }

    icoCobChangeType(drvCob, CO_COB_TYPE_TRANSMIT);
    retVal = icoCobSet(drvCob, 0x123 , CO_COB_RTR_NONE, 1);
    retVal = icoTransmitMessage(drvCob, trData, 0u);

    icoCobChangeType(drvCob, CO_COB_TYPE_RECEIVE);
    retVal = icoCobSet(drvCob, 0x234 , CO_COB_RTR, 2);
    retVal = icoTransmitMessage(drvCob, trData, 0u);

    icoCobChangeType(drvCob, CO_COB_TYPE_TRANSMIT);
    retVal = icoCobSet(drvCob, 0x345 + CO_COB_29BIT , CO_COB_RTR_NONE, 3);
    retVal = icoTransmitMessage(drvCob, trData, 0u);

    icoCobChangeType(drvCob, CO_COB_TYPE_RECEIVE);
    retVal = icoCobSet(drvCob, 0x456 + CO_COB_29BIT, CO_COB_RTR, 4);
    retVal = icoTransmitMessage(drvCob, trData, 0u);

    return (retVal);
}
#endif /* DRIVER_TEST */
