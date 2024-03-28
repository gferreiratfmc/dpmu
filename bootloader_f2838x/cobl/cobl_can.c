/* cobl_can
*
* Copyright (c) 2012-2020 emotas embedded communication GmbH
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
* \brief can driver
*
* very hardware depend
*/

/* standard includes
--------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



/* header of project specific types
---------------------------------------------------------------------------*/
#include <user_config.h>

#include <cobl_type.h>
#include <cobl_can.h>
#include <cobl_timer.h>
//#include <cobl_debug.h>

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
static void coblSetBitRate(void);
static void coblRxBufferInit(U32 mnum, U32 id);

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
/** CAN Controller address */
//static volatile U32 *  pCan = (void*)0x0004A000U; /* CANB */

static volatile U32 *  pCan = (void*)0x00048000U; /* CANB */

#ifdef CFG_CAN_BACKDOOR
static U8 listenOnlyMode = 0u;
#endif /*CFG_CAN_BACKDOOR*/

/***************************************************************************/
/**
* \brief can initialization
*
* \param
*       nothing
* \retval
*       CAN_OK OK
*/

CanState_t coblCanInit(void)
{
U8 mn;

	#ifdef COBL_DEBUG
		printf("canInit\n");
	#endif
	/* reset listenOnly flag */
#ifdef CFG_CAN_BACKDOOR
	listenOnlyMode = 0u;
#endif /* CFG_CAN_BACKDOOR */

	/* initialize CAN controller, setup timing, pin description, CAN mode ...*/
	pCan[CCAN_CNTL] = CCAN_CNTL_INIT | CCAN_CNTL_CCE;

	coblSetBitRate();

	for ( mn = 1; mn <= 32; mn++) {
		pCan[CCAN_IF_ARB(1u)] = 0u; /* Msg invalid */

        /* write to nm message object */
        pCan[CCAN_IF_CMDMSK(1u)] = (CCAN_CMDMSK_WR | CCAN_CMDMSK_ARB | mn);

        while ( (pCan[CCAN_IF_CMDMSK(1u)] & CCAN_CMDREQ_BUSY) != 0u) {};
	}

	/* create RX Buffer */
	coblRxBufferInit(CCAN_RX_OBJ, 0);

	/* Bus on */
	coblCanEnable();

	return CAN_OK;
}

/***************************************************************************/
/**
* \brief coblSetBitRate - set bitrate
*
* http://www.can-wiki.info/bittiming/tq.html
*
* The implemented values are for 36Mhz CAN clock.
*/
static void coblSetBitRate(void)
{
U16 bitrate = CFG_BITRATE();
U32 pre = 0u;
U32 seg1 = 13u;
U32 seg2 = 2;
U32 brpe = 0;

	coblCanDisable();

	switch(bitrate) {
	case 125:
	    /* 100 = 64+36 */
		pre = 36u;
		brpe = 1;
		break;
	case 250:
		pre = 50u;
		break;
	case 500:
		pre = 25u;
		break;
	case 1000:
		pre = 20u;
		seg1 = 8u;
		seg2 = 1u;
		break;
	default:
		return;
	}

    /* set configuration bit */
    pCan[CCAN_CNTL] = CCAN_CNTL_INIT | CCAN_CNTL_CCE;

	/* setup timing registers for the given Bitrate */
    pre -= 1ul; /* now register value! */
    pCan[CCAN_BIT] =  ((seg2 - 1ul) << 12)
                    | ((seg1 - 1ul) << 8)
                    | ((pre & 0x3Ful))
                    | (brpe << 16 )
                    ;  /* SJW = 1tq */

}

#ifdef CFG_CAN_BACKDOOR
/***************************************************************************/
/**
* \brief disable the CAN controller
*
*/
void coblCanListenOnlyMode(void)
{
	listenOnlyMode = 1u;
}
#endif /*CFG_CAN_BACKDOOR*/

/***************************************************************************/
/**
* \brief disable the CAN controller
*
*/
void coblCanDisable(void)
{
	/* Bus off */
	pCan[CCAN_CNTL] = 1;
}

/***************************************************************************/
/**
* \brief enable the CAN controller
*/
void coblCanEnable(void)
{
    /* Bus on */
    pCan[CCAN_CNTL] = 0;

#ifdef CFG_CAN_BACKDOOR
    if (listenOnlyMode == 1u) {
        /* enable TEST mode */
        pCan[CCAN_CNTL] = CCAN_CNTL_TEST;
        /* Set silent bit */
        pCan[CCAN_TST] |= CCAN_TST_SILENT;
    }
#endif /* CFG_CAN_BACKDOOR */


	/* wait for Buson */
	timerWait100ms();
}


/***************************************************************************/
/**
* \brief configure CANopen Filter Set
*
* ID 0x000 - NMT
* ID 0x600 + nodeId - SDO Request
*
* In case of NodeId == 0 -> receive all messages
*/

