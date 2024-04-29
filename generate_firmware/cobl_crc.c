/*
* cobl_crc.c
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
* \brief cobl_crc.c
*
* CRC Polynom - like CANopen Block transfer (CCITT, LSB first)
*
* The polynomial shall be x^16 + x^12 + x^5 + 1.
* The calculation shall be made with an initial value of 0.
*
*/



/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <stdio.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <cobl_type.h>
#include <cobl_crc.h>


#define CRC_ROM_TABLE 1
/*
#define CRC_RAM_TABLE 1
#define CRC_WITHOUT_TABLE 1
#define CRC_WITHOUT_TABLE_SMALL 1
*/

/* constant definitions
---------------------------------------------------------------------------*/
#define CCITT_MASK 0x1021

#ifdef CRC_ROM_TABLE
/** crc table in ROM - created by printTable() - CCITT_MASK configured */
const U16 crcTable[256] = {
0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0 };
#endif

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
#ifdef INTERNAL_PRINT_TABLE
static void printTable(void);
#endif

#ifdef CRC_WITHOUT_TABLE_SMALL
static U16 crcWithoutTableSmall(const U8 * pBuffer, U16 u16StartValue, U32 u32Count);
#endif

#ifdef CRC_WITHOUT_TABLE
static U16 crcWithoutTable(const U8 * pBuffer, U16 u16StartValue, U32 u32Count);
#endif

#ifdef CRC_RAM_TABLE
static void crcInitTable(void);
#endif

#if defined(CRC_ROM_TABLE) || defined(CRC_RAM_TABLE)
static U16 crcWithTable( const U8 * pBuffer, U16 u16StartValue, U32 u32Count);
#endif


/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/
#ifdef CRC_RAM_TABLE
static U16 crcTable[256];
#endif

/********************************************************************/
/**
* initialize the CRC calculation
*/
void crcInitCalculation(void)
{
#ifdef CRC_RAM_TABLE
	crcInitTable();
#endif
}

/********************************************************************/
/**
* CRC calculation
*
* \returns
* CRC sum of pBuffer
*/
U16 crcCalculation(
		const U8 * pBuffer, /**< buffer to calculate */
		U16 u16StartValue, /**< start value for calculation */
		U32 u32Count /**< number of bytes */
		)
{
U16 retVal;

#if defined(CRC_RAM_TABLE) || defined(CRC_ROM_TABLE)
	retVal = crcWithTable(pBuffer, u16StartValue, u32Count);
#endif

#ifdef CRC_WITHOUT_TABLE_SMALL
	retVal = crcWithoutTableSmall(pBuffer, u16StartValue, u32Count);
#endif

#ifdef CRC_WITHOUT_TABLE
	retVal = crcWithoutTable(pBuffer, u16StartValue, u32Count);
#endif

	return retVal;
}





#ifdef CRC_WITHOUT_TABLE_SMALL
/********************************************************************/
/**
* fix, but small code for CCITT
* http://www.eagleairaust.com.au/code/crc16.htm
*/
static U16 crcWithoutTableSmall(
		const U8 * pBuffer,
		U16 u16StartValue,
		U32 u32Count /**< number of bytes */
		)
{
U32 lcount = u32Count;
U8 u8Val;
U16 u16Crc = u16StartValue;

	while (lcount--)
	{
		u8Val = *pBuffer++;

		u16Crc = (U8)(u16Crc >> 8) | (u16Crc << 8);
		u16Crc ^= u8Val;
		u16Crc ^= (U8)(u16Crc & 0xFF) >> 4;
		u16Crc ^= (u16Crc << 8) << 4; /* two shifts 4 and 8 bit for better asm code */
		u16Crc ^= ((u16Crc & 0xFF) << 4) << 1;
	}

    /* printf(" crc: 0x%04x\n",(int)u16Crc); */

    return (u16Crc);
}
#endif

