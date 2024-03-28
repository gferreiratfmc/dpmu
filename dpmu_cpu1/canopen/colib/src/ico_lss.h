/*
* ico_lss.h - contains internal defines for LSS
*
* Copyright (c) 2012-2022 emotas embedded communication GmbH
*
*-------------------------------------------------------------------
* $Id: ico_lss.h 41921 2022-09-01 10:39:03Z boe $

*-------------------------------------------------------------------
*
*
*/

/**
* \file
* \brief dataty type header
*/

#ifndef ICO_LSS_H
#define ICO_LSS_H 1


/* datatypes */

/* LSS command specifier */
#define LSS_CS_SWITCH_GLOBAL				0x04u
#define LSS_CS_NODE_ID						0x11u
#define LSS_CS_BITRATE                      0x13u
#define LSS_CS_ACTIVATE_BITRATE             0x15u
#define LSS_CS_STORE						0x17u
#define LSS_CS_SWITCH_SEL_VENDOR			0x40u
#define LSS_CS_SWITCH_SEL_PRODUCT			0x41u
#define LSS_CS_SWITCH_SEL_REVISION			0x42u
#define LSS_CS_SWITCH_SEL_SERIAL			0x43u
#define LSS_CS_SWITCH_SEL_ANSWER			0x44u
#define LSS_CS_IDENT_VENDOR					0x46u
#define LSS_CS_IDENT_PRODUCT				0x47u
#define LSS_CS_IDENT_REVISION_LOW			0x48u
#define LSS_CS_IDENT_REVISION_HIGH			0x49u
#define LSS_CS_IDENT_SERIAL_LOW				0x4au
#define LSS_CS_IDENT_SERIAL_HIGH			0x4bu
#define LSS_CS_NON_CONFIG_REMOTE_SLAVE		0x4cu
#define LSS_CS_IDENTITY_SLAVE				0x4fu
#define LSS_CS_NON_CONFIG_SLAVE				0x50u
#define LSS_CS_FAST_SCAN					0x51u
#define LSS_CS_INQUIRE_VENDOR				0x5au
#define LSS_CS_INQUIRE_PRODUCT				0x5bu
#define LSS_CS_INQUIRE_REVISION				0x5cu
#define LSS_CS_INQUIRE_SERIAL				0x5du
#define LSS_CS_INQUIRE_NODEID				0x5eu
#define LSS_CS_INQUIRE_ADDRESS				0x5fu

#define LSS_FD_CS_SWITCH_SEL_NOT			0x64u  /* command specifier for switch state selective unconfigured */
#define LSS_FD_CS_SWITCH_SEL_ALL			0x65u  /* command specifier for switch state selective all devices */

#define LLS_FD_CS_SWITCH_SEL_ANSWER			0x77   /* workaround specifier for answer for switch state selective */

/* LSS can ids */
#define LSS_CAN_ID_SLAVE			0x7e4u
#define LSS_CAN_ID_MASTER			0x7e5u
#define LSS_FD_CAN_ID_SWITCH_SEL	0x7d0u

/* LSS FD can id type */
#define LSS_CAN_ID_TYPE_CC	0u	/* classical CAN ID */
#define LSS_CAN_ID_TYPE_FD	1u	/* can fd CAN ID */
#define LSS_CAN_ID_TYPE_CC_MASK	2u	/* cand fd CAN ID with mask for special LSS services */

/* function prototypes */
void	icoLssResetAppl(void);
void	icoLssReset(UNSIGNED8 *pNodeId);
void	icoLssHandler(CO_REC_DATA_T *pRecData);
void	icoLssMasterHandler(CO_REC_DATA_T *pRecData);
void	icoLssResetNodeId(void);
void	icoLssMasterReset(void);
void	icoLssVarInit(void);
void	icoLssMasterVarInit(void);
RET_T	icoLssFastScanCmd(UNSIGNED32 lIdNumber, UNSIGNED8 lBitCheck,
	UNSIGNED8 lLssSub, UNSIGNED8 lLssNext, UNSIGNED16 lTimeOutVal);


#endif /* ICO_LSS_H */