void coblCanConfigureFilter(
		U16 nodeId
	)
{
    //disable filter
	coblCanDisable();


#ifdef CFG_CAN_BACKDOOR
	if (nodeId == 0u) {
	    // enable reception for everything (11bit)
	    pCan[CCAN_IF_CMDMSK(1u)] = CCAN_CMDMSK_WR | CCAN_CMDMSK_ARB
	                        | CCAN_CMDMSK_CTRL | CCAN_CMDMSK_MASK;

        pCan[CCAN_IF_MASK(1u)] = 0x0000u; /* receive all */
                                           /* all IDs, ext. RTR */
        /* only base ids (IDbit 28..18) /Data frames */
        pCan[CCAN_IF_ARB(1u)] = 0x0000u /* all IDs - value irrelevant */
                            | CCAN_IFARB_DIR_RX /* data frame */
                            | CCAN_IFARB_MSGVAL;
                            /* + base identifier only */

        /* enable acceptance filter */
        pCan[CCAN_IF_MCTRL(1u)] = CCAN_IFMCTRL_EOB
                            | CCAN_IFMCTRL_RXIE
                            | CCAN_IFMCTRL_UMASK;

        pCan[CCAN_IF_CMDMSK(1u)] = CCAN_CMDMSK_WR | CCAN_CMDMSK_ARB
                                | CCAN_CMDMSK_CTRL | CCAN_CMDMSK_MASK | CCAN_RX_OBJ;
        while ( (pCan[CCAN_IF_CMDREQ(1u)] & CCAN_CMDREQ_BUSY) != 0u)  {};
	} else
#endif /* CFG_CAN_BACKDOOR */
	{
	    //set new filter
        coblRxBufferInit(CCAN_RX_OBJ, 0);
        coblRxBufferInit(CCAN_RX_OBJ + 1, 0x600 + nodeId);
	}

	//enable filter
	coblCanEnable();
}


/***************************************************************************/
/**
* \brief receive a message, better get a received message
*
* \retval
*       CAN_OK - message received/saved
* \retval
*       CAN_EMPTY - no message received
*/

CanState_t coblCanReceive(
		CanMsg_t * const pMsg		/**< pointer to save message */
)
{
U32 new;
U16 mnum;
U32 id;
U8 len;
U32 data;
U32 intid;

	/* check Busoff state -> go Buson */
	if ((pCan[CCAN_CNTL] & CCAN_CNTL_INIT) != 0) {
		pCan[CCAN_CNTL] = 0;
	}


	intid = pCan[CCAN_INT];

    if (intid == 0x8000ul) {

    } else
    if (intid == 0ul) {
        /* software interrupt */
        mnum = 0;
    } else
    if ((intid >= 1ul) && (intid <= 32ul)) {
        mnum = intid;
    } else
    {
        /* not valid */
    }

    if (mnum == 0ul) {
        return CAN_EMPTY; /* Problem? */
    }

    pCan[CCAN_IF_CMDMSK(1u)] = CCAN_CMDMSK_RD | CCAN_CMDMSK_ARB
                            | CCAN_CMDMSK_CTRL | CCAN_CMDMSK_DATA_A
                            | CCAN_CMDMSK_DATA_B
                            | CCAN_CMDMSK_CLRINTPND
                            | mnum;
    while ( (pCan[CCAN_IF_CMDREQ(1u)] & CCAN_CMDREQ_BUSY) != 0ul) {};


    if ((pCan[CCAN_IF_MCTRL(1u)] & CCAN_IFMCTRL_NEWDAT) == 0u)
    {
        /* no new data ?? */
        return CAN_EMPTY;
    }

    /* now check for receive messages */
    new = pCan[CCAN_NEWDATA2] << 16
            | pCan[CCAN_NEWDATA1];
    if ((new & (1ul << (mnum - 1))) == 0) {
          /* no RX dataframe */
         // return CAN_EMPTY;
    }


    id = pCan[CCAN_IF_ARB(1u)];


    if ((id & CCAN_IFARB_XTD) != 0u) {
        /* 29bit ID */
        return CAN_EMPTY;
    }

    /* only base frames supported */
    id = (id >> 18) & 0x7FFul;

	/* save message at buffer */
	pMsg->cobId.id = id;

	len = (U8)pCan[CCAN_IF_MCTRL(1)] & 0x0F;
	if (len > 8) {
		len = 8;
	}
	pMsg->dlc = len;

	data = (U32)pCan[CCAN_IF_DA(1u)];
	pMsg->msg.u32Data[0] =   data;

	data = (U32)pCan[CCAN_IF_DB(1u)];
    pMsg->msg.u32Data[1] =   data;


#ifdef COBL_DEBUG
	if ((pCan[CCAN_IF_MCTRL(1)] & CCAN_IFMCTRL_MSGLST) != 0) {
		cobl_puts("RX Overflow\n");

		pCan[CCAN_IF_CMDMSK(1)] = CCAN_CMDMSK_WR | CCAN_CMDMSK_CTRL;
		pCan[CCAN_IF_MCTRL(1)] &= ~CCAN_IFMCTRL_MSGLST;

		pCan[CCAN_IF_CMDREQ(1)] = (U32)mnum;
		while ( (pCan[CCAN_IF_CMDREQ(1)] & CCAN_CMDREQ_BUSY) != 0) {};
	}
	printMsg(pMsg);
#endif

	return CAN_OK;
}