#ifdef CRC_WITHOUT_TABLE
/********************************************************************/
/**
* crc calculation without a table
* Polynom: CCITT_MASK
*
*/
static U16 crcWithoutTable(
		const U8 * pBuffer,
		U16 u16StartValue,
		U32 u32Count /**< number of bytes */
		)
{

U16 carryBit;
U32 lcount = u32Count;
U8 u8Val;
U16 u16Crc = u16StartValue;
U8 i;

	while (lcount--)
	{
		u8Val = *pBuffer++;

		u16Crc ^= ((U16)u8Val << 8);

		for (i = 0; i < 8; i++) {

			carryBit = (u16Crc & 0x8000) ? 1 : 0;
			u16Crc <<= 1;

			if (carryBit != 0) {
				u16Crc ^= CCITT_MASK;
			}
		}
	}

    /* printf(" crc: 0x%04x\n",(int)u16Crc); */

    return (u16Crc);
}
#endif

#ifdef CRC_RAM_TABLE
/********************************************************************/
/**
* initialize a RAM table for crc calculation
* Polynom: CCITT_MASK
*
*/
static void crcInitTable(void)
{
U16 b;
U16 v;
U8 i;

	for (b = 0; b < 256; b++)
	{
		v = b << 8;
		for (i = 0; i < 8; i++) {
			if ((v & 0x8000) != 0) {
				v = (v << 1) ^ CCITT_MASK;
			} else {
				v = v << 1;
			}
		}

		crcTable[b] = v;
	}
}
#endif

#if defined(CRC_ROM_TABLE) || defined(CRC_RAM_TABLE)
/********************************************************************/
/**
* crc calculation with table
* Polynom: CCITT_MASK
*
*/
static U16 crcWithTable(
		const U8 * pBuffer,
		U16 u16StartValue,
		U32 u32Count /**< number of bytes */
		)
{
	
	U32 count = 0;
U32 lcount = u32Count;
U16 u16Crc = u16StartValue;
U8 u8Val;

	while(lcount --) {
		u8Val = *pBuffer++;

		u16Crc = (u16Crc << 8) ^ crcTable[ (U8)(u16Crc >> 8) ^ u8Val];

		count++;
	}

    printf("CRC16 processed Bytes=%u\r\n", count);
	printf(" crc: 0x%04xu\n",u16Crc);
	
    return u16Crc;
}
#endif


/* #define INTERNAL_TEST_CRC 1*/
#ifdef INTERNAL_TEST_CRC
/********************************************************************/
/**
 * check the CRC definition in CiA 301
 * (chapter 7.2.4.3.16 - crc for sdo block transfer)
 */
void testCrc(void)
{
U8 buffer[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
U16 lRetval;

#ifdef CRC_WITHOUT_TABLE
	lRetval = crcWithoutTable(&buffer[0], 0, sizeof(buffer));
	if(lRetval == 0x31C3) {
		printf("correct for CiA301\n");
	} else {
		printf("incorrect\n");
	}
#endif

#ifdef CRC_WITHOUT_TABLE_SMALL
	lRetval = crcWithoutTableSmall(&buffer[0], 0, sizeof(buffer));
	if(lRetval == 0x31C3) {
		printf("correct for CiA301\n");
	} else {
		printf("incorrect\n");
	}
#endif

#ifdef CRC_RAM_TABLE
	crcInitTable();
#endif
#if defined(CRC_RAM_TABLE) || defined(CRC_ROM_TABLE)
	lRetval = crcWithTable(&buffer[0], 0, sizeof(buffer));
	if(lRetval == 0x31C3) {
		printf("correct for CiA301\n");
	} else {
		printf("incorrect\n");
	}
#endif

	/* printTable(); */

}
#endif


#ifdef INTERNAL_PRINT_TABLE
#  ifdef CRC_RAM_TABLE
#  else
#    error "CRC_RAM_TABLE required!"
#  endif
/********************************************************************/
/**
 * print the RAM CRC table to define it in ROM
 *
 */
static void printTable(void)
{
int i;

	crcInitTable();

	printf("const U16 crcTable[256] = {");
	for (i = 0; i < 256; i++) {
		if ((i % 8) == 0) {
			printf("\n");
		}
		printf("0x%04x, ", crcTable[i]);
	}
	printf("};");

}
#endif

