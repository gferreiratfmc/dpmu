/*
 * ipc_cpu1.c
 *
 *  Created on: 21 dec. 2023
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#include <stdbool.h>

#include "cli_cpu1.h"
#include "co_common.h"
#include "device.h"
#include "ipc.h"
#include "main.h"
#include "timer.h"

#pragma RETAIN(bootOffsetCPU2)
#pragma DATA_SECTION(bootOffsetCPU2, "bootOffsetCPU2")
uint32_t bootOffsetCPU2 = 0;

void ipc_sync_comm(uint32_t flag, bool canopen_comm)
{
    int state = 0;
    int state_next = state;
    bool done = false;
    uint32_t lastTimerTicks = 0;

    /* sync with CPU2, IPC_FLAG<flag>
     *
     * still being able to communicate with IOP
     * in case of CPU2 error
     * */
    do {
        switch(state) {
        case 0:
            IPC_setFlagLtoR(IPC_CPU1_L_CPU2_R, flag);
            lastTimerTicks = timer_get_ticks();;
            state_next = 1;
            break;

        case 1:
            if(IPC_isFlagBusyRtoL(IPC_CPU1_L_CPU2_R, flag)) {
                    state_next = 2;
            }
//            else {
//                if( timer_get_ticks() - lastTimerTicks > 100) {
//                    state_next = 10;
//                }
//            }
            break;
        case 2: IPC_ackFlagRtoL(IPC_CPU1_L_CPU2_R, flag);
                    state_next = 3;
                break;
        case 3: if(!IPC_isFlagBusyLtoR(IPC_CPU1_L_CPU2_R, flag))
                    state_next = 4;
                break;
        case 4:
            done = true;
            Serial_debug(DEBUG_INFO, &cli_serial, "ipc_sync_comm() bootOffSetCPU2:: [%lu]\r\n", bootOffsetCPU2 );
            break;

//        case 10:
//            bootOffsetCPU2++;
//            state_next = 0;
//            Device_bootCPU2(BOOTMODE_BOOT_TO_FLASH_SECTOR4);
//            break;

        }



        if(state != state_next) {
            Serial_debug(DEBUG_INFO, &cli_serial, "ipc_sync_comm() state: %d  state_next: %d\r\n", state, state_next);
        }
        state = state_next;

        /* keep communication:
         *  with IOP in case of CPU2 locking error
         *  with UART for possible debugging
         *  with CPU2 in case of "message handling"
         */
        cli_check_for_new_commands_from_UART(); /* commands through UART */
        check_cpu2_dbg();                       /* debug messages from CPU2 */

        /*TODO below while() can not be executed before startup_sequence() is done
         * Add a timer or is watchdog enough ?
         */
        if(canopen_comm)
        {
            while (coCommTask() == CO_TRUE)
                ;
        }

        debugCPU2Flash();
    } while(!done);
}

void IPC_wait_for_ack_comm(IPC_Type_t ipcType, uint32_t flags)
{
    while (IPC_isFlagBusyLtoR(ipcType, flags))
    {
        /* keep communication:
         *  with IOP in case of CPU2 locking error
         *  with UART for possible debugging
         *  with CPU2 in case of "message handling"
         */
        cli_check_for_new_commands_from_UART(); /* commands through UART */
        check_cpu2_dbg(); /* debug messages from CPU2 */
        while (coCommTask() == CO_TRUE)
            ;
    }
}
