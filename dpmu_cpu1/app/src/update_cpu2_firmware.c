
#include <ipc.h>
#include <stdbool.h>

#include "emifc.h"
#include "fwupdate.h"

bool startCPU2FirmwareUpdateFlag = false;

void startCPU2FirmwareUpdate() {
    startCPU2FirmwareUpdateFlag = true;
}

bool cpu2FirmwareReady() {
    return startCPU2FirmwareUpdateFlag;
}

void executeCPU2FirmwareUpdate(struct Serial *cli_serial ){

    if( startCPU2FirmwareUpdateFlag == false ) {
        return;
    }
    /*** FW update of CPU2 ***/
    uint16_t cpu2BinaryStatus = fwupdate_updateExtRamWithCPU2Binary();
    switch (cpu2BinaryStatus)
    {
    case fw_not_available:
        Serial_printf(cli_serial, "\r\n Cpu2 Firmware Not Available \r\n");
        break;
    case fw_checksum_error:
        Serial_printf(cli_serial, "\r\n Cpu2 Firmware Checksum Not OK \r\n");
        /* Reset external RAM*/
        /*ToDo: Change RAM Size if Required*/
        memset((uint32_t*) EXT_RAM_START_ADDRESS_CS2, 0, EXT_RAM_SIZE_CS2);
        break;
    case fw_ok:
        Serial_printf(cli_serial, "\r\n Cpu2 Firmware Updated \r\n");
        break;
    default:
        break;
    }
    // Release EMIF1, CPU2 might need it in its bootloader
    emifc_realease_cpun_as_master(CPU_TYPE_ONE);
    /* Flags to sync/signal cpu2 bootloader */
    // IPC_sync(IPC_CPU1_L_CPU2_R, IPC_FLAG11);
    IPC_setFlagLtoR(IPC_CPU1_L_CPU2_R, IPC_FLAG11);

    startCPU2FirmwareUpdateFlag = false;
}
