/**
 * \file main.c
 *
 * Main C source for DPMU CPU1
 */

// System includes.
#include <file.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Application includes.
#include "app/pt-1.4/pt.h"
#include "application_vars.h"
#include "board.h"
#include "check_CPU2.h"
#include "cli_cpu1.h"
#include "co.h"
#include "co_canopen.h"
#include "common.h"
#include "co_p401.h"
#include "debug_log.h"
#include "initialization_app.h"
#include "emifc.h"
#include "error_handling.h"
#include "ext_flash.h"
#include "flash_api.h"
#include "fwupdate.h"
#include "GlobalV.h"
#include "i2c_test.h"
#include "ipc_cpu1.h"
#include "lfs_api.h"
#include "log.h"
#include "main.h"
#include "serial.h"
#include "shared_variables.h"
#include "startup_sequence.h"
#include "temperature_sensor.h"
#include "timer.h"
#include "watchdog.h"

struct Serial cli_serial;

// Command structure managed by CPU1.
#pragma DATA_SECTION(cpu1_command, "ramgs0")
cpu1_command_t cpu1_command;

// Status structure managed by CPU2.
#pragma DATA_SECTION(cpu2_status, "ramgs4")
cpu2_status_t cpu2_status;

#pragma DATA_SECTION(ipc_msg, "MSGRAM_CPU1_TO_CPU2")
msg_t ipc_msg;

#pragma DATA_SECTION(canb_incoming_msg_cpy, "MSGRAM_CPU2_TO_CPU1")
canb_msg_t canb_incoming_msg_cpy = {
        .raw = {0, 0, 0, 0, 0, 0, 0, 0}
};

static void CPU1_Board_init();
static void check_cpu2_ind(void);
void check_cpu2_dbg(void);
void toogle_LED1(timer_t *tqe);



/**
 * The startup function.
 */
void main(void)
{

    Device_init();

    /* configure and the start watchdog */
    watchdog_init();

    /* initiate CANopen stack */
    /* also starts the heart-beat */
    co_init();
    coEventRegister_SDO_SERVER_DOMAIN_READ(log_read_domain);

    Serial_set_debug_level(DEBUG_ERROR);
    Serial_set_debug_level(DEBUG_INFO);

    CPU1_Board_init();

    //
    // Clear any IPC flags if set already
    //
    IPC_init(IPC_CPU1_L_CPU2_R);

    EINT;
    ERTM;

    //
    // Synchronize both the cores.
    //
    //IPC_sync(IPC_CPU1_L_CPU2_R, IPC_FLAG31);

    asm(" RPT #5 || NOP");

    // Initialise timer queue.
    timerq_init();

    /* initialize cli/serial/UART */
    Serial_ctor(&cli_serial);   // Initialize Serial struct
    cli_ctor(&cli_serial);      // Initialize cli struct
    Serial_open(&cli_serial);   // Open the CLI serial device.
    cli_init();                 // Initialize the CLI.

    // Write a short welcome message to serial device.
    Serial_printf(&cli_serial, "\r\n\n\nHello from DPMU\r\n");

    /* define the addresses of the boot configuration memories */
    uint32_t boot_pin_config = *((uint32_t *)0x00078008);
    uint32_t boot_def_low = *((uint32_t *)0x0007800C);

    /* print the content of the boot configuration memories */
    Serial_printf(&cli_serial, "BOOTPINCONFIG = 0x%lX\r\n", boot_pin_config);
    Serial_printf(&cli_serial, "BOOTDEF-LOW   = 0x%lX\r\n", boot_def_low);
    Serial_printf(&cli_serial, "*************** DPMU_CPU1 Firmware compilation ********************\r\n");
    Serial_printf(&cli_serial, "\r\nDPMU_CPU1 Firmware compilation timestamp= %s %s\r\n", __DATE__, __TIME__ );



    cli_ok();
    Serial_printf(&cli_serial, "DPMU_CPU1 CLI OK\r\n" );
    ext_flash_config();
    Serial_printf(&cli_serial, "DPMU_CPU1 ext_flash_config\r\n" );
    log_can_init();


    Serial_printf(&cli_serial, "IPC_PUMPREQUEST_REG %08X\r\n", IPC_PUMPREQUEST_REG);

    //
    // Boot CPU2 core
    //
    Serial_printf(&cli_serial, "DPMU_CPU1 Booting DPMU CPU2\r\n" );
    Device_bootCPU2(BOOTMODE_BOOT_TO_FLASH_SECTOR0);
    Serial_printf(&cli_serial, "DPMU_CPU1 DPMU CPU2 boot called\r\n" );

    /*** FW update of CPU2 ***/
    uint16_t cpu2BinaryStatus = fwupdate_updateExtRamWithCPU2Binary();
//    Serial_printf(&cli_serial, "File[%s] - function[%s] - line[%d]\r\n", __FILE__, __FUNCTION__, __LINE__);
    switch (cpu2BinaryStatus){
        case fw_not_available:
            Serial_printf(&cli_serial, "\r\n Cpu2 Firmware Not Available \r\n");
        break;

        case fw_checksum_error:
            Serial_printf(&cli_serial, "\r\n Cpu2 Firmware Checksum Not OK \r\n");

            /* Reset external RAM*/
            /*ToDo: Change RAM Size if Required*/
            memset ((uint32_t*)EXT_RAM_START_ADDRESS_CS2, 0, EXT_RAM_SIZE_CS2 );
        break;

        case fw_ok:
            Serial_printf(&cli_serial, "\r\n Cpu2 Firmware Updated \r\n");
            /* Flags to sync/signal cpu2 bootloader */
            ipc_sync_comm(IPC_FLAG11, true);
        break;

        default:
        break;
    }

    // Release EMIF1, CPU2 might need it in its bootloader
    emifc_realease_cpun_as_master(CPU_TYPE_ONE);

    /* sharedVars_cpu1toCpu2
     * set CPU1 as master for source memory */
    MemCfg_setGSRAMMasterSel(MEMCFG_SECT_GS2, MEMCFG_GSRAMMASTER_CPU1);

    /* sharedVars_cpu2toCpu1
     * set CPU2, the other core, as master for source memory */
    MemCfg_setGSRAMMasterSel(MEMCFG_SECT_GS3, MEMCFG_GSRAMMASTER_CPU2);

    /* Clear state vector */
    sharedVars_cpu1toCpu2.iop_operation_request_state = 0; //TODO Set to IDLE

    log_debug_log_set_state(true);

    watchdog_feed();

    if( sharedVars_cpu1toCpu2.dpmu_default_flag == true ) {
        Serial_printf(&cli_serial, "DPMU IS DEFAULT\r\n");
    } else {
        Serial_printf(&cli_serial, "DPMU IS REDUNDANT\r\n");
    }
    /* flag to sync the cpu2 application
     *
     * still being able to communicate with IOP
     * in case of CPU2 error
     * */
    ipc_sync_comm(IPC_FLAG31, true);


    //
    // Initialize I2C HW
    //
    temperature_sensors_init();

    static timer_t async_timer_toogle_LED1;
    int32_t duration = 500;
    timer_init(&async_timer_toogle_LED1, duration, "Test timer", toogle_LED1, NULL, true);
    timer_start(&async_timer_toogle_LED1);


    /* load default values stored in external FLASH */

    Serial_debug(DEBUG_INFO, &cli_serial, "IPC_PUMPREQUEST_REG %08X\r\n", IPC_PUMPREQUEST_REG);
    Serial_debug(DEBUG_INFO, &cli_serial, "Started\r\n");
    AppVarsReadRequest();

    for (;;) {

        cli_check_for_new_commands_from_UART();

        while (coCommTask() == CO_TRUE);

        co401Task();

        // Check for messages from CPU2.
        check_cpu2_ind();

        // Forward any debug messages from CPU2 to cli_serial device.
        check_cpu2_dbg();

        /* check if there are new debug data to store */
        //log_store_debug_log_to_ram();
        //log_debug_read_from_ram( );
        log_can_state_machine();


        /* check every second */
        readAlltemperatures();

        HandleAppVarsOnExternalFlashSM();

        /* check changes from controlling algorithm in CPU2 */
        check_changes_from_CPU2();

        /* check for errors and handle them */
        error_check_for_errors();

        // Handle timerq.
        timerq_tick();

        /* WARNING - make sure the WD is NOT fead to often !
         * cause you might do the good thing and implement the
         * window functionality of the WD
         * */
        watchdog_feed();
    }
}

