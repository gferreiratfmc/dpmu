/*
* cobl_application.h
*
* Copyright (c) 2012-2019 emotas embedded communication GmbH
*-------------------------------------------------------------------
* SVN  $Id: cobl_application.h 29606 2019-10-23 10:25:58Z hil $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \file
* \brief cobl_application.h - application relatetd parts
*
*/



#ifndef COBL_APPLICATION_H
#define COBL_APPLICATION_H 1

/* constant definitions
---------------------------------------------------------------------------*/
/* data type definition
---------------------------------------------------------------------------*/

/** \brief application header */
typedef struct {
	FlashAddr_t applSize; 	/**< filled application flash size */
	U16 crcSum;				/**< crc sum of application flash without config block */
	U16 reserved1;
	U32 od1018Vendor;		/**< application - 0x1018:1 */
	U32 od1018Product;		/**< application - 0x1018:2 */
	U32 od1018Version;		/**< application - 0x1018:3 */
	U32 swVersion;			/**< software version - company specific, possible different from 0x1018:3 */
} ConfigBlock_t;

/* prototypes 
*---------------------------------------------------------------------*/
FlashAddr_t applFlashStart(U8 nbr);
FlashAddr_t applFlashEnd(U8 nbr);
U16 applChecksum(U8 nbr);
CoblRet_t applCheckConfiguration(U8 nbr);
CoblRet_t applCheckChecksum(U8 nbr);
CoblRet_t applCheckImage(void);
const ConfigBlock_t * applConfigBlock(U8 nbr);

#endif /* COBL_APPLICATION_H */
