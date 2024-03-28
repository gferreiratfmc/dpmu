/*
* cobl_user.h
*
* Copyright (c) 2015-2020 emotas embedded communication GmbH
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
* \brief cobl_user.h
*
* User specific functionality
*
*/



#ifndef COBL_USER_H
#define COBL_USER_H 1


/* datatypes
*---------------------------------------------------------------------------*/

/* function prototypes
*---------------------------------------------------------------------------*/
U8 userGetNodeId(void);
U16 userGetBitrate(void);


U32 userOd1000(void);
U32 userOd1018_1(void);
U32 userOd1018_2(void);
U32 userOd1018_3(void);
U32 userOd1018_4(void);
U32 userOd5F00_0(void);


U32 userOdU32ReadFct(U16 index, U8 subIndex);
CoblObjInfo_t userSegReadFct(U16 index, U8 subIndex);
#endif

