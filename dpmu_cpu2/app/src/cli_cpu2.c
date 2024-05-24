/*
 * cli_cpu2.c
 *
 *  Created on: 11 juni 2023
 *      Author: us
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "cli_cpu2.h"
#include "CLLC.h"
#include "energy_storage.h"
#include "hal.h"
#include "sensors.h"
#include "switch_matrix.h"
#include "timer.h"

// Command structure managed by CPU1.
#pragma DATA_SECTION(cpu1_command, "ramgs0")
cpu1_command_t cpu1_command;

// Status structure managed by CPU2.
#pragma DATA_SECTION(cpu2_status, "ramgs4")
cpu2_status_t cpu2_status;

// Buffer used to send CPU2 debug messages to CPU1 for sending to the serial port.
#pragma DATA_SECTION(ipc_debug_msg, "MSGRAM_CPU2_TO_CPU1")
char ipc_debug_msg[MAX_CPU2_DBG_LEN + 1];


static void cli_switch_matrix(char *buf);
static void cli_switch_matrix_cont(char *buf);
static void cli_measure_zero_current_matrix_switch(char *buf);
static void cli_measure_cell_current(char *buf);
static void cli_save_soh(char *buf);

void count_number_of_arguments(const char *buf);

static int nargs = 0;

static inline int cli_nargs(void)
{
    return nargs;
}

/**
 * @brief Respond with OK message
 */
void cli_ok(void)
{
    PRINT("\r\n->");
}

/**
 * Called by super loop to handle any incoming test commands from CPU1/cli.c.
 */
void check_incoming_cli_commands(void)
{
    uint32_t command;
    uint32_t addr;
    uint32_t data;

    // Check for CLI test commands.
    if (IPC_readCommand(IPC_CPU2_L_CPU1_R, IPC_FLAG_CLI, false, &command, &addr, &data) != false) {
        if (command == IPC_CLI_CMD) {
            // Special handling for 'cpu2' sub-commands from the CLI. We need to start with the Ack.
            IPC_ackFlagRtoL(IPC_CPU2_L_CPU1_R, IPC_FLAG_CLI);

            // Act on sub-command here.
            char *subcmd = (char *)addr;
            if (strcmp(subcmd, "?") == 0) {
                // Show help message for cpu2 sub-commands.
                PRINT("Sub-commands:\r\n");
                PRINT("  sc              simulate short-circuit immediately\r\n");
                PRINT("  gs              get state (of something)\r\n");
                PRINT("swmatrix  [1..30] select a energy cell in external energy storage\r\n");
                PRINT("swmatcont [turns] continually iterate over energy cells in external energy storage (0 -> infinity)\r\n");
                PRINT("measZeroI [turns] measure zero current offset for matrix switch (0 -> infinity)\r\n");
                PRINT("measCellI [turns] measure cell current (0 -> infinity)\r\n");
                PRINT("soh tests write/read SOH to/from external flash\r\n");
            } else if (strcmp(subcmd, "sc") == 0) {
                efuse_top_half_flag = 1;
            } else if (strcmp(subcmd, "gs") == 0) {
                PRINT(" Nothing here yet\r\n");
            } else if (strcmp(subcmd, "swmatrix") >= 0) {
                cli_switch_matrix(subcmd);
            } else if (strcmp(subcmd, "swmatcont") >= 0) {
                cli_switch_matrix_cont(subcmd);
            } else if (strcmp(subcmd, "swmatcont") >= 0) {
                cli_measure_zero_current_matrix_switch(subcmd);
            } else if (strcmp(subcmd, "swmatcont") >= 0) {
                cli_measure_cell_current(subcmd);
            } else if (strcmp(subcmd, "soh") >= 0) {
                cli_save_soh(subcmd);
            } else {
                PRINT(" ERROR - Unknown sub-command\r\n");
            }

            cli_ok();
        } else {
            // CLI test commands sent by the CLI 'ipc' command.
            switch (command) {
            case IPC_PING:
                IPC_sendResponse(IPC_CPU2_L_CPU1_R, IPC_PONG);
                break;

            case IPC_GET_TICKS:
                IPC_sendResponse(IPC_CPU2_L_CPU1_R, timer_get_ticks());
                break;

            default:
                break;
            }

            // Acknowledge the flag.
            IPC_ackFlagRtoL(IPC_CPU2_L_CPU1_R, IPC_FLAG_CLI);
        }
    }
}