/**
 * Check for messages from CPU2.
 */
static void check_cpu2_ind(void)
{
    uint32_t cmd32 = 0xFFFFFFFF;
    uint32_t addr;
    uint32_t data;

    cmd32 = 0xFFFFFFFF;

    if (IPC_readCommand(IPC_CPU1_L_CPU2_R, IPC_FLAG_MESSAGE_CPU2_TO_CPU1, false, &cmd32, &addr, &data) != false) {

        switch (cmd32) {
            case IPC_SHORT_CIRCUIT:
                Serial_printf(&cli_serial, "\r\n*** SHORT CIRCUIT #%lu ***\r\n\r\n->", cpu2_status.num_short_circ);
                // Acknowledge the flag.
                IPC_ackFlagRtoL(IPC_CPU1_L_CPU2_R, IPC_FLAG_MESSAGE_CPU2_TO_CPU1);
                break;

            default:
                break;
        }


    }
}

/**
 * Check for debug messages from CPU2.
 *
 * These messages are meant to be forwarded to the cli_serial device.
 */
void check_cpu2_dbg(void)
{
    uint32_t cmd;
    uint32_t addr;
    uint32_t data;

    if (IPC_readCommand(IPC_CPU1_L_CPU2_R, IPC_FLAG_CPU2_DBG, false, &cmd, &addr, &data) != false) {
        switch (cmd) {
        case IPC_CPU2_PRINT:
            if (addr != 0) {
                Serial_printf(&cli_serial, "%s", (char*) addr);
            }
            break;

        default:
            break;
        }

        // Acknowledge the flag.
        IPC_ackFlagRtoL(IPC_CPU1_L_CPU2_R, IPC_FLAG_CPU2_DBG);
    }
}



static void CPU1_Board_init()
{
    EALLOW;

    PinMux_init();
    SYSCTL_init();
    INPUTXBAR_init();
//    SYNC_init();
    CPUTIMER_init();
    DMA_init();
    EPWM_init();
    GPIO_init();
    I2C_init();
    MEMCFG_init();
    SCI_init();
    INTERRUPT_init();

    EDIS;
}

void toogle_LED1(timer_t *tqe)
{
    //GPIO_writePin(LED1, !GPIO_readPin(LED1));
}
