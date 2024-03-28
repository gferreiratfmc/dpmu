/*
 * common.h
 *
 *  Created on: 13 dec. 2022
 *      Author: us
 */

#ifndef COMMON_INC_COMMON_H_
#define COMMON_INC_COMMON_H_


#include "driverlib.h"
#include "device.h"
//#include "board.h"

#include "GlobalV.h"
#include "shared_variables.h"

// Any core specific includes goes here.
#if defined(CPU1)
#elif defined(CPU2)
#else
#error "No target CPU defined!"
#endif

#undef USE_IPC_MESSAGE_QUEUES

#define ACCEPTED_MARGIN_ON_DC_BUS  2

/*
 * Defs for IPC between CPU1 & CPU2.
 */
#define IPC_FLAG_MESSAGE_CPU1_TO_CPU2        IPC_FLAG0  // this is the normal IPC flag used between CPU1 and CPU2
#define IPC_FLAG_CLI                         IPC_FLAG1  // this IPC flag is used for tests issued via CLI
#define IPC_CPU1_REQUIERS_EMERGECY_SHUT_DOWN IPC_FLAG2  // request from CPU1 or IOP for emergency shutdown

    /* CPU2 -> CPU1 */
#define IPC_NEW_CONSTANT_CURRENT_CHARGE_DONE IPC_FLAG8  // signal to CPU1 it is time to calculate SoH
#define IPC_NEW_ENERGY_CELLS_VOLTAGE         IPC_FLAG9  // signal to CPU1 it is time to calculate SoC
#define IPC_IOP_REQUEST_CHANGE_OF_STATE      IPC_FLAG10 // Request from CPU1 or IOP to change state in state machine, controlling algorithm
#define IPC_FLAG_CPU2_DBG                    IPC_FLAG30 // this IPC flag is used when printing from CPU2

#define IPC_CANB_OUTGOING                    IPC_FLAG16
#define IPC_CANB_INCOMING                    IPC_FLAG17

#define IPC_MSG_SIZE        128
#define MAX_CPU2_DBG_LEN    128

#define MAX(a,b) \
    ({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b; })

/*
 * define CANB commands
 */

typedef enum{
    /* set parameters - incoming and reply */
    CANB_TARGET_VOLTAGE_SET,    // outgoing and reply
    CANB_TARGET_VOLTAGE_READ,   // outgoing and reply
    CANB_VDROOP_SET,            // outgoing and reply
    CANB_VDROOP_READ,           // outgoing and reply
    CANB_VDROOP_INCREASE,       // outgoing and reply
    CANB_VDROOP_DECREASE,       // outgoing and reply


    /* read parameters - outgoing and reply */
    CANB_READ_BUS_VOLTAGE_READ, // outgoing and reply

    /* error handling */
    CANB_EMERGENCY_TURN_OFF,                // outgoing and incoming, no reply
    CANB_SHORT_CIRCUIT,                     // outgoing (detected), no reply
    CANB_OVER_CURRENT,                      // outgoing (detected), no reply
    CANB_DISCONNECT_SHARED_BUS,             // outgoing and reply
    CANB_CONNECT_SHARED_BUS,                // outgoing and reply
    CANB_DISCONNECT_SHARED_BUS_INCOMING,    // incoming and reply
    CANB_CONNECT_SHARED_BUS_INCOMING,       // incoming and reply
    CANB_ASK_FOR_PWR_BUDGET,                // request info from other DPMU
    CANB_PWR_BUDGET_IS,                     // sent from DPMU without being requested
} msg_canb_t;

/*
 * define commands for IPC messages
 */
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

typedef union {
    // Raw CAN PDU.
    struct {
        uint16_t raw[8];
    };

    // Sample CAN command PDU.
    struct {
        uint16_t   address;
        msg_canb_t cmd;
        uint16_t   data[7];
    } send;

    // Sample CAN response PDU.
    struct {
        uint16_t   address;
        msg_canb_t rsp;
        uint16_t   data[7];
    } recv;
} canb_msg_t;

typedef enum switch_states { \
    SW_OFF = 0, \
    SW_ON \
} switch_states_t;

typedef enum switches {
    SWITCHES_QIRS = 0,  // Switch Load Bus
    SWITCHES_QLB,       // Switch Load Bus
    SWITCHES_QSB,       // Switch Share Bus
    SWITCHES_QINB,      // Switch Input Bus
} switches_t;

typedef uint16_t mid_t;

typedef struct
{
} msg_ping_t;

typedef struct
{
} msg_pong_t;

typedef struct
{
    uint32_t val;
} msg_ticks_t;

typedef struct
{
} msg_data_t;

typedef struct
{
    mid_t id;
    union
    {
        uint16_t raw[IPC_MSG_SIZE]; // make room for any size message
        msg_ping_t ping;
        msg_pong_t pong;
        msg_ticks_t ticks;
        msg_data_t data;
    };
} msg_t;

typedef union
{
    char str[128];
} cpu1_command_t;

typedef struct
{
//    ADC_Raw AdcRawValue;
    volatile uint32_t num_short_circ;
} cpu2_status_t;

// Command structure managed by CPU1.
extern cpu1_command_t cpu1_command;

// Status structure managed by CPU2.
extern cpu2_status_t cpu2_status;


#endif /* COMMON_INC_COMMON_H_ */