static void cli_switch_matrix(char *buf)
{
    uint16_t cellNr;
    float cellValue = 0.0;
    float cellValueAvg = 0.0;
    char str[20];

    count_number_of_arguments(buf);

    /* no arguments */
    if (cli_nargs() < 2) {
        PRINT("Argument error [none]");
        return;
    }

    if (cli_nargs() > 2) {
        PRINT("Argument error [>1 arguments]");
        return;
    }

    /* one argument, channel
     * read and print channel number and phase */
    if (cli_nargs() == 2 && sscanf(buf, "%s %d", str, &cellNr) == 2) {
        if (cellNr >= BAT_30) {
            PRINT("Argument error [bad channel number]");
            return;
        } else {
            if (cellNr == BAT_0) {
                switch_matrix_reset();
                PRINT("Cell switch matrix reseted");
            } else {
//                if (cellNr >= BAT_15_N) /* BAT_15_N is used as negative polarity for cell 16 */
//                    cellNr += 1;

                PRINT("Cell %02d  ", cellNr);

//                switch_matrix_reset();
                switch_matrix_connect_cell(cellNr);

                DEVICE_DELAY_US(10000);
                for( int i=0; i<10; i++) {
                    if( cellNr <= BAT_15) {
                        cellValue = sensorVector[V_DwnfIdx].realValue;
                    } else {
                        cellValue = sensorVector[V_UpfIdx].realValue;
                    }
                    if( (cellNr & 1) == 1) {
                        cellValueAvg = cellValueAvg - cellValue;
                    } else {
                        cellValueAvg = cellValueAvg + cellValue;
                    }
                    DEVICE_DELAY_US(1000);
                }
                cellValueAvg = cellValueAvg / 10;

                PRINT("Voltage Cell[%02d] = [%6.3f] ",  cellNr , cellValueAvg);
            }
        }
    }
}

static void cli_switch_matrix_cont(char *buf)
{
    int turns_left = 1;
    int turns = 0;
    char str[20];
    int infinit = 0;

    count_number_of_arguments(buf);

    if (cli_nargs() > 2) {
        PRINT("Argument error [too many arguments (>0)]");
        return;
    }

    if (cli_nargs() == 2) {
        if(sscanf(buf, "%s %d", str, &turns_left) == 2);
        if(0 == turns_left)
            infinit = 1;
    }

    /* one argument, channel
     * read and print channel number and phase */
    while (turns_left-- || infinit) {
        PRINT("\r\nTurn: %d\r\n", ++turns);

        for (int i = BAT_1; i < BAT_30; i++) {
            int cellNr = i;

            switch_matrix_reset();

//            if (i >= BAT_15_N) /* BAT_15_N is used as negative polarity for cell 16 */
//                cellNr = i + 1; /* some magic for upper half of matrix */

            PRINT("Cell %02d  ", i);

            /* effective range 1..15 (BAT_1..BAT_15 and 17..30 (BAT_16..BAT_30) */
            switch_matrix_connect_cell(cellNr);

            DEVICE_DELAY_US(1000000);

        }

        /* leave test function in reset state, known state */
        switch_matrix_reset();
    }
    PRINT("\r\nTurns done: %d\r\n", turns);
}

/* measures the zero current from the matrix switch, cell level
 *
 * disconnects all cells, delay and then measure the zero current for both low
 * and high side
 *
 * parameter
 *      the sub command and arguments
 *
 * returns
 *      none
 *
 * assumptions:
 *      none
 * */