/***************************************************************************/
/**
* \brief transmit a message
*
* \todo
* This function block as long as the former message was not send.
* A buffer mechanism could be implemented. But this require more resources.
*
* \retval
*       CAN_OK - message transmitted
* \retval
*       CAN_BUSY - no message transmitted, CAN controller busy
*/

CanState_t coblCanTransmit(
	const CanMsg_t * const pMsg
)
{
const U8 mnum = CCAN_TX_OBJ;

#ifdef COBL_DEBUG
	printf("canTransmit \n");
	printMsg(pMsg);
#endif


	/* busy check */

#if 0
	txreq = (UNSIGNED32)pCan[CCAN_TXRQ2] << 16
			| (UNSIGNED32)pCan[CCAN_TXRQ1];


	if ((txreq != 0) {
		return CAN_BUSY;
	}
#else
	/* currently no buffer mechanism -> blocking  (Buffer 1..16) */
//	while (pCan[CCAN_TXRQ1] != 0) { }
#endif


	/* set invalid */
	pCan[CCAN_IF_ARB(1)] =  0; /* not valid */
    pCan[CCAN_IF_CMDMSK(1)] = CCAN_CMDMSK_WR | CCAN_CMDMSK_ARB;
  	pCan[CCAN_IF_CMDREQ(1)] = mnum;
	while ( (pCan[CCAN_IF_CMDREQ(1)] & CCAN_CMDREQ_BUSY) != 0) {};

	/* configure new message */
	pCan[CCAN_IF_CMDMSK(1)] = CCAN_CMDMSK_WR | CCAN_CMDMSK_ARB
						| CCAN_CMDMSK_CTRL | CCAN_CMDMSK_DATA_A
						| CCAN_CMDMSK_DATA_B | CCAN_CMDMSK_WR_TXRQST;



	/* write message to the CAN controller */
	/* only base ids (IDbit 28..18) /Data frames */
	pCan[CCAN_IF_ARB(1)] = ((U32)pMsg->cobId.id << 18)
						| CCAN_IFARB_DIR_TX
						| CCAN_IFARB_MSGVAL;


	pCan[CCAN_IF_DA(1u)] = (U32)pMsg->msg.u32Data[0];
    pCan[CCAN_IF_DB(1u)] = (U32)pMsg->msg.u32Data[1];


	/* set DLC and CAN Controller flags */
	pCan[CCAN_IF_MCTRL(1)] = (U32)pMsg->dlc
						| CCAN_IFMCTRL_EOB
						| CCAN_IFMCTRL_TXRQST
						| CCAN_IFMCTRL_NEWDAT;

	/* transmit it */
	pCan[CCAN_IF_CMDMSK(1u)] = CCAN_CMDMSK_WR | CCAN_CMDMSK_ARB
	                        | CCAN_CMDMSK_CTRL | CCAN_CMDMSK_DATA_A
	                        | CCAN_CMDMSK_DATA_B | CCAN_CMDMSK_WR_TXRQST
	                        | mnum;

	/* wait some clocks */
	while ( (pCan[CCAN_IF_CMDREQ(1)] & CCAN_CMDREQ_BUSY) != 0) {};

	return CAN_OK;
}


/***************************************************************************/
/**
* \brief coblRxBufferInit - init receive buffer of the CAN controller
*
* This function initialize the Receive Message box of the CAN controller
* for Basic CAN. This means, the Receive Filter are configured to receive
* all messages.
*
* The filter ist set to receive only base data frames.
*/
static void coblRxBufferInit(
		U32 mnum, /**< C_CAN Message buffer number */
		U32 id /**< CAN ID */

	)
{

    pCan[CCAN_IF_CMDMSK(1u)] = CCAN_CMDMSK_WR | CCAN_CMDMSK_ARB
                        | CCAN_CMDMSK_CTRL | CCAN_CMDMSK_MASK;


    /* only base ids (IDbit 28..18) /Data & RTR frames */
    pCan[CCAN_IF_MASK(1u)] = (0x7fful << 18) /* masked ID bits */
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
                            | CCAN_CMDMSK_CTRL | CCAN_CMDMSK_MASK | mnum;
    while ( (pCan[CCAN_IF_CMDREQ(1u)] & CCAN_CMDREQ_BUSY) != 0u) {};
}



#ifdef COBL_DEBUG
/***************************************************************************/
/**
 * Debug
 * print the message information to stdout
 */
void printMsg(
		const CanMsg_t * const pMsg
		)
{
	cobl_puts("Message ");
	cobl_ptr(pMsg);
	cobl_puts("\n");

	cobl_puts("> 0x");
	cobl_hex16(pMsg->cobId.id);
	cobl_puts(" ");
	cobl_hex8(pMsg->dlc);
	cobl_puts(" ");
	cobl_hex32(pMsg->msg.u32Data[0]);
	cobl_puts(" ");
	cobl_hex32(pMsg->msg.u32Data[1]);
	cobl_puts("\n");
}
#endif
