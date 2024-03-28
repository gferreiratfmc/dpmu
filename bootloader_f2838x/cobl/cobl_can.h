/* cobl_can
*
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
* \brief CAN driver
*/

#ifndef COBL_CAN_H
#define COBL_CAN_H 1


/* register size - minimum 16bit supported, currently */
/* N.B.:  sizeof(UNSIGNED32) = 2, that is way the register offsets are divided by 2 */

typedef UNSIGNED32 REG_T;

/* Register offsets */
#define CCAN_CNTL       (0x000u/2u)     // CAN Control Register
#define CCAN_STAT       (0x004u/2u)     // Error and Status Registe
#define CCAN_ERR        (0x008u/2u)     // Error Counter Register
#define CCAN_BIT        (0x00Cu/2u)     // Bit Timing Register
#define CCAN_INT        (0x010u/2u)     // Interrupt Register
#define CCAN_TST        (0x014u/2u)     // Test Register
#define CCAN_PERR       (0x018u/2u)     //  CAN Parity Error Code Register


#pragma CHECK_MISRA("-19.7")
/* n..IF number 1,2 */
#define IF_OFFSET(n)        (((((n) - 1u) * 0x20u) / 2u))   // 0x120 - 0x100 == 0x20

#define CCAN_IF_CMDREQ(n)   ((0x100u+IF_OFFSET(n)) / 2u)      //  IF1 Command Register
#define CCAN_IF_CMDMSK(n)   ((0x100u+IF_OFFSET(n)) / 2u)      //  IF1 Command Register

#define CCAN_IF_MASK(n)     ((0x104u+IF_OFFSET(n)) / 2u)      //  IF1 Mask Register
#define CCAN_IF_ARB(n)      ((0x108u+IF_OFFSET(n)) / 2u)      //  IF1 Arbitration Register
#define CCAN_IF_MCTRL(n)    ((0x10cu+IF_OFFSET(n)) / 2u)      //  IF1 Message Control Register
#define CCAN_IF_DA(n)       ((0x110u+IF_OFFSET(n)) / 2u)      //  IF1 Data A Register
#define CCAN_IF_DB(n)       ((0x114u+IF_OFFSET(n)) / 2u)      // IF1 Data B Register

#pragma RESET_MISRA("all")

#define CCAN_TXRQ1      (0x084u/2u)         // CAN Transmission Request Register
#define CCAN_TXRQ2      (0x088u/2u)         // CAN Transmission Request 2_1 Register

#define CCAN_NEWDATA1   (0x098u/2u)         // CAN New Data Register
#define CCAN_NEWDATA2   (0x08Cu/2u)         // CAN New Data 2_1 Register

#define CCAN_MSG1INT    (0x0ACu/2u)         // CAN Interrupt Pending Register
#define CCAN_MSG2INT    (0x0B0u/2u)         // CAN Interrupt Pending 2_1 Register

#define CCAN_MSG1VAL    (0x0C0u/2u)         //  CAN Message Valid Register
#define CCAN_MSG2VAL    (0x0C4u/2u)         //  CAN Message Valid 2_1 Register

/* Register bit definitions */
#define CCAN_CNTL_INIT      ((REG_T)1u << 0)
#define CCAN_CNTL_IE        ((REG_T)1u << 1)
#define CCAN_CNTL_SIE       ((REG_T)1u << 2)
#define CCAN_CNTL_EIE       ((REG_T)1u << 3)
#define CCAN_CNTL_CCE       ((REG_T)1u << 6)
#define CCAN_CNTL_TEST      ((REG_T)1u << 7)

#define CCAN_TST_SILENT      ((REG_T)1u << 3)

#define CCAN_STAT_EPASS     ((REG_T)1u << 5)
#define CCAN_STAT_EWARN     ((REG_T)1u << 6)
#define CCAN_STAT_BOFF      ((REG_T)1u << 7)

#define CCAN_CMDREQ_MN_MASK     0x3Fu
#define CCAN_CMDREQ_BUSY        ((REG_T)1u << 15)
#define CCAN_CMDREQ_MAX_MN      32u