void cli_measure_zero_current_matrix_switch(char *buf)
{
    int turns_left = 1;
    int turns = 0;
    char str[20];
    int infinit = 0;

    count_number_of_arguments(buf);

    if (cli_nargs() > 2) {
        PRINT("Argument error [too many arguments (>0)]");
        return;
    }

    if (cli_nargs() == 2) {
        if(sscanf(buf, "%s %d", str, &turns_left) == 2);
        if(0 == turns_left)
            infinit = 1;
    }

    /* one argument, channel
     * read and print channel number and phase */
    while (turns_left-- || infinit) {
        PRINT("\r\nTurn: %d\r\n", ++turns);

        /* reset cell matrix switch so no current can run from the cells */
        switch_matrix_reset();

        DEVICE_DELAY_US(1000000);

        /* execute measurement */
        CLLC_VI.CurrentOffset1 = HAL_ADC_readResult(ADCDRESULT_BASE, ADC_SOC_NUMBER0);
        CLLC_VI.CurrentOffset2 = HAL_ADC_readResult(ADCDRESULT_BASE, ADC_SOC_NUMBER1);

        PRINT("CLLC_VI.CurrentOffset1 %04d  ",     CLLC_VI.CurrentOffset1);
        PRINT("CLLC_VI.CurrentOffset2 %04d  \r\n", CLLC_VI.CurrentOffset2);
    }
    PRINT("\r\nTurns done: %d\r\n", turns);
}

/* measures the current from the matrix switch, cell level
 *
 * parameter
 *      the sub command and arguments
 *
 * returns
 *      none
 *
 * assumptions:
 *      at least one cell has selected been connected
 * */
void cli_measure_cell_current(char *buf)
{
    int turns_left = 1;
    int turns = 0;
    char str[20];
    int infinit = 0;

    count_number_of_arguments(buf);

    if (cli_nargs() > 2) {
        PRINT("Argument error [too many arguments (>0)]");
        return;
    }

    if (cli_nargs() == 2) {
        if(sscanf(buf, "%s %d", str, &turns_left) == 2);
        if(0 == turns_left)
            infinit = 1;
    }

    /* one argument, channel
     * read and print channel number and phase */
    while (turns_left-- || infinit) {
        PRINT("\r\nTurn: %d\r\n", ++turns);

        DEVICE_DELAY_US(1000000);

        /* execute measurement */
        CLLC_VI.IDAB2_RAW = convert_current(HAL_ADC_readResult(ADCDRESULT_BASE,
                                                               ADC_SOC_NUMBER0),
                                            1,
                                            CLLC_VI.CurrentOffset1);
        CLLC_VI.IDAB3_RAW = convert_current(HAL_ADC_readResult(ADCDRESULT_BASE,
                                                               ADC_SOC_NUMBER1),
                                            1,
                                            CLLC_VI.CurrentOffset2);

        PRINT("CLLC_VI.IDAB2_RAW %04d  ",     CLLC_VI.IDAB2_RAW);
        PRINT("CLLC_VI.IDAB3_RAW %04d  \r\n", CLLC_VI.IDAB3_RAW);
    }
    PRINT("\r\nTurns done: %d\r\n", turns);
}

void cli_save_soh(char *buffer){

    float initialCapacitance;
    float currentCapacitance;

    for(int i=0;i<10;i++) {
        PRINT("\r\n========== TEST CAPACITANCES W/R EXTERNAL FLASH:[%d]\r\n",i);


        initialCapacitance = i*10.0;
        currentCapacitance = i*9.5;
        PRINT("=== Write initialCapacitance:[%8.2f]\r\n", initialCapacitance );
        PRINT("=== Write currentCapacitance:[%8.2f]\r\n", currentCapacitance );
        startSaveCapacitance = true;
        while( requestCPU1ToSaveCapacitancesToFlash( initialCapacitance, currentCapacitance) != true);

        DEVICE_DELAY_US( 10000 );
        PRINT("=== Read initialCapacitance:[%8.2f]\r\n", sharedVars_cpu1toCpu2.initialCapacitance );
    }
}

void count_number_of_arguments(const char *buf)
{
    nargs = 0;

    while (*buf) {
        // Increment argument count.
        nargs++;
        // Skip non-spaces.
        while (*buf && !isspace(*buf)) {
            buf++;
        }
        // Skip spaces.
        while (*buf && isspace(*buf)) {
            buf++;
        }
    }
}
