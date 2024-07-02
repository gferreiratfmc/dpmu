/*
* fwupdate.c - contains update functionality
*
* Copyright (c) 2014 emotas embedded communication GmbH
*-------------------------------------------------------------------
* $Id: fwupdate.c 14796 2016-08-12 12:07:26Z ro $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \file
* \brief firmware update
* Jump to the bootloader for updating.
*
*/

/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serial.h"

/* header of project specific types
---------------------------------------------------------------------------*/
#include <gen_define.h>


#ifdef BOOT
#include <sysctl.h>
#include <co_datatype.h>
#include <fwupdate.h>
#include "emifc.h"
#include "flash_api.h"

static const uint16_t crcTable[256] = {
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


/* constant definitions
---------------------------------------------------------------------------*/

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
--------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/
#pragma DATA_SECTION(cobl_command,"command");
UNSIGNED8 cobl_command[16];

/* local defined variables
---------------------------------------------------------------------------*/
extern struct Serial cli_serial;

uint16_t binaryImageBufferRead[IMAGE_BUFFER_SIZE];
uint16_t binaryImageBuffer[IMAGE_BUFFER_SIZE];

/***************************************************************************/
/**
* \brief jump to the bootloader
*
* \param
*	nothing
* \results
*	nothing
*/
void jump2Bootloader(void)
{
    void (*pAppl)(void);
    DINT;

    memcpy( &cobl_command[0], COMMAND_BOOT, COMMAND_SIZE);

    /* we jump directoy to start of boot loader */
    pAppl = (void (*)(void))0x00080000;
    pAppl();
}

#endif /* BOOT */


static FWImageStatus fwupdate_isCPU2ImageAvailable(){

    uint32_t data;

//    flash_api_init();
    /*Read checksum*/
    memcpy( &data, (uint32_t*)FLASH_CPU2_CHECKSUM_ADDRESS, sizeof(data) );
    Serial_printf(&cli_serial, "\r\ndata from FLASH_CPU2_CHECKSUM_ADDRESS:[0x%08p]\r\n", data);

   if (data!= 0xFFFFFFFF){
       return fw_available;
   }
   return fw_not_available;

}

#define CONFIG_DSP
static uint16_t fwupdate_calculateChecksum(const unsigned char* bootImageAddress, uint32_t bootImageSize){

        uint32_t lcount = 0;
        unsigned char valueFromRAM;
        uint16_t u16Crc = 0;
        unsigned char u8Val;
        int32_t remainingWords;
        uint16_t wordsToTransfer;

        uint32_t readAdrress;

        EMIF1_Config emif;

        uint32_t count = 0;

        lcount = ( bootImageSize / (TRANSFER_SIZE/2) ) + 1;

        remainingWords = bootImageSize;
        wordsToTransfer = TRANSFER_SIZE/2;
        readAdrress = (uint32_t)bootImageAddress;

        Serial_printf(&cli_serial, "bootImageAddress:[0x%08p] bootImageSize:[%lu]\r\n",bootImageAddress, bootImageSize );

        while( lcount-- )  {
            if(  remainingWords <  TRANSFER_SIZE/2) {
                 wordsToTransfer = remainingWords;
            }

            emif.address = readAdrress;
            emif.cpuType = CPU_TYPE_ONE;
            emif.data   = (uint16_t*)message;
            emif.size   = wordsToTransfer;

            emifc_cpu_read_memory(&emif);

            for( int i = 0; i< wordsToTransfer; i++) {

                for( int j = 0; j < 2; j++) {
                    count++;
                    if( j == 0 ) {
                        valueFromRAM = 0x00FF & message[i];
                    } else {
                        valueFromRAM = 0xFF00 & message[i];
                        valueFromRAM = valueFromRAM >> 8;

                    }
                    u8Val = valueFromRAM & 0x00FFu;

                    u16Crc = (u16Crc << 8) ^ crcTable[ (unsigned char)(u16Crc >> 8) ^ u8Val];

                }
            }

            readAdrress = readAdrress + (wordsToTransfer);
            remainingWords = remainingWords - wordsToTransfer;

            if( remainingWords <= 0) {
                break;
            }
        }
        Serial_printf(&cli_serial, "CRC16 processed Bytes=%lu\r\n", count);
        return(u16Crc);
}



FWImageStatus fwupdate_updateExtRamWithCPU2Binary(){

    uint32_t readFromFlashAddressWords, writeToRamAddressWords;
    uint32_t readFromFlashAddressBytes, writeToRamAddressBytes;

    uint32_t imageSizeFromMemory;
    uint16_t imageChecksum;
    uint16_t imageChecksumFromMemory;
    FWImageStatus applicationOK = fw_not_available;
    int32_t remainingWords, wordsToTransfer;
    uint32_t lcount = 0;

    EMIF1_Config emif = {.address = EXT_RAM_START_ADDRESS_CS2,
                         .cpuType = CPU_TYPE_ONE,
                         .data   = (uint16_t*)binaryImageBuffer,
                         .size   = IMAGE_BUFFER_SIZE};

    EMIF1_Config emifRead = {.address = EXT_RAM_START_ADDRESS_CS2,
                             .cpuType = CPU_TYPE_ONE,
                             .data   = (uint16_t*)binaryImageBufferRead ,
                             .size   = IMAGE_BUFFER_SIZE};


    /*Get the application size from flash*/
    memcpy (&imageSizeFromMemory, (uint32_t*)FLASH_CPU2_CONFIG_BLOCK_ADDRESS, sizeof(imageSizeFromMemory));

    /* Reset external RAM*/
    memset ((uint32_t*)EXT_RAM_START_ADDRESS_CS2, 0, EXT_RAM_SIZE_CS2 );


    /*Check if binary is available in CPU1 Flash*/
    readFromFlashAddressWords = FLASH_CPU2_CONFIG_BLOCK_ADDRESS;
    readFromFlashAddressBytes = FLASH_CPU2_CONFIG_BLOCK_ADDRESS;
    writeToRamAddressWords = EXT_RAM_START_ADDRESS_CS2;
    writeToRamAddressBytes = EXT_RAM_START_ADDRESS_CS2;
    Serial_printf(&cli_serial, "File[%s] - function[%s] - line[%d]\r\n", __FILE__, __FUNCTION__, __LINE__);
    if (fwupdate_isCPU2ImageAvailable()){
        lcount = ( ( imageSizeFromMemory + FLASH_CPU2_CONFIG_BLOCK_SIZE_IN_WORDS ) / (IMAGE_BUFFER_SIZE) ) + 1;
        remainingWords = imageSizeFromMemory + FLASH_CPU2_CONFIG_BLOCK_SIZE_IN_WORDS;
        wordsToTransfer = IMAGE_BUFFER_SIZE;

        Serial_printf(&cli_serial, "File[%s] - function[%s] - line[%d]\r\n", __FILE__, __FUNCTION__, __LINE__);
        while( lcount-- ) {

            if(  remainingWords <  IMAGE_BUFFER_SIZE) {
                 wordsToTransfer = remainingWords;
            }
            //Serial_printf(&cli_serial, "\r\n lcount:[%lu], wordsToTransfer:[%ld], remainingWords:[%ld]\r\n",lcount, wordsToTransfer, remainingWords);
            //Serial_printf(&cli_serial, "File[%s] - function[%s] - line[%d]\r\n", __FILE__, __FUNCTION__, __LINE__);
            memset( binaryImageBuffer, 0,  wordsToTransfer);
            //Serial_printf(&cli_serial, "File[%s] - function[%s] - line[%d]\r\n", __FILE__, __FUNCTION__, __LINE__);
            /* Start with config block*/
            flash_api_read(readFromFlashAddressWords, (uint16_t*) binaryImageBuffer, wordsToTransfer);


            //Serial_printf(&cli_serial, "File[%s] - function[%s] - line[%d]\r\n", __FILE__, __FUNCTION__, __LINE__);

//            for (uint16_t c = 0; c < wordsToTransfer; c++)
//            {
//                if (((readFromFlashAddressBytes + (c * 2)) % wordsToTransfer) == 0)
//                {
//                    Serial_printf(&cli_serial, "\r\n W[0x%08p]:", readFromFlashAddressBytes + (c * 2));
//                }
//                Serial_printf(&cli_serial, " 0x%04X", binaryImageBuffer[c]);
//            }

            memset(message, 0, wordsToTransfer);
            memcpy(message, binaryImageBuffer, wordsToTransfer);
            emif.address = writeToRamAddressWords;
            emif.data = (uint16_t*) message;
            emif.size = wordsToTransfer;
            emif.cpuType = CPU_TYPE_ONE;

            emifc_cpu_write_memory(&emif);

            emifRead.address = writeToRamAddressWords;
            emifRead.data = (uint16_t*) message;
            emifRead.size = wordsToTransfer;
            emifRead.cpuType = CPU_TYPE_ONE;
            memset(binaryImageBufferRead, 0, wordsToTransfer);
            emifc_cpu_read_memory(&emifRead);
            memcpy(binaryImageBufferRead, message, wordsToTransfer);

//            for (uint16_t c = 0; c < wordsToTransfer; c++)
//            {
//                if ((writeToRamAddressBytes + (c * 2)) % wordsToTransfer == 0)
//                {
//                    Serial_printf(&cli_serial, "\r\n R[0x%08p]:", writeToRamAddressBytes + (c * 2));
//                }
//                Serial_printf(&cli_serial, " 0x%04X", binaryImageBufferRead[c]);
//            }

            readFromFlashAddressWords = readFromFlashAddressWords + wordsToTransfer;
            readFromFlashAddressBytes = readFromFlashAddressBytes + 2 * wordsToTransfer;
            writeToRamAddressWords = writeToRamAddressWords + wordsToTransfer;
            writeToRamAddressBytes = writeToRamAddressBytes + 2 * wordsToTransfer;

            remainingWords = remainingWords - wordsToTransfer;

            if (remainingWords <= 0)
            {
                break;
            }

        }

       /* Calculate the checksum of the image written to the external RAM*/
       imageChecksum = fwupdate_calculateChecksum((unsigned char *)EXT_RAM_START_ADDRESS_CS2 + 0x08, imageSizeFromMemory);

       Serial_printf(&cli_serial, "\r\n FW CPU2 Checksum fwupdate_calculateChecksum:[0x%04X] \r\n", imageChecksum );

       /*Read the checksum from flash*/
       memcpy (&imageChecksumFromMemory, (unsigned char *)(FLASH_CPU2_CONFIG_BLOCK_ADDRESS+0x02), sizeof(imageChecksumFromMemory));
       Serial_printf(&cli_serial, "\r\n FW CPU2 Checksum imageChecksumFromMemory:[0x%04X] \r\n", imageChecksumFromMemory );

       if (imageChecksum == imageChecksumFromMemory){
           applicationOK = fw_ok;
       } else {
           applicationOK = fw_checksum_error;
       }

    }

    Serial_printf(&cli_serial, "File[%s] - function[%s] - line[%d]\r\n", __FILE__, __FUNCTION__, __LINE__);
    return applicationOK;

}
