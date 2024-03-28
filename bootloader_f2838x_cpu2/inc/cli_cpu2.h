/*
 * cli_cpu2.h
 *
 *  Created on: 11 juni 2023
 *      Author: us
 */

#ifndef APP_INC_CLI_CPU2_H_
#define APP_INC_CLI_CPU2_H_


#include <stdint.h>
#include <stdio.h>
#include <ipc.h>

/**
 * A debug print macro.
 *
 * !!! MUST NOT BE CALLED FROM INTERRRUPT CONTEXT !!!
 */
#define IPC_FLAG_CPU2_DBG   IPC_FLAG30 // this IPC flag is used when printing from CPU2
#define IPC_MSG_SIZE        128
#define MAX_CPU2_DBG_LEN    128

#define PRINT(fmt, ...) \
    do { \
        int dbglen = snprintf(ipc_debug_msg, MAX_CPU2_DBG_LEN, fmt, ##__VA_ARGS__); \
        ipc_debug_msg[MAX_CPU2_DBG_LEN] = '\0'; \
        if (IPC_sendCommand(IPC_CPU2_L_CPU1_R, IPC_FLAG_CPU2_DBG, false, IPC_CPU2_PRINT, (uint32_t)&ipc_debug_msg, dbglen)) { \
            IPC_waitForAck(IPC_CPU2_L_CPU1_R, IPC_FLAG_CPU2_DBG); \
        } \
    } while (0)

typedef enum
{
    // Maybe these two ids can be useful in some self-test mode?
    IPC_PING,               // get IPC_PONG message
    IPC_PONG,               // response to IPC_PING message

    // Indications from CPU2.
    IPC_SHORT_CIRCUIT,      // short circuit indication message from CPU2
    IPC_CPU2_PRINT,         // CPU2 debug print command

    // CLI test command ids (may be removed later).
    IPC_GET_TICKS,          // get tick timer value from CPU2
    IPC_GET_DATA,           // get some data
    IPC_CLI_CMD,            // CLI command for CPU2

    /* CPU1 -> CPU2 */
    IPC_SWITCHES_QIRS,      // Update state of switch In-Rush circuitry
    IPC_SWITCHES_QLB,       // Update state of switch Load Bus
    IPC_SWITCHES_QSB,       // Update state of switch Share Bus
    IPC_SWITCHES_QINB,      // Update state of switch Input Bus
    IPC_SWITCHES_QIRS_DONE, // Update state of switch In-Rush circuitry

    /* CPU2 -> CPU1 */

} mid_cli_ipc;              // 'mid' stands for Message Id

extern char ipc_debug_msg[];


#endif /* APP_INC_CLI_CPU2_H_ */
