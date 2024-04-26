#include "boot.h"
#include "cli_cpu2.h"
#include "flash_api.h"
#include "string.h"
#include "emifc.h"
#include "emif.h"

typedef enum{
    upgrade_state_start = 0,
    upgrade_state_delete_primary,
    upgrade_state_flash_primary,
    upgrade_state_delete_secondary,
    upgrade_state_finish
}UpgradeState;

static uint16_t memoryBuffer[FLASH_ONE_CALL_SIZE];

static UpgradeState state = upgrade_state_start;
static uint16_t applicationUpgraded = 0;

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

#define CONFIG_DSP

uint32_t boot_calculateCRC(const unsigned char* pBuffer, uint32_t bootImageSize){

    uint32_t lcount = bootImageSize;
    uint16_t u16Crc = 0;
    unsigned char u8Val;

    while(lcount --)  {
#ifdef CONFIG_DSP
        /* low part */
        u8Val = *pBuffer & 0x00FFu;

        u16Crc = (u16Crc << 8) ^ crcTable[ (unsigned char)(u16Crc >> 8) ^ u8Val];

        /* high part */
        u8Val = (*pBuffer++ >> 8) & 0x00FFu;
#else
        u8Val = *pBuffer++;
#endif
        u16Crc = (u16Crc << 8) ^ crcTable[ (unsigned char)(u16Crc >> 8) ^ u8Val];
    }

    return(u16Crc);
}

static ConfigBlock_t configBlock;
static EMIF1_Config  emifBlock;

void boot_getMemoryConfiguration(MemoryConfig_t* pMemoryConfig){
    EMIF1_Config* pEmif = &emifBlock;

    ConfigBlock_t* pConfigBlock = boot_getConfigMemoryBlock();

    /* Change EMIF Block Config to point to the application in memory*/
    pEmif->address = EXT_RAM_APPL_START;
    pEmif->size    = FLASH_ONE_CALL_SIZE/2;

    pMemoryConfig->config = pConfigBlock;
    pMemoryConfig->emif   = pEmif;
}

ConfigBlock_t* boot_getConfigMemoryBlock(){

    ConfigBlock_t* pConfigBlock = &configBlock ;
    EMIF1_Config* pEmifBlock = &emifBlock;

    pEmifBlock->address = EXT_RAM_CFG_BLOCK_ADDRESS;
    pEmifBlock->cpuType = CPU_TYPE_TWO;
    pEmifBlock->data    = memoryBuffer;
    pEmifBlock->size    = 4;

    emifc_cpu_read_memory(pEmifBlock);

    pConfigBlock->applicationSize = memoryBuffer[0];
    pConfigBlock->crcSum          = memoryBuffer[2];

    return pConfigBlock;
}

void boot_upgradeApplication()
{
    static uint32_t  flashAddress;
    static uint32_t  eraseAddress;
    static uint32_t  noErasePages;
    static int32_t   sizeLeft;

    MemoryConfig_t memConfig;
    boot_getMemoryConfiguration(&memConfig);
    uint32_t writeSize;
    UpgradeState nextState = state;


    while(1){
        switch (state){
        case upgrade_state_start:
            eraseAddress = FLASH_APPL_START;
            flashAddress = FLASH_APPL_START;
            sizeLeft     = memConfig.config->applicationSize;
            noErasePages = APPLICATION_IMAGE_SIZE/FLASH_ERASE_SIZE;
            emifc_configure_cpu2();
            nextState = upgrade_state_delete_primary;
            break;
        case upgrade_state_delete_primary:

            flash_api_erase(eraseAddress);
            eraseAddress +=FLASH_ERASE_SIZE;
            noErasePages --;

            if (noErasePages == 0){
                nextState = upgrade_state_flash_primary;
            }
            PRINT("STATE ERASE\r\n");
            break;
        case upgrade_state_flash_primary:

            if (sizeLeft >  FLASH_ONE_CALL_SIZE/2){
                writeSize = FLASH_ONE_CALL_SIZE/2;
//                PRINT("STATE FLASH 1\r\n");
            }
            else if (sizeLeft > 0){
                writeSize = sizeLeft;
//                PRINT("STATE FLASH 2\r\n");
            }
            else {
                writeSize = 0;
                nextState = upgrade_state_delete_secondary;
//                PRINT("STATE FLASH DONE\r\n");
            }

            if (writeSize > 0){

                /* Read 8 bytes at a time*/
                emifc_cpu_read_memory(memConfig.emif);
                // Write to the flash
                flash_api_write(flashAddress, memConfig.emif->data);
                flashAddress += writeSize;
                memConfig.emif->address += writeSize;
                sizeLeft-=writeSize;
                memset (memConfig.emif->data, 0, memConfig.emif->size);
//                PRINT("STATE FLASH 4\r\n");
            }

            break;
        case upgrade_state_delete_secondary:
            memset((uint32_t*)EXT_RAM_APPL_START, 0, APPLICATION_IMAGE_SIZE);

            // Release EMIF1
             EMIF_selectMaster(EMIF1CONFIG_BASE, EMIF_MASTER_CPU1_NG2);

            nextState = upgrade_state_finish;
            PRINT("STATE FLASH DELETE SECONDARY IMAGE\r\n");
            break;
        case upgrade_state_finish:
            applicationUpgraded = 1;
            PRINT("STATE FINISH\r\n");
            break;
        default:
            break;
        }

        if (nextState!=state){
            PRINT("CPU 2 bootUpggrade - NEXT STATE:[%d]\r\n", nextState);
            state = nextState;
        }
        if (applicationUpgraded){
            return;
        }
    }
}

void boot_jumpToApplication()
{

//    pAppl = (void (*)(void))(FLASH_APPL_START); /* BEGIN section, call code_start  */
//    pAppl();
}

void boot_jumpToApplicationAddr( uint32_t bootAddress )
{
    void (*pAppl)(void);

    pAppl = (void (*)(void))(bootAddress);

    pAppl();
}
