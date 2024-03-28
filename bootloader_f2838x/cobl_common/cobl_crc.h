/*
* cobl_crc.h
*
* Copyright (c) 2012-2019 emotas embedded communication GmbH
*-------------------------------------------------------------------
* SVN  $Id: cobl_crc.h 31977 2020-05-07 09:35:53Z ro $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \file
* \brief cobl_crc.h - CRC calculation
*
*/

#ifndef COBL_CRC_H
#define COBL_CRC_H 1

/* constant definitions
---------------------------------------------------------------------------*/
/** startvalue like sdo block transfer */
#ifndef CRC_START_VALUE	
#  define CRC_START_VALUE	0
#endif

#define CCITT_MASK 0x1021

/* external Prototypes */
/* ------------------------------------------------------------------*/
#ifdef CRC_USER
extern void crcUserInit(void);
extern U16 crcUser( const U8 * pBuffer, U16 u16StartValue, U32 u32Count);
#endif


/* Prototypes */
/* ------------------------------------------------------------------*/
void crcInitCalculation(void);
U16 crcCalculation( const U8 * pBuffer, U16 u16StartValue, U32 u32Count);

#endif /* COBL_CRC_H */