#define CCAN_CMDMSK_DATA_B      ((REG_T)1u << 16)
#define CCAN_CMDMSK_DATA_A      ((REG_T)1u << 17)
#define CCAN_CMDMSK_WR_TXRQST   ((REG_T)1u << 18)
#define CCAN_CMDMSK_CLRINTPND   ((REG_T)1u << 19)
#define CCAN_CMDMSK_CTRL        ((REG_T)1u << 20)
#define CCAN_CMDMSK_ARB         ((REG_T)1u << 21)
#define CCAN_CMDMSK_MASK        ((REG_T)1u << 22)
#define CCAN_CMDMSK_WR          ((REG_T)1u << 23)
#define CCAN_CMDMSK_RD          0ul

#define CCAN_IFMASK_MDIR        ((REG_T)1u << 30)
#define CCAN_IFMASK_MXTD        ((REG_T)1u << 31)

#define CCAN_IFARB_DIR_MSK      ((REG_T)1u << 29)
#define CCAN_IFARB_DIR_TX       ((REG_T)1u << 29)
#define CCAN_IFARB_DIR_RX       0ul
#define CCAN_IFARB_XTD          ((REG_T)1u << 30)
#define CCAN_IFARB_MSGVAL       ((REG_T)1u << 31)

#define CCAN_IFMCTRL_DLC_MASK   0x000Fu
#define CCAN_IFMCTRL_EOB        ((REG_T)1u << 7)
#define CCAN_IFMCTRL_TXRQST     ((REG_T)1u << 8)
#define CCAN_IFMCTRL_RMTEN      ((REG_T)1u << 9)
#define CCAN_IFMCTRL_RXIE       ((REG_T)1u << 10)
#define CCAN_IFMCTRL_TXIE       ((REG_T)1u << 11)
#define CCAN_IFMCTRL_UMASK      ((REG_T)1u << 12)
#define CCAN_IFMCTRL_INPND      ((REG_T)1u << 13)
#define CCAN_IFMCTRL_MSGLST     ((REG_T)1u << 14)
#define CCAN_IFMCTRL_NEWDAT     ((REG_T)1u << 15)


#define CCAN_TX_OBJ 1u
#define CCAN_RX_OBJ 2u

/** \brief general return value: CAN driver State */
typedef enum {
	CAN_OK = 0, /**< OK */
	CAN_EMPTY,  /**< queue/CAN controller is empty (RX) */
	CAN_BUSY,   /**< CAN controller is full/busy (TX) */

	/* some specific errors - socket related errors */
	CAN_ERROR_SOCKET,	/**< socket() error */
	CAN_ERROR_BIND,		/**< bind() error */
	CAN_ERROR_IOCTL,	/**< ioctl() error */
	/* specific errors end ----- */

	CAN_ERROR	/**< general Error */
} CanState_t;

/** \brief identifiere type */
typedef enum { basic, extended } IdType_t;
/** \brief CAN data format type */
typedef enum { data, rtr } IdRtr_t;

/** Bootloader supports 29bit Identifieres - default not used! */
/* #define COBL_29_BIT_ID 1 */

/** \brief Communication identifiere type */
typedef struct {
#ifdef COBL_29_BIT_ID
	U32 id; /**< identifiere */
#else
	U16 id; /**< identifiere */
#endif
	/* not used members deactivated */
/*
	IdType_t bType;
	IdRtr_t bRtr;
*/

} CobId_t;


/**
 * \brief access data on different ways
 *
 * Use only u32Data[] for compile time initialization!
 * Therefore u32Data[] must be the first member (C89)!
 */
typedef union {
	U32 u32Data[2]; /**< 32bit access */
	U16 u16Data[4]; /**< 16bit access */
#ifdef CONFIG_DSP
#else
    U8 u8Data[8];   /**< 8bit access - special handling for DSP req. */
#endif
} CanData_t;

/** \brief CAN message buffer */
typedef struct {
	CobId_t cobId; /**< COB Id / CAN Id */
	U8 dlc; /**< DLC value */
	CanData_t msg; /**< data */
} CanMsg_t;




/* Prototypes */
/*---------------------------------------------------------*/

CanState_t coblCanInit(void);
CanState_t coblCanReceive(CanMsg_t * const pMsg);
CanState_t coblCanTransmit(const CanMsg_t * const pMsg);

void coblCanDisable(void);
void coblCanEnable(void);
void coblCanConfigureFilter(U16 nodeId);
void coblCanListenOnlyMode(void);

/* Debug */
void printMsg(const CanMsg_t * const pMsg);

#endif /* COBL_CAN_H */
