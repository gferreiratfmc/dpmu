/**
 * @file    main.c
 *
 * Main module of DPMU CPU2
 *
 */
//git diff branch1:fil1 branch2:fil2
//git checkout branch -- sökväg/filnamn

//
// Included Files
//
#include <error_handling.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <switches.h>

#include "board.h"
#include "charge.h"
#include "cli_cpu2.h"
#include "common.h"
#include "dcbus.h"
#include "DCDC.h"
#include "debug_log.h"
#include "device.h"
#include "driverlib.h"
#include "energy_storage.h"
#include "hal.h"
#include "sensors.h"
#include "shared_variables.h"
#include "state_machine.h"
#include "switches.h"
#include "switch_matrix.h"
#include "timer.h"

#define PERIOD_1_MS  1
#define PERIOD_10_MS (PERIOD_1_MS * 10)
#define PERIOD_1_S   (PERIOD_1_MS * 1000)

// This flag variable will be set by EPWM Trip Zone interrupt handlers.
volatile uint16_t efuse_top_half_flag = 0;


static void check_incoming_commands(void)
{
    uint32_t command;
    uint32_t addr;
    uint32_t data;
    uint8_t  state;

//    if( IPC_isFlagBusyRtoL(IPC_CPU1_L_CPU2_R, IPC_SWITCHES_QINB) ){
//        PRINT("===========> Setting QINB:%d\r\n", sharedVars_cpu1toCpu2.QinbSwitchRequestState);
//        switches_Qinb( sharedVars_cpu1toCpu2.QinbSwitchRequestState );
//        IPC_ackFlagRtoL(IPC_CPU1_L_CPU2_R, IPC_SWITCHES_QINB);
//    }
//
//    if( IPC_isFlagBusyRtoL(IPC_CPU1_L_CPU2_R, IPC_SWITCHES_QLB)){
//        PRINT("===========> Setting QLB:%d\r\n", sharedVars_cpu1toCpu2.QlbSwitchRequestState);
//        switches_Qlb( sharedVars_cpu1toCpu2.QlbSwitchRequestState );
//        IPC_ackFlagRtoL(IPC_CPU1_L_CPU2_R, IPC_SWITCHES_QLB);
//    }
//
//    if( IPC_isFlagBusyRtoL(IPC_CPU1_L_CPU2_R, IPC_SWITCHES_QSB)){
//        PRINT("===========> Setting QSB:%d\r\n", sharedVars_cpu1toCpu2.QsbSwitchRequestState);
//        switches_Qsb( sharedVars_cpu1toCpu2.QsbSwitchRequestState );
//        IPC_ackFlagRtoL(IPC_CPU1_L_CPU2_R, IPC_SWITCHES_QSB);
//    }
//
//    if( IPC_isFlagBusyRtoL(IPC_CPU1_L_CPU2_R, IPC_SWITCHES_QIRS)){
//        //switches_Qinrush( sharedVars_cpu1toCpu2.QlbSwitchRequestState );
//        IPC_ackFlagRtoL(IPC_CPU1_L_CPU2_R, IPC_SWITCHES_QIRS);
//    }


    if (IPC_readCommand(IPC_CPU2_L_CPU1_R, IPC_FLAG_MESSAGE_CPU1_TO_CPU2, false, &command, &addr, &data) != false) {
        // We received a command.

        // Copy sub-command to message buffer.
        strncpy((char*)&state, (char*)addr,  1);

        switch (command) {
        case IPC_PING:
            IPC_sendResponse(IPC_CPU2_L_CPU1_R, IPC_PONG);
            break;

        case IPC_GET_TICKS:
            IPC_sendResponse(IPC_CPU2_L_CPU1_R, timer_get_ticks());
            break;
        case IPC_SWITCHES_QIRS:
            //switches_Qinrush(state); /* run inrush current limiter */
            break;

        case IPC_SWITCHES_QLB:
            switches_Qlb(state);
            break;

        case IPC_SWITCHES_QSB:
            switches_Qsb(state);
            break;

        case IPC_SWITCHES_QINB:
            switches_Qinb(state);
            break;

        default:
            break;
        }

        // Acknowledge the flag.
        IPC_ackFlagRtoL(IPC_CPU2_L_CPU1_R, IPC_FLAG_MESSAGE_CPU1_TO_CPU2);

        if( sharedVars_cpu1toCpu2.debug_log_disable_flag==true ) {
            DisableDebugLog();
        } else {
            EnableDebugLog();
        }
    }
}

