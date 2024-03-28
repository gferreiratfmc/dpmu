/* cobl_canopen.h
 *
 * Copyright (c) 2012-2019 emotas embedded communication GmbH
 *-------------------------------------------------------------------
 * SVN  $Id: cobl_canopen.h 29606 2019-10-23 10:25:58Z hil $
 *
 *
 *-------------------------------------------------------------------
 *
 *
 */

/********************************************************************/
/**
 * \file
 * \brief canopen definitions
 */

#ifndef COBL_CANOPEN
#define COBL_CANOPEN_H 1

#include <cobl_can.h>
/** \brief callback function call */
typedef CoblRet_t (*PtrCallFunction_t)(const CanMsg_t * const);

/** \brief object dictionary definition */
typedef struct
{
	U8 bCompareSecond; /**< check second word, too */
	U8 bCallFunction; /**< use function pointer for calling */
	U8 reserved[2]; /**< 4 byte alignment */
	CanData_t req; /**< Request compare data */
	CanData_t mask; /**< request mask */
	CanData_t resp; /**< response, if no function is calling */
	PtrCallFunction_t ptrCallFunction; /**< callback function */
} CoOd_t;


/* global variables */
extern const CoOd_t coOd[];
extern const U16 odTableSize;

#define COBL_HB_TIME 2000 /* 2000ms */
#define SDO_ABORT_GENERAL 	0x08000000ul
#define SDO_ABORT_LARGE		0x06070012ul
#define SDO_ABORT_TOGGLE	0x05030000ul
#define SDO_ABORT_FLASH		0x06060000ul
#define SDO_ABORT_STATE		0x08000022ul

#define SDO_TOGGLE_BIT 		0x10
#define SDO_LAST_BIT		0x01

#define SDO_CCS_MASK		0xE0
#define SDO_CCS_DL_SEGMENT 	0x00
#define SDO_CCS_UL_SEGMENT	0x60

#define SDO_N_MASK			0x0E

#define NMT_RESET_NODE		129
#define NMT_RESET_COMM		130

/* Prototypes */
/* ---------------------------------------------------------------*/
CoblRet_t canopenInit(void);
CoblRet_t canopenCyclic(void);

/* only for object dictionary function pointer */
CoblRet_t canopenSdoSegmentedFirst(const CanMsg_t * const pMsg);
CoblRet_t canopenSdoSegReadFirst(CoblObjInfo_t * const pObjInfo);

CoblRet_t canopenSendEmcy(U16 errorcode, U8 add0, U16 add1, U16 add2);
#ifdef COBL_DEBUG
void printTable(void);
#endif


#endif /* COBL_CANOPEN_H */
