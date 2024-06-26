/*
 * boot.h
 *
 *  Created on: 10 feb. 2023
 *      Author: vb
 */

#ifndef INC_BOOT_H_
#define INC_BOOT_H_

#include "stdint.h"
#include "emifc.h"
/*
 * (2 * ((256 * 1024ul) - FLASH_BL_SIZE)) // 480 KB (Remaining FLASH size)
 *
 * */
//#define APPLICATION_IMAGE_SIZE    (2*240*1024ul)
#define APPLICATION_IMAGE_SIZE    (2*32*1024ul)


#define FLASH_ONE_CALL_SIZE 16u   // Number of bytes that are flashed at the same time

#define FLASH_BL_SIZE         (2 * 16 * 1024ul) // 32KB  (BL size in kbytes )
#define FLASH_APPL_START      (0x088000)
//#define FLASH_APPL_START      (0x0A0000)
//#define FLASH_APPL_START      (0x000A0000)
#define FLASH_APPL_END        (FLASH_APPL_START + (APPLICATION_IMAGE_SIZE / 2) - 1) // 0x000B FFFF
#define FLASH_ERASE_SIZE      (8ul*1024ul)


#define EXT_RAM_CFG_BLOCK_SIZE     8
#define EXT_RAM_CFG_BLOCK_ADDRESS  (0x00100000U)
#define EXT_RAM_APPL_START         (EXT_RAM_CFG_BLOCK_ADDRESS + EXT_RAM_CFG_BLOCK_SIZE)
#define EXT_RAM_APPL_END           (EXT_RAM_APPL_START + (APPLICATION_IMAGE_SIZE/2) -1)

typedef struct{
    uint32_t applicationSize;
    uint16_t crcSum;

}ConfigBlock_t;

typedef struct{
    EMIF1_Config*  emif;
    ConfigBlock_t* config;
}MemoryConfig_t;


ConfigBlock_t* boot_getConfigMemoryBlock();
uint32_t boot_calculateCRC(const unsigned char* bootImageAddress, uint32_t bootImageSize);
void boot_upgradeApplication();
void boot_jumpToApplication();
void boot_jumpToApplicationAddr( uint32_t bootAddress );

#endif /* INC_BOOT_H_ */