/*
 * Called by super loop to handle any interrupt triggered by the interrupts:
 * eFuseBB (external interrupt 5)
 * eFuseVin  (GLOAD_4_3 EPWM One Shot Interrupt)
 */
static void handle_top_half_interrupts(void)
{
    if (efuse_top_half_flag == true) {
        cpu2_status.num_short_circ += 1;
        // Send a short-circuit indication message to CPU1.
        IPC_sendCommand(IPC_CPU2_L_CPU1_R, IPC_FLAG_MESSAGE_CPU1_TO_CPU2, false, IPC_SHORT_CIRCUIT, 0, 0);
        efuse_top_half_flag = false;
    }

    // A trip interrupt was generate to the GLOAD_4_3 EPWM.
    if ( eFuseInputCurrentOcurred == true ) {
        handleEfuseVinOccurence();
    }

    // A trip interrupt was generate to the BEG EPWM.
    if( eFuseBuckBoostOcurred == true) {
        handleEFuseBBOccurence();
    }

}



void main(void)
{
    //
    // Initialize device clock and peripherals
    //
    Device_init();

    //
    // Disable pin locks and enable internal pull-ups.
    //

    // The following is already called by Device_Init() as its last step.
    Device_initGPIO();

    GPIO_writePin(LED2, 1);

    //
    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    //
    Interrupt_initModule();

    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    //
    Interrupt_initVectorTable();

    // Initialize timer queue.
    timerq_init();

    //
    // Clear any IPC flags if set already
    //
    IPC_init(IPC_CPU2_L_CPU1_R);


    //
    // Enable Global Interrupt (INTM) and real time interrupt (DBGM)
    //
    EINT;
    ERTM;

    // Initialize CPU2 status area in RAMGS1.
    cpu2_status.num_short_circ = 0;

    //
    // Synchronize both cores
    // IPC_sync(IPC_CPU2_L_CPU1_R, IPC_FLAG11); // Needed when running CPU2 without bootloader
    IPC_sync(IPC_CPU2_L_CPU1_R, IPC_FLAG31);

    Board_init();
    HAL_StopPwmDCDC();
    HAL_StopPwmInrushCurrentLimit();

    PRINT( "DPMU_CPU2 Firmware compilation timestamp= %s %s\r\n", __DATE__, __TIME__ );

    StateMachineInit();
    DCDCConverterInit();
    HAL_StartPwmCounters();

    //    timer_enable();
    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    // Enable sync and clock to PWM
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    /* set CPU2, the other core, as master for source memory */
    MemCfg_setGSRAMMasterSel(MEMCFG_SECT_GS3, MEMCFG_GSRAMMASTER_CPU2);

    InitializeSensorParameters();

    //
    // Loop forever.
    //
    while (1) {
        // Check if there are any incoming CLI test commands from CPU1.
        check_incoming_cli_commands();

        // Check if there are any incoming commands from CPU1.
        check_incoming_commands();

        // Mimic a bottom-half interrupt handler, scheduled by the CPU2 top-half interrupt handlers.
        handle_top_half_interrupts();

        // Handle timer queue.
        timerq_tick();

        /******* check/update for new values sent from IOP *******/

        /******* DC Bus ************/
        dcbus_update_settings();
        dcbus_check();

        /******* Energy Bank *******/
        energy_storage_update_settings();
        energy_storage_check();


        UpdateDebugLog();
        //UpdateDebugLogFake();

        CheckMainStateMachineIsRunning();

    }
}
