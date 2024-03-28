/*
* codrv_dcan.h
*
* Copyright (c) 2018 emotas embedded communication GmbH
*-------------------------------------------------------------------
* SVN  $Id: codrv_dcan.h 29114 2019-08-30 15:38:11Z phi $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \file
* \brief dcan from Bosch 
*
* Bosch D_CAN driver implementation -
* Adapted for TI TMS320F28379D
*
* The base of this driver packaged was C_CAN.
* That is why many names start with CCAN.
*/

#ifndef CODRV_DCAN_H
#define CODRV_DCAN_H 1


/* CAN base registers for CAN  A and CAN B */
#define CAN_BASE_ADDRESS_CAN_A 0x00048000U
#define CAN_BASE_ADDRESS_CAN_B 0x0004A000U

/* reconfigure if CAN B shall be used */
#ifndef CAN_BASE_ADDRESS
# define CAN_BASE_ADDRESS CAN_BASE_ADDRESS_CAN_A
#endif /* CAN_BASE_ADDRESS */

/* number of message objects */
#define CCAN_MSGOBJ_NUM 32u

#include "co_datatype.h"

/* register size - minimum 16bit supported, currently */
/* N.B.:  sizeof(UNSIGNED32) = 2, that is way the register offsets are divided by 2 */
typedef UNSIGNED32 REG_T;

/* Register offsets */
#define CCAN_CNTL		(0x000u/2u)     // CAN Control Register
#define CCAN_STAT		(0x004u/2u)     // Error and Status Registe
#define CCAN_ERR		(0x008u/2u)     // Error Counter Register
#define CCAN_BIT		(0x00Cu/2u)     // Bit Timing Register
#define CCAN_INT		(0x010u/2u)     // Interrupt Register
#define CCAN_TST		(0x014u/2u)     // Test Register
#define CCAN_PERR		(0x018u/2u)     //  CAN Parity Error Code Register

#define CCAN_RAM_INIT	(0x040u/2u)     //  RAM Init

#pragma CHECK_MISRA("-19.7")
/* n..IF number 1,2 */
#define IF_OFFSET(n)		(((((n) - 1u) * 0x20u)))   // 0x120 - 0x100 == 0x20

#define CCAN_IF_CMDREQ(n)	((0x100u+IF_OFFSET(n)) / 2u)      //  IF1 Command Register
#define CCAN_IF_CMDMSK(n)   ((0x100u+IF_OFFSET(n)) / 2u)      //  IF1 Command Register

#define CCAN_IF_MASK(n)	    ((0x104u+IF_OFFSET(n)) / 2u)      //  IF1 Mask Register
#define CCAN_IF_ARB(n)		((0x108u+IF_OFFSET(n)) / 2u)      //  IF1 Arbitration Register
#define CCAN_IF_MCTRL(n)	((0x10cu+IF_OFFSET(n)) / 2u)      //  IF1 Message Control Register
#define CCAN_IF_DA(n)		((0x110u+IF_OFFSET(n)) / 2u)      //  IF1 Data A Register
#define CCAN_IF_DB(n)		((0x114u+IF_OFFSET(n)) / 2u)      // IF1 Data B Register

#pragma RESET_MISRA("all")

#define CCAN_TXRQ21		(0x088u/2u)         // CAN Transmission Request Register


#define CCAN_NEWDATA21	(0x09Cu/2u)         // CAN New Data Register



#define CCAN_MSG21INT	(0x0B0u/2u)         // CAN Interrupt Pending 2_1 Register


#define CCAN_MSG21VAL	(0x0C4u/2u)         //  CAN Message Valid 2_1 Register

/* Register bit definitions */
#define	CCAN_CNTL_INIT 		((REG_T)1u << 0)
#define	CCAN_CNTL_IE 		((REG_T)1u << 1)
#define	CCAN_CNTL_SIE 		((REG_T)1u << 2)
#define	CCAN_CNTL_EIE 		((REG_T)1u << 3)
#define CCAN_CNTL_CCE  		((REG_T)1u << 6)
#define CCAN_CNTL_TEST  	((REG_T)1u << 7)
#define CCAN_CNTL_ABO  		((REG_T)1u << 9)
#define CCAN_CNTL_SWR	  	((REG_T)1u << 15)

#define CCAN_STAT_EPASS		((REG_T)1u << 5)
#define CCAN_STAT_EWARN		((REG_T)1u << 6)
#define CCAN_STAT_BOFF		((REG_T)1u << 7)

#define CCAN_RAM_INIT_START		((REG_T)1u << 4)
#define CCAN_RAM_INIT_DONE		((REG_T)1u << 5)

/* max 32 message objects */
#define	CCAN_CMDREQ_MN_MASK 	0x3Fu
#define	CCAN_CMDREQ_BUSY		((REG_T)1u << 15)
#define CCAN_CMDREQ_MAX_MN		32u

#define	CCAN_CMDMSK_DATA_B 		((REG_T)1u << 16)
#define	CCAN_CMDMSK_DATA_A 		((REG_T)1u << 17)
#define	CCAN_CMDMSK_WR_TXRQST 	((REG_T)1u << 18)
#define	CCAN_CMDMSK_RD_CLRNEWDAT 	((REG_T)1u << 18)
#define	CCAN_CMDMSK_CLRINTPND 	((REG_T)1u << 19)
#define	CCAN_CMDMSK_CTRL 		((REG_T)1u << 20)
#define	CCAN_CMDMSK_ARB 		((REG_T)1u << 21)
#define	CCAN_CMDMSK_MASK 		((REG_T)1u << 22)
#define	CCAN_CMDMSK_WR 			((REG_T)1u << 23)
#define	CCAN_CMDMSK_RD 			0ul

#define	CCAN_IFMASK_MDIR 		((REG_T)1u << 30)
#define	CCAN_IFMASK_MXTD 		((REG_T)1u << 31)

#define	CCAN_IFARB_DIR_MSK		((REG_T)1u << 29)
#define	CCAN_IFARB_DIR_TX 		((REG_T)1u << 29)
#define	CCAN_IFARB_DIR_RX 		0ul
#define	CCAN_IFARB_XTD 		    ((REG_T)1u << 30)
#define	CCAN_IFARB_MSGVAL 		((REG_T)1u << 31)

#define	CCAN_IFMCTRL_DLC_MASK 	0x000Fu
#define	CCAN_IFMCTRL_EOB 		((REG_T)1u << 7)
#define	CCAN_IFMCTRL_TXRQST 	((REG_T)1u << 8)
#define	CCAN_IFMCTRL_RMTEN 		((REG_T)1u << 9)
#define	CCAN_IFMCTRL_RXIE 		((REG_T)1u << 10)
#define	CCAN_IFMCTRL_TXIE 		((REG_T)1u << 11)
#define	CCAN_IFMCTRL_UMASK 		((REG_T)1u << 12)
#define	CCAN_IFMCTRL_INPND 		((REG_T)1u << 13)
#define	CCAN_IFMCTRL_MSGLST 	((REG_T)1u << 14)
#define	CCAN_IFMCTRL_NEWDAT 	((REG_T)1u << 15)




/* prototypes - not in canopen stack headers */
void codrvCanInterrupt(void);

#endif
