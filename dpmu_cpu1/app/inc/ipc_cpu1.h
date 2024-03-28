/*
 * ipc_cpu1.h
 *
 *  Created on: 21 dec. 2023
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#ifndef APP_INC_IPC_CPU1_H_
#define APP_INC_IPC_CPU1_H_

/* sync with CPU2, IPC_FLAG31
 *
 * still being able to communicate with IOP
 * in case of CPU2 error
 * */
void ipc_sync_comm(uint32_t flag, bool canopen_comm);
void IPC_wait_for_ack_comm(IPC_Type_t ipcType, uint32_t flags);


#endif /* APP_INC_IPC_CPU1_H_ */
