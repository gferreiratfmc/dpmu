#include "boot.h"
#include "cli_cpu2.h"
#include "device.h"
#include "driverlib.h"
#include "emifc.h"
#include "flash.h"
#include "flash_api.h"
#include "flash_programming_f2838x_c28x.h"

// Buffer used to send CPU2 debug messages to CPU1 for sending to the serial port.
#pragma DATA_SECTION(ipc_debug_msg, "MSGRAM_CPU2_TO_CPU1")
char ipc_debug_msg[MAX_CPU2_DBG_LEN + 1];

void main(void){
    uint32_t crcCalculated;

    /* disable all interrupts, if not disabled after reset */
    DINT;
    Device_init();

    /* disable interrupt */
    DINT;
    IER = 0x0000;
    IFR = 0x0000;

    //
    // Clear any IPC flags if set already
    //
    IPC_init(IPC_CPU1_L_CPU2_R);

    PRINT("\r\nBOOTLOADER CPU2 STARTED\r\n");

    ConfigBlock_t * pConfig = boot_getConfigMemoryBlock();

    PRINT("IPC_PUMPREQUEST_REG %08lX\r\n", IPC_PUMPREQUEST_REG);
    PRINT("FLASH_FBPRDY_BANKRDY %08lX ", FLASH0CTRL_BASE + 0x22);

    //
    // Sync CPU1 and CPU2.
    //
//    IPC_sync(IPC_CPU2_L_CPU1_R, IPC_FLAG11);
    /* Check if firmware is available in EXT RAM*/
    PRINT("CRC %08X\r\n", pConfig->crcSum);
    if (pConfig->crcSum != 0x00000000){
        PRINT("CRC EXCIST\r\n");

        crcCalculated = boot_calculateCRC((unsigned char*)EXT_RAM_APPL_START, pConfig->applicationSize);

        if(crcCalculated == pConfig->crcSum ){
            PRINT("CRC OK\r\n");
#ifdef _FLASH
            memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (uint32_t)&RamfuncsLoadSize);
#endif
            /* initialize basic flash */
            Flash_initModule(FLASH0CTRL_BASE, FLASH0ECC_BASE, 3);
            Flash_claimPumpSemaphore(FLASH_CPU2_WRAPPER);

            boot_upgradeApplication();

            /* release flash pump so bootloader in CPU1 can use it */
            Flash_releasePumpSemaphore();
        }
    }

    // Release EMIF1
    emifc_realease_cpun_as_master(CPU_TYPE_TWO);

    PRINT("BL CPU2 DONE\r\n");

    boot_jumpToApplication();

    while(1){
    }
}
