/*
* cobl_crc.h
*
* Copyright (c) 2012 emtas GmbH
*-------------------------------------------------------------------
* SVN  $Date: 2012-04-30 16:43:25 +0200 (Mo, 30. Apr 2012) $
* SVN  $Rev: 413 $
* SVN  $Author: $
*
*
*-------------------------------------------------------------------
* Changelog:
*
*
*/

/********************************************************************/
/**
* \file
* \brief cobl_crc.h
*
*/



#ifndef COBL_CRC_H
#define COBL_CRC_H 1

/** startvalue like sdo block transfer */
#define CRC_START_VALUE	0

void crcInitCalculation(void);
U16 crcCalculation( const U8 * pBuffer, U16 u16StartValue, U32 u32Count);


#endif /* COBL_CRC_H */
