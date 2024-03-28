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

/**
 * A debug print macro.
 *
 * !!! MUST NOT BE CALLED FROM INTERRRUPT CONTEXT !!!
 */
#define PRINT(fmt, ...) \
    do { \
        int dbglen = snprintf(ipc_debug_msg, MAX_CPU2_DBG_LEN, fmt, ##__VA_ARGS__); \
        ipc_debug_msg[MAX_CPU2_DBG_LEN] = '\0'; \
        if (IPC_sendCommand(IPC_CPU2_L_CPU1_R, IPC_FLAG_CPU2_DBG, false, IPC_CPU2_PRINT, (uint32_t)&ipc_debug_msg, dbglen)) { \
            IPC_waitForAck(IPC_CPU2_L_CPU1_R, IPC_FLAG_CPU2_DBG); \
        } \
    } while (0)

// Change this later.
extern volatile uint16_t efuse_top_half_flag;
extern char ipc_debug_msg[];

/*
 *  The following functions are defined in cli_cpu2.c
 */
void check_incoming_cli_commands(void);


#endif /* APP_INC_CLI_CPU2_H_ */
