/*
 * cli.h
 *
 *  Created on: 3 apr. 2022
 *      Author: us
 */

#ifndef CLI_H_
#define CLI_H_

#include <stdint.h>
#include <stdbool.h>

#include "clicmd.h"
#include "serial.h"
//#define LC_INCLUDE "lc-addrlabels.h"
#include "app/pt-1.4/pt.h"

// Firmware version string.
#define FW_VERSION "1.0.0"

#define CMDBUFLEN  128    // length of command line buffer
#define CAN        0x18   // ASCII code for CAN (cancel), also Ctrl-X

struct Cli
{
    struct Serial *_serial;         // the serial device used for i/o
    char _buf[CMDBUFLEN+1];
    char *_args;                    // pointer to command arguments
    int _nargs;                     // number of arguments
    bool _echo;                     // true if echo enabled
    bool _ready;                    // true if command line received (seen RETURN char)
    struct pt _pt_cmd;              // protothread struct for command line parser
};

extern struct Cli cli;

void cli_ok(void);
bool cli_is_ready(void);
void cli_ctor(struct Serial *ser);
void cli_init(void);
bool cli_switches(uint32_t switchs, bool state);
bool cli_switches_non_blocking(uint32_t switchs, bool state);
void cli_check_for_new_commands_from_UART(void);

__attribute__((ramfunc)) __interrupt void INT_cli_serial_RX_ISR(void);
__attribute__((ramfunc)) __interrupt void INT_cli_serial_TX_ISR(void);

extern struct Serial cli_serial;

#endif /* CLI_H_ */
