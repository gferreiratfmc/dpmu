/*
 * cli.c
 *
 *  Created on: 3 apr. 2022
 *      Author: us
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "common.h"
#include "cli_cpu1.h"
#include "hal.h"
#include "i2c_com.h"
#include "i2c_test.h"
#include "log.h"
#include "main.h"
#include "emifc.h"
#include "ext_flash.h"
#include "shared_variables.h"
#include "timer.h"
#include "temperature_sensor.h"
#include "lfs_api.h"
//#include "../../../dpmu_cpu2/app/inc/switches.h"

struct Cli cli;

#pragma DATA_SECTION(ipc_cli_msg, "MSGRAM_CPU1_TO_CPU2")
static char ipc_cli_msg[64+1];

static inline const char *cli_args(struct Cli *cli)
{
    return cli->_args;
}

static inline int cli_nargs(struct Cli *cli)
{
    return cli->_nargs;
}

static inline bool cli_get_echo(struct Cli *cli)
{
    return cli->_echo;
}

static inline bool cli_set_echo(struct Cli *cli, bool on)
{
    return cli->_echo = on;
}

static void cli_help(void);
static void cli_reset(void);
static void cli_info(void);
static void cli_version(void);
static void cli_sw_reset();
static void cli_echo(void);
static void cli_led(void);
static void cli_ipc(void);
static void cli_set_switch_state(void);
static void cli_toggling_switches(void);
static void cli_error(const char *reason);
static void cli_pwm_start(void);
static void cli_pwm_stop(void);
static void cli_pwm_set_duty_cycle(void);
static void cli_pwm_set_frequency(void);
static void cli_pwm_set_phase(void);
static void cli_test_inrush_limiter(void);
static void cli_ext_flash(void);
static void cli_ext_flash_chip_erase(void);
//static void cli_ext_flash_sector_erase(void);
static void cli_lfs_test(void);
static void cli_debug_level(void);
static void cli_i2c_test(void);
static void cli_i2c_scan(void);
static void cli_tempSensor_test(void);
static void cli_read_temperatures(void);
static void cli_switch_matrix(void);
static void cli_switch_matrix_cont(void);

static void cli_mkfslog(void);
static void cli_ls(void);
static void cli_rm(void);
static void cli_cat(void);
static void cli_create(void);
static void cli_lfs_test2(void);
static void cli_lfs_size(void);

static void cli_write_testlog_debug(void);
static void cli_write_testlog_can(void);

static void cli_cpu2_cmd(void);

static void cli_dma_test_gsram_ext_ram(void);

static void cli_tq_blocking(void);
static void cli_tq_async(void);

static const struct CliCmd cmds[] = {
    {"?",           "",                         &cli_help,                  "show this help message"                        },
    {"help",        "",                         &cli_help,                  "show this help message"                        },
    {"reset",       "",                         &cli_reset,                 "cause a reset of CPU1 after a second"          },
    {"info",        "",                         &cli_info,                  "show firmware info"                            },
    {"version",     "",                         &cli_version,               "show firmware version"                         },
    {"swreset",     "",                         &cli_sw_reset,              "immediate SW reset"                            },
    {"echo",        "[0|1]",                    &cli_echo,                  "set echo mode, 0=off, 1=on"                    },
    {"led",         "[0|1]",                    &cli_led,                   "turn LED on or off"                            },
    {"ipc",         "",                         &cli_ipc,                   "test IPC between CPU cores"                    },
    {"sw",          "[sw] [state]",             &cli_set_switch_state,      "set state of switch"                           },
    {"sw_cont",     "",                         &cli_toggling_switches,     "continuously toggling all switches"            },
    {"pwm_start",   "[chan]",                   &cli_pwm_start,             "turn pwm channel on"                           },
    {"pwm_stop",    "[chan]",                   &cli_pwm_stop,              "turn pwm channel off"                          },
    {"inrush",      "",                         &cli_test_inrush_limiter,   "test inrush limiter"                           },
    {"pwm_phase",   "[channel] [phase]",        &cli_pwm_set_phase,         "set pwm channel phase"                         },
    {"xflash",      "",                         &cli_ext_flash,             "test external flash access"                    },
    {"xflash_erase","",                         &cli_ext_flash_chip_erase,  "erase entire external flash"                   },
    {"dl",          "",                         &cli_debug_level,           "set debug level (0=OFF)"                       },
    {"i2c",         "",                         &cli_i2c_test,              "test i2c devices"                              },
    {"i2c_scan",    "",                         &cli_i2c_scan,              "search for i2c devices"                              },
    {"tempsens",    "",                         &cli_tempSensor_test,       "test temperature sensors"                      },
    {"tempreadall", "",                         &cli_read_temperatures,     "read all temperatures"                         },
    {"",            "",                         NULL,                       ""                                              },
    {"lfs_test",    "",                         &cli_lfs_test,              "tests littlefs"                                },
    {"mkfslog",     "",                         &cli_mkfslog,               "format the log filesystem"                     },
    {"ls",          "",                         &cli_ls,                    "show dir listing"                              },
    {"rm",          "file",                     &cli_rm,                    "remove file"                                   },
    {"cat",         "file",                     &cli_cat,                   "show file contents"                            },
    {"create",      "file",                     &cli_create,                "create new file"                               },
    {"lfs_test2",   "",                         &cli_lfs_test2,             "create log_xxxx and check write/read"          },
    {"lfs_size",    "",                         &cli_lfs_size,              "show size of external flash filesystem"        },
    {"",            "",                         NULL,                       ""                                              },
    {"dma_ext_ram", "startVal turns",           &cli_dma_test_gsram_ext_ram, "DMA for GSRAM0 -> ExtRAM -> GSRAM1, turns < 0 -> run forever"},
    {"tq_blocking", "duration",                 &cli_tq_blocking,           "test timer queue (and priority queue)"         },
    {"tq_async",    "duration",                 &cli_tq_async,              "test timer queue (and priority queue)"         },
    {"",            "",                         NULL,                       ""                                              },
    {"wr_debuglog", "startVal entries",         &cli_write_testlog_debug,   "write test data to debug log"                  },
    {"wr_canlog",   "startVal entries",         &cli_write_testlog_can,     "write test data to can log"                    },
    {"",            "",                         NULL,                       ""                                              },
    {"cpu2",        "subcommand",               &cli_cpu2_cmd,              "forward sub-command to CPU2"                   },
    {"  swmatrix",  "[1..30]",                  &cli_switch_matrix,         "select a energy cell in external energy storage"},
    {"  swmatcont", "[turns]",                  &cli_switch_matrix_cont,    "continually iterate over energy cells in external energy storage (0 -> infinity)"},
    {"",            "",                         NULL,                       ""                                              },
    {"",            "",                         NULL,          "  !!        WARNING - USE THESE AT YOU OWN RISK         !!" },
    {"",            "",                         NULL,          "  !!  PREFEREABLE WITHOUT ANY ENERGY STORAGE CONNECTED  !!" },
    {"pwm_dc",      "[chan] [duty]",            &cli_pwm_set_duty_cycle,    "set pwm channel duty cycle"                    },
    {"pwm_freq",    "[chan] [freq]",            &cli_pwm_set_frequency,     "set pwm channel frequency"                     },
    {NULL,          NULL,                       NULL,                       NULL                                            }
};

/**
 * @brief Show command line help message
 */
static void cli_help(void)
{
    int start_column_for_arguments = 16;
    int start_column_for_description = 32 - start_column_for_arguments;

    Serial_printf(&cli_serial, "\rusage:\r\n");

    for (int i = 0; cmds[i].name != NULL; ++i) {
        const struct CliCmd *cmd = &cmds[i];
        int len;

        if(cmd->name != "") /* check for non empty string */
        {
            /* print command name */
            len = Serial_printf(&cli_serial, "  %s", cmd->name);
            /* Had to use this ugly hack instead of the %.*s format. Seems that printf is non-standard. */
            while (len++ < start_column_for_arguments) {
                Serial_printf(&cli_serial, " ");
            }

            /* print arguments */
            len = Serial_printf(&cli_serial, "%s", cmd->args);
            while (len++ < start_column_for_description) {
                Serial_printf(&cli_serial, " ");
            }
        }

        /* print description */
        Serial_printf(&cli_serial, "%s\r\n", cmd->descr);
    }

    cli_ok();
}

/**
 * @brief Cause a reset of CPU1.
 */
static void cli_reset()
{
    Serial_printf(&cli_serial, "  Resetting CPU1 in one second...\r\n");
    uint32_t now = timer_get_ticks();
    while (timer_get_ticks() - now < PERIOD_1_S);
    SysCtl_resetDevice();
    cli_ok();
}

/**
 * @brief Show firmware info
 */
static void cli_info(void)
{
    Serial_printf(&cli_serial, " DPMU %s\r\n", FW_VERSION);
    cli_ok();
}

/**
 * @brief Show firmware version
 */
static void cli_version(void)
{
    Serial_printf(&cli_serial, " V%s\r\n", FW_VERSION);
    cli_ok();
}

static void cli_sw_reset(void)
{
//    HWREG(SIMRESET) = SYSCTL_SIMRESET_XRSN;
    SysCtl_simulateReset(SYSCTL_CAUSE_XRS);
//    SysCtl_resetDevice();

    Serial_printf(&cli_serial, " SW reset did not work\r\n");
    cli_ok();
}

static void cli_echo(void)
{
    uint32_t val;

    if (sscanf(cli_args(&cli), "%lu", &val) == 1) {
        cli_set_echo(&cli, val != 0);
        cli_ok();
    } else {
        Serial_printf(&cli_serial, " echo is %s\r\n", cli_get_echo(&cli) ? "ON" : "OFF");
        cli_ok();
    }
}

/**
 * @brief Turn LED on or off
 */
static void cli_led(void)
{
    int i;

    if(cli_nargs(&cli) > 1){
        cli_error("Argument error");
        return;
    }

    if (sscanf(cli_args(&cli), "%d", &i) == 1) {
        GPIO_writePin(SPARE_B6, i == 1 ? 0 : 1);
        cli_ok();
    } else {
        cli_error("Missing argument");
    }
}

static void cli_ipc(void)
{
//    uint32_t command = 0;
    uint32_t addr = 0;
    uint32_t data = 0;

    // Send a IPC_PING and expect an IPC_PONG in response.
    Serial_debug(DEBUG_ERROR, cli._serial, "Sending IPC_PING\r\n");
    /*res =*/ IPC_sendCommand(IPC_CPU1_L_CPU2_R, IPC_FLAG_CLI, false, IPC_PING, addr, data);

    // Wait for acknowledgment.
    IPC_waitForAck(IPC_CPU1_L_CPU2_R, IPC_FLAG_CLI);

    // Read response.
    data = IPC_getResponse(IPC_CPU1_L_CPU2_R);
    if (data == IPC_PONG) {
        Serial_debug(DEBUG_ERROR, cli._serial, "Received IPC_PONG\r\n");
        cli_ok();
    } else {
        Serial_debug(DEBUG_ERROR, cli._serial, "Received response: %x\r\n", data);
        cli_error("Bad IPC response");
    }

    // Send a IPC_GET_TICKS and expect a response containing the CPU2 tick counter value.
    Serial_debug(DEBUG_ERROR, cli._serial, "Sending IPC_GET_TICKS\r\n");
    /*res =*/ IPC_sendCommand(IPC_CPU1_L_CPU2_R, IPC_FLAG_CLI, false, IPC_GET_TICKS, addr, data);

    // Wait for acknowledgment.
    IPC_waitForAck(IPC_CPU1_L_CPU2_R, IPC_FLAG_CLI);

    // Read response.
    data = IPC_getResponse(IPC_CPU1_L_CPU2_R);
    Serial_debug(DEBUG_ERROR, cli._serial, "Received tick count: %lu\r\n", data);
    cli_ok();
}

static void cli_set_switch_state_err_msg(void)
{
    Serial_debug(DEBUG_INFO, &cli_serial, " 0 [0|1] turn all switches off\r\n");
    Serial_debug(DEBUG_INFO, &cli_serial, " 1 [0|1] turn switch INRUSH on(run)/off\r\n");
    Serial_debug(DEBUG_INFO, &cli_serial, " 2 [0|1] turn switch LOAD on/off\r\n");
    Serial_debug(DEBUG_INFO, &cli_serial, " 3 [0|1] turn switch SHARE on/off\r\n");
    Serial_debug(DEBUG_INFO, &cli_serial, " 4 [0|1] turn switch INBUS on/off\r\n");
}

static void cli_set_switch_state(void)
{
    int switch_nr;
    int state;

    /* no arguments */
    if(cli_nargs(&cli) < 1){
        cli_set_switch_state_err_msg();
        cli_error("Argument error [none]");
        return;
    }

    if (cli_nargs(&cli) > 2) {
        cli_set_switch_state_err_msg();
        cli_error("Argument error [>2 argument]");
        return;
    }

    /* start pwm channel */
    if (cli_nargs(&cli) == 2 && sscanf(cli_args(&cli), "%d %d", &switch_nr, &state) == 2) {
        switch (switch_nr)
        {
        case 0:
            /* send instruction to CPU2 */
            cli_switches(IPC_SWITCHES_QIRS, SW_OFF);
            cli_switches(IPC_SWITCHES_QLB,  SW_OFF);
            cli_switches(IPC_SWITCHES_QSB,  SW_OFF);
            cli_switches(IPC_SWITCHES_QINB, SW_OFF);
            break;
        case SWITCHES_QIRS+1:
            /* send instruction to CPU2 */
            cli_switches(IPC_SWITCHES_QIRS, state);
            break;
        case SWITCHES_QLB+1:
            /* send instruction to CPU2 */
            cli_switches(IPC_SWITCHES_QLB, state);
            break;
        case SWITCHES_QSB+1:
            /* send instruction to CPU2 */
            cli_switches(IPC_SWITCHES_QSB, state);
            break;
        case SWITCHES_QINB+1:
            /* send instruction to CPU2 */
            cli_switches(IPC_SWITCHES_QINB, state);
            break;
        default:
            cli_set_switch_state_err_msg();
            cli_error("Argument error");
        }

    } else {
        cli_set_switch_state_err_msg();
        cli_error("Argument error [bad switch number]");
    }

    cli_ok();
}

static void cli_toggling_switches(void)
{
//    switches_test_cont_toggling();

    cli_ok();
}

static void cli_pwm_start_err_msg(void)
{
    Serial_debug(DEBUG_INFO, &cli_serial, " 0 Start BEG_1/2 if tripped\r\n");
    Serial_debug(DEBUG_INFO, &cli_serial, " 1 BEG_1/2 140kHz  HAL_DcdcPulseModePwmSetting\r\n");
    Serial_debug(DEBUG_INFO, &cli_serial, " 2 BEG_1/2 140kHz HAL_DcdcNormalModePwmSetting\r\n");
    Serial_debug(DEBUG_INFO, &cli_serial, " 3 run HAL_StartPwmCllcCellDischarge1\r\n");
    Serial_debug(DEBUG_INFO, &cli_serial, " 4 run HAL_StartPwmCllcCellDischarge2\r\n");
    Serial_debug(DEBUG_INFO, &cli_serial, " 5 run HAL_StartPwmInrushCurrentLimit\r\n");
}

static void cli_pwm_start(void)
{
    int channel;

    /* no arguments */
    if(cli_nargs(&cli) < 1) {
        cli_pwm_start_err_msg();
        cli_error("Argument error [none]");
        return;
    }

    if (cli_nargs(&cli) > 1) {
        cli_pwm_start_err_msg();
        cli_error("Argument error [>1 argument]");
        return;
    }

    /* start pwm channel */
    if (cli_nargs(&cli) == 1 && sscanf(cli_args(&cli), "%d", &channel) == 1) {
        switch(channel) {
        case 0: // BEG_1_2
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 11, SYSCTL_CPUSEL_CPU1);
            HAL_StartPwmDCDC();
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 11, SYSCTL_CPUSEL_CPU2);
            break;
        case 1:
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 11, SYSCTL_CPUSEL_CPU1);
            HAL_DcdcPulseModePwmSetting();
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 11, SYSCTL_CPUSEL_CPU2);
            break;
        case 2:
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 11, SYSCTL_CPUSEL_CPU1);
            HAL_DcdcNormalModePwmSetting();
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 11, SYSCTL_CPUSEL_CPU2);
            break;
        case 3: // QABPWM_4_5_BASE - QABPWM_6_7_BASE
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 8, SYSCTL_CPUSEL_CPU1);
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 3, SYSCTL_CPUSEL_CPU1);
            HAL_StartPwmCllcCellDischarge1();
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 8, SYSCTL_CPUSEL_CPU2);
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 3, SYSCTL_CPUSEL_CPU2);
            break;
        case 4: // QABPWM_12_13_BASE - QABPWM_14_15_BASE
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 6, SYSCTL_CPUSEL_CPU1);
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 7, SYSCTL_CPUSEL_CPU1);
            HAL_StartPwmCllcCellDischarge2();
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 6, SYSCTL_CPUSEL_CPU2);
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 7, SYSCTL_CPUSEL_CPU2);
            break;
        case 5: // Inrush
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 12, SYSCTL_CPUSEL_CPU1);
            HAL_StartPwmInrushCurrentLimit();
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 12, SYSCTL_CPUSEL_CPU2);
            break;
        default:
            cli_pwm_start_err_msg();
            cli_error("Argument error [bad channel number]");
            return;
        }
    } else {
        cli_error("Argument error");
    }

    cli_ok();
}

static void cli_pwm_stop_err_msg(void)
{
    Serial_debug(DEBUG_INFO, &cli_serial, " 0 run HAL_StopPwmDCDC\r\n");
    Serial_debug(DEBUG_INFO, &cli_serial, " 1 run HAL_StopPwmDCDC\r\n");
    Serial_debug(DEBUG_INFO, &cli_serial, " 2 run HAL_StopPwmDCDC\r\n");
    Serial_debug(DEBUG_INFO, &cli_serial, " 3 run HAL_StopPwmCllcCellDischarge1\r\n");
    Serial_debug(DEBUG_INFO, &cli_serial, " 4 run HAL_StopPwmCllcCellDischarge2\r\n");
    Serial_debug(DEBUG_INFO, &cli_serial, " 5 run HAL_StopPwmInrushCurrentLimit\r\n");
}

static void cli_pwm_stop(void)
{
    int channel;

    /* no arguments */
    if(cli_nargs(&cli) < 1){
        cli_pwm_stop_err_msg();
        cli_error("Argument error [none]");
        return;
    }

    if (cli_nargs(&cli) > 1) {
        cli_pwm_stop_err_msg();
        cli_error("Argument error [>1 argument]");
        return;
    }

    /* stop pwm channel */
    if (cli_nargs(&cli) == 1 && sscanf(cli_args(&cli), "%d", &channel) == 1) {
        switch(channel) {
        case 0: // BEG_1_2
        case 1:
        case 2:
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 11, SYSCTL_CPUSEL_CPU1);
            HAL_StopPwmDCDC();
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 11, SYSCTL_CPUSEL_CPU2);
            break;
        case 3: // QABPWM_4_5_BASE - QABPWM_6_7_BASE
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 8, SYSCTL_CPUSEL_CPU1);
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 3, SYSCTL_CPUSEL_CPU1);
            HAL_StopPwmCllcCellDischarge1();
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 8, SYSCTL_CPUSEL_CPU2);
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 3, SYSCTL_CPUSEL_CPU2);
            break;
        case 4: // QABPWM_12_13_BASE - QABPWM_14_15_BASE
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 6, SYSCTL_CPUSEL_CPU1);
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 7, SYSCTL_CPUSEL_CPU1);
            HAL_StopPwmCllcCellDischarge2();
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 6, SYSCTL_CPUSEL_CPU2);
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 7, SYSCTL_CPUSEL_CPU2);
            break;
        case 5: // Inrush
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 12, SYSCTL_CPUSEL_CPU1);
            HAL_StopPwmInrushCurrentLimit();
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 12, SYSCTL_CPUSEL_CPU2);
            break;
        default:
            cli_pwm_stop_err_msg();
            cli_error("Argument error [bad channel number]");
            return;
        }
    }

    cli_ok();
}

static void cli_pwm_set_duty_cycle_read_err_msg(void)
{
    Serial_debug(DEBUG_INFO, &cli_serial, "0 read duty cycle of BEG_1_2_BASE EPWM_COUNTER_COMPARE_A\r\n");
//    Serial_debug(DEBUG_INFO, &cli_serial, "1 read duty cycle of BEG_1_2_BASE EPWM_COUNTER_COMPARE_A\r\n");
//    Serial_debug(DEBUG_INFO, &cli_serial, "2 read duty cycle of BEG_1_2_BASE EPWM_COUNTER_COMPARE_A\r\n");
}

static void cli_pwm_set_duty_cycle_err_msg(void)
{
    Serial_debug(DEBUG_INFO, &cli_serial, " 0 <DC> set selected duty cycle, DC %, BEG_1_2_BASE EPWM_COUNTER_COMPARE_A\r\n");
//    Serial_debug(DEBUG_INFO, &cli_serial, " 1 run HAL_DcdcPulseModePwmSetting(),  BEG_1_2_BASE\r\n");
//    Serial_debug(DEBUG_INFO, &cli_serial, " 2 run HAL_DcdcNormalModePwmSetting(), BEG_1_2_BASE\r\n");
}

static void cli_pwm_set_duty_cycle(void)
{
    int channel;
    uint16_t duty_cycle;
    uint16_t time_base_period;
    uint16_t counter_compare;

    /* no arguments */
    if(cli_nargs(&cli) < 1){
        cli_pwm_set_duty_cycle_read_err_msg();
        cli_error("Argument error [none]");
        return;
    }

    if (cli_nargs(&cli) > 2) {
        cli_pwm_set_duty_cycle_read_err_msg();
        cli_error("Argument error [>2 arguments]");
        return;
    }

    /* one argument, channel
     * read and print channel number and duty cycle*/
    if (cli_nargs(&cli) == 1 && sscanf(cli_args(&cli), "%d", &channel) == 1) {
        switch(channel) {
        case 0: // BEG_1_2
//        case 1:
//        case 2:
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 11, SYSCTL_CPUSEL_CPU1);
            time_base_period = HAL_PWM_getTimeBasePeriod(BEG_1_2_BASE);
            counter_compare = HAL_PWM_getCounterCompareValue(BEG_1_2_BASE, EPWM_COUNTER_COMPARE_A);
            duty_cycle = counter_compare * 100. / time_base_period;
            Serial_printf(&cli_serial, "\r\nTime base %d Counter compare: %d Duty cycle %d%%\r\n", time_base_period, counter_compare, duty_cycle);
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 11, SYSCTL_CPUSEL_CPU2);
            break;
        default:
            cli_pwm_set_duty_cycle_read_err_msg();
            cli_error("Argument error [bad channel number]");
            return;
        }
    }

    /* two arguments, channel, duty cycle
     * update duty cycle of channel*/
    if (cli_nargs(&cli) == 2 && sscanf(cli_args(&cli), "%d %d", &channel, &duty_cycle) == 2) {
        switch(channel) {
        case 0: // BEG_1_2 Duty cycle as argument
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 11, SYSCTL_CPUSEL_CPU1);
            time_base_period = HAL_PWM_getTimeBasePeriod(BEG_1_2_BASE);
            counter_compare = ((uint32_t)time_base_period * duty_cycle / 100) & 0xffff;
            HAL_StartPwmDCDC();
            HAL_PWM_setCounterCompareValue(BEG_1_2_BASE, EPWM_COUNTER_COMPARE_A, (uint16_t)( ((uint16_t)counter_compare) & 0xffff) );
            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 11, SYSCTL_CPUSEL_CPU2);
            break;
//        case 1: // BEG_1_2 Pulse mode
//            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 11, SYSCTL_CPUSEL_CPU1);
//            HAL_StartPwmDCDC();
//            HAL_DcdcPulseModePwmSetting();
//            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 11, SYSCTL_CPUSEL_CPU2);
//            break;
//        case 2: // BEG_1_2 Normal mode
//            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 11, SYSCTL_CPUSEL_CPU1);
//            HAL_StartPwmDCDC();
//            HAL_DcdcNormalModePwmSetting();
//            SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL0_EPWM, 11, SYSCTL_CPUSEL_CPU2);
//            break;
        default:
            cli_pwm_set_duty_cycle_err_msg();
            cli_error("Argument error [bad channel number]");
            return;
        }
    }

    cli_ok();
}

static void cli_pwm_set_frequency(void)
{
//    int channel;
//    uint16_t frequency;
//    uint16_t periodCount;
//
//    float present_duty_cycle;
//    uint16_t present_time_base_period;
//    uint16_t new_time_base_period;
//    uint16_t present_counter_compare;
//    uint16_t new_counter_compare;
//
//    /* no arguments */
//    if(cli_nargs(&cli) < 1){
//        cli_error("Argument error [none]");
//        return;
//    }
//
//    if (cli_nargs(&cli) > 2) {
//        cli_error("Argument error [>2 arguments]");
//        return;
//    }
//
//    /* one argument, channel
//     * read and print channel number and frequency */
//    if (cli_nargs(&cli) == 1 && sscanf(cli_args(&cli), "%d", &channel) == 1) {
//        /* calculate periodCount from the frequency
//         * @ 100 MHz */
//
//        /* read present values */
//        present_time_base_period = HAL_PWM_getTimeBasePeriod(BEG_1_2_BASE);
//        present_counter_compare = HAL_PWM_getCounterCompareValue(BEG_1_2_BASE, EPWM_COUNTER_COMPARE_A);
//
//        /* calculate current DC */
//        present_duty_cycle = present_counter_compare / present_time_base_period;
//
//        /* calculate new period reg value */
//        new_time_base_period = frequency / DEVICE_SYSCLK_FREQ;
//
//        /* calculate new DC reg value */
//        new_counter_compare = new_time_base_period * present_duty_cycle;
//
//        /* stop pwm */
//        /* write reg 1 */
//        /* write reg2 */
//        /* start pwm */
//
//
//        switch(channel) {
//        case 0:
//        case 1:
//        case 2:
//            frequency = 0;
//            HAL_PWM_setTimeBasePeriod(BEG_1_2_BASE, periodCount);
//            Serial_printf(&cli_serial, "\r\nPWM channel %d freq: %d\r\n", channel, frequency);
////            cli_ok();
//            break;
//        case 3:
//            frequency = 0;
//            HAL_PWM_setTimeBasePeriod(QABPWM_4_5_BASE, periodCount);
////            HAL_PWM_setTimeBasePeriod(QABPWM_6_7_BASE, periodCount);  // linked
//            Serial_printf(&cli_serial, "\r\nPWM channel %d freq: %d\r\n", channel, frequency);
////            cli_ok();
//            break;
//        case 4:
//            frequency = 0;
//            HAL_PWM_setTimeBasePeriod(QABPWM_12_13_BASE, periodCount);
//            HAL_PWM_setTimeBasePeriod(QABPWM_14_15_BASE, periodCount);
//            Serial_printf(&cli_serial, "\r\nPWM channel %d freq: %d\r\n", channel, frequency);
////            cli_ok();
//            break;
//        case 5:
//            frequency = 0;
//            HAL_PWM_setTimeBasePeriod(InrushCurrentLimit_BASE, periodCount);
//            Serial_printf(&cli_serial, "\r\nPWM channel %d freq: %d\r\n", channel, frequency);
////            cli_ok();
//            break;
//        default: cli_error("Argument error [bad channel number]");
//                 return;
//        }
//    }
//
//    /* two arguments, channel, duty frequency
//     * update frequency of channel */
//    if (cli_nargs(&cli) == 2 && sscanf(cli_args(&cli), "%d %d", &channel, &frequency) == 2) {
//        switch(channel) {
//        case 0: //set PWM channel 0 duty cycle to frequency
////            cli_ok();
//            break;
//        case 1: //set PWM channel 1 duty cycle to frequency
////            cli_ok();
//            break;
//        case 2: //set PWM channel 2 duty cycle to frequency
////            cli_ok();
//            break;
//        default: cli_error("Argument error [bad channel number]");
//                 return;
//        }
//    }
    Serial_debug(DEBUG_ERROR, &cli_serial, "DID NOTHING\r\n");

    cli_ok();
}

static void cli_pwm_set_phase(void)
{
    int channel;
    uint16_t phase;

    /* no arguments */
    if(cli_nargs(&cli) < 1){
        cli_error("Argument error [none]");
        return;
    }

    if (cli_nargs(&cli) > 2) {
        cli_error("Argument error [>2 arguments]");
        return;
    }

    /* one argument, channel Case 0  is CellDischarge Channel 1   Case 1 is CellDischarge Channel 2  Phase shall be within 0 to 180
     * read and print channel number and phase */
    if (cli_nargs(&cli) == 1 && sscanf(cli_args(&cli), "%d", &channel) == 1) {
        switch(channel) {
        case 0:
            phase = 0;
            HAL_PWM_setPhaseShift(QABPWM_6_7_BASE, (int)phase/360*HAL_EPWM_getTimeBasePeriod(QABPWM_6_7_BASE));
            Serial_printf(&cli_serial, "\r\nPWM channel %d freq: %d\r\n", channel, phase);
//            cli_ok();
            break;
        case 1:
            phase = 0;
            HAL_PWM_setPhaseShift(QABPWM_14_15_BASE, (int)phase/360*HAL_EPWM_getTimeBasePeriod(QABPWM_14_15_BASE));
            Serial_printf(&cli_serial, "\r\nPWM channel %d freq: %d\r\n", channel, phase);
//            cli_ok();
            break;

        default: cli_error("Argument error [bad channel number]");
                 return;
        }
    }

    /* two arguments, channel, phase
     * update phase of channel */
    if (cli_nargs(&cli) == 2 && sscanf(cli_args(&cli), "%d %d", &channel, &phase) == 2)
    {
        if(channel >= 0 && channel <= 3){
            HAL_PWM_setPhaseShift(channel, phase);
        } else
        {
            cli_error("Argument error [bad channel number]");
            return;
        }
//        switch(channel) {
//        case 0: HAL_PWM_setPhaseShift(channel, phase);//set PWM channel 0 duty cycle to phase
////            cli_ok();
//            break;
//        case 1: HAL_PWM_setPhaseShift(channel, phase);//set PWM channel 1 duty cycle to phase
////            cli_ok();
//            break;
//        case 2: HAL_PWM_setPhaseShift(channel, phase);//set PWM channel 2 duty cycle to phase
////            cli_ok();
//            break;
//        default: cli_error("Argument error [bad channel number]");
//                 return;
//        }
    }

    cli_ok();
}

static void cli_test_inrush_limiter(void)
{
    // TODO: Move to CPU2.
//    DCDC_softstart();
    cli_switches(IPC_SWITCHES_QIRS, SW_ON);
    cli_switches(IPC_SWITCHES_QIRS, SW_OFF);

    cli_ok();
}

static void cli_ext_flash(void)
{
    ext_flash_test();
    cli_ok();
}

static void cli_ext_flash_chip_erase(void)
{
    ext_flash_chip_erase();
    cli_ok();
}

struct {
    int errcode;
    const char *descr;
} lfs_errdescr[] = {
    { LFS_ERR_OK,           "No error" },
    { LFS_ERR_IO,           "Error during device operation" },
    { LFS_ERR_CORRUPT,      "Corrupted" },
    { LFS_ERR_NOENT,        "No directory entry" },
    { LFS_ERR_EXIST,        "Entry already exists" },
    { LFS_ERR_NOTDIR,       "Entry is not a dir" },
    { LFS_ERR_ISDIR,        "Entry is a dir" },
    { LFS_ERR_NOTEMPTY,     "Dir is not empty" },
    { LFS_ERR_BADF,         "Bad file number" },
    { LFS_ERR_FBIG,         "File too large" },
    { LFS_ERR_INVAL,        "Invalid parameter" },
    { LFS_ERR_NOSPC,        "No space left on device" },
    { LFS_ERR_NOMEM,        "No more memory available" },
    { LFS_ERR_NOATTR,       "No data/attr available" },
    { LFS_ERR_NAMETOOLONG,  "File name too long" }
};

static void cli_lfs_error(int err)
{
    for (int i = 0; i < sizeof(lfs_errdescr) / sizeof(lfs_errdescr[0]); ++i) {
        if (err == lfs_errdescr[i].errcode) {
            cli_error(lfs_errdescr[i].descr);
            return;
        }
    }

    cli_error("Unknown error");
}

static void cli_lfs_test(void)
{
    lfs_api_file_system_test();

    cli_ok();
}

static void cli_mkfslog(void)
{
    int err = lfs_api_format_filesystem();

    if (err) {
        cli_lfs_error(err);
    } else {
        cli_ok();
    }
}

static void cli_ls(void)
{
    int err;

    Serial_printf(&cli_serial, "Directory listing:\r\n");

    do {
        if ((err = lfs_api_mount_filesystem())) {
            cli_lfs_error(err);
            break;
        }

        if ((err = lfs_api_show_directory_contents("/"))) {
            cli_lfs_error(err);
            break;
        }

        lfs_api_unmount_filesystem();
        cli_ok();
    } while (0);
}

static void cli_rm(void)
{
    if (cli_nargs(&cli) == 0) {
        cli_error("Filename missing");
        return;
    }

    int err;

    if ((err = lfs_api_mount_filesystem())) {
        cli_lfs_error(err);
        return;
    }

    do {
        if ((err = lfs_remove(&ext_flash_lfs, cli_args(&cli)))) {
            cli_lfs_error(err);
        }

        lfs_api_unmount_filesystem();
        cli_ok();
    } while (0);
}

static void cli_cat(void)
{
    if (cli_nargs(&cli) == 0) {
        cli_error("Filename missing");
        return;
    }

    cli_ok();
}

static void cli_create(void)
{
    if (cli_nargs(&cli) == 0) {
        cli_error("Filename missing");
        return;
    }

    int err;

    if ((err = lfs_api_mount_filesystem())) {
        cli_lfs_error(err);
        return;
    }

    do {
        lfs_file_t file;

        if ((err = lfs_file_open(&ext_flash_lfs, &file, cli_args(&cli), LFS_O_RDWR | LFS_O_CREAT))) {
            cli_lfs_error(err);
            break;
        }

        lfs_file_close(&ext_flash_lfs, &file);

        lfs_api_unmount_filesystem();
        cli_ok();
    } while (0);
}

static void cli_lfs_test2(void)
{
    lfs_api_file_test_002();
    cli_ok();
}

static void cli_lfs_size(void)
{
    lfs_ssize_t size = lfs_fs_size(&ext_flash_lfs);

    if (size < 0) {
        cli_lfs_error(size);
    } else {
        Serial_printf(&cli_serial, "Number of allocated blocks: %ld bytes\r\n", size);
        cli_ok();
    }
}

static void cli_write_testlog_debug(void)
{
    uint8_t starting_value = 0;
    uint32_t nr_of_log_entries = 10;
    EMIF1_Config emif1_ram_debug_log_read = {EXT_RAM_START_ADDRESS_CS2,
                                             CPU_TYPE_ONE,
                                             sizeof(debug_log_t),
                                             (uint16_t*)message};

    debug_log_t readBack;
    /* temporarily set CPU1 as master of memory section */
    MemCfg_setGSRAMMasterSel(MEMCFG_SECT_GS3, MEMCFG_GSRAMMASTER_CPU1);

    uint8_t nr_of_arguments = cli_nargs(&cli);
    if(nr_of_arguments > 2)
    {
        cli_error("Argument error - Too many arguments");
        return;
    }

    if(1 == nr_of_arguments)
    {
        sscanf(cli_args(&cli), "%x", &starting_value);
    }

    if(2 == nr_of_arguments)
    {
        sscanf(cli_args(&cli), "%x %lx", &starting_value, &nr_of_log_entries);
    }

    for(uint8_t j = 0; j < nr_of_log_entries; j++)
    {
        unsigned char value = (j + starting_value) & 0xff;
        bool status;

        value = ((0xff - value)<<8) | (value);
        sharedVars_cpu2toCpu1.debug_log.counter   = j; //value + i++;
        sharedVars_cpu2toCpu1.debug_log.ISen1    = 10+j; //value + i++;
        sharedVars_cpu2toCpu1.debug_log.ISen2    = 20+j; //value + i++;
        sharedVars_cpu2toCpu1.debug_log.IF_1     = 30+j; //value + i++;
        sharedVars_cpu2toCpu1.debug_log.I_Dab2  = 60+j;  //value + i++;
        sharedVars_cpu2toCpu1.debug_log.I_Dab3  = 70+j;  //value + i++;
        sharedVars_cpu2toCpu1.debug_log.Vbus  = 80+j;    //value + i++;
        sharedVars_cpu2toCpu1.debug_log.VStore  = 90+j;  // i++;
        sharedVars_cpu2toCpu1.debug_log.elapsed_time  = 2000+j;  // i++;
        sharedVars_cpu2toCpu1.debug_log.CurrentState  = j;  // i++;
        status = log_store_debug_log((unsigned char *)&sharedVars_cpu2toCpu1.debug_log);

        /* check if log is active */
        if(false == status)
            return;
    }
    for(uint8_t j = 0; j < nr_of_log_entries; j++)
    {
        memset( &message[0], 0, 14 );
        emif1_ram_debug_log_read.size = sizeof(debug_log_t);
        emifc_cpu_read_memory(&emif1_ram_debug_log_read);

        memcpy( &readBack, message,  sizeof(debug_log_t));
        //for(int i = 0; i < emif1_ram_debug_log_read.size; i++)
        {
            //Serial_debug(DEBUG_ERROR, &cli_serial, "%04x ", message[i]);
            Serial_debug(DEBUG_ERROR, &cli_serial, "counter:[%d] ", readBack.counter);
            Serial_debug(DEBUG_ERROR, &cli_serial, "ISen1:[%d] ", readBack.ISen1);
            Serial_debug(DEBUG_ERROR, &cli_serial, "ISen2:[%d] ", readBack.ISen2);
            Serial_debug(DEBUG_ERROR, &cli_serial, "IF_1:[%d] ", readBack.IF_1);
            Serial_debug(DEBUG_ERROR, &cli_serial, "I_Dab2:[%d] ", readBack.I_Dab2);
            Serial_debug(DEBUG_ERROR, &cli_serial, "I_Dab3:[%d] ", readBack.I_Dab3);
            Serial_debug(DEBUG_ERROR, &cli_serial, "Vbus:[%d] ", readBack.Vbus);
            Serial_debug(DEBUG_ERROR, &cli_serial, "VStore:[%d] ", readBack.VStore);
            Serial_debug(DEBUG_ERROR, &cli_serial, "elapsed_time:[%d] ", readBack.elapsed_time);
        }
        Serial_debug(DEBUG_ERROR, &cli_serial, "\r\n");

        emif1_ram_debug_log_read.address += emif1_ram_debug_log_read.size;
    }

    /* configure as normal - set CPU2 as master of memory section */
    MemCfg_setGSRAMMasterSel(MEMCFG_SECT_GS3, MEMCFG_GSRAMMASTER_CPU2);
}

static void cli_write_testlog_can(void)
{
    uint8_t starting_value = 0;
    uint32_t nr_of_log_entries = 10;
    unsigned char teslog[8];
    //EMIF1_Config emif1_ram_can_log_read = {EXT_RAM_START_ADDRESS_CS2,
    EMIF1_Config emif1_ram_can_log_read = {EXT_FLASH_START_ADDRESS_CS3,
                                             CPU_TYPE_ONE,
                                             6, /* 16 bit words + 2 words for time stamp */
                                             (uint16_t*)message};
//    EMIF1_Config emif1_flash_log_read = {EXT_FLASH_START_ADDRESS_CS3,
//                                         CPU_TYPE_ONE,
//                                         6, /* 16 bit words + 2 words for time stamp */
//                                         (uint16_t*)message};

    /* temporarily set CPU1 as master of memory section */
    MemCfg_setGSRAMMasterSel(MEMCFG_SECT_GS3, MEMCFG_GSRAMMASTER_CPU1);

    uint8_t nr_of_arguments = cli_nargs(&cli);
    if(nr_of_arguments > 2)
    {
        cli_error("Argument error - Too many arguments");
        return;
    }

    if(1 == nr_of_arguments)
    {
        sscanf(cli_args(&cli), "%x", &starting_value);
    }

    if(2 == nr_of_arguments)
    {
        sscanf(cli_args(&cli), "%x %lx", &starting_value, &nr_of_log_entries);
    }

    for(uint8_t j = 0; j < nr_of_log_entries; j++)
    {
        uint8_t i = 0;

        for(; i < sizeof(teslog); i++)
        {
            unsigned char value = (j + i + starting_value) & 0xff;

//            value = ((0xff - value)<<8) | (value);
            value = ((j << 8) & 0xff) | ((value) & 0xff);
            teslog[i]   = value;
        }

        log_store_can_log(sizeof(teslog), teslog);
    }

    for(uint8_t j = 0; j < nr_of_log_entries; j++)
    {
        emif1_ram_can_log_read.size = (sizeof(teslog) / 2) + 2;    /* 16 bit words + 2 words for time stamp */
        emifc_cpu_read_memory(&emif1_ram_can_log_read);

        for(int i = 0; i < emif1_ram_can_log_read.size; i++)
        {
            Serial_debug(DEBUG_ERROR, &cli_serial, "%04x ", message[i]);
        }
        Serial_debug(DEBUG_ERROR, &cli_serial, "\r\n");

        emif1_ram_can_log_read.address += emif1_ram_can_log_read.size;
    }

    /* configure as normal - set CPU2 as master of memory section */
    MemCfg_setGSRAMMasterSel(MEMCFG_SECT_GS3, MEMCFG_GSRAMMASTER_CPU2);
}

static void cli_cpu2_cmd(void)
{
    if (cli_nargs(&cli) == 0) {
        cli_error("Missing sub-command");
        return;
    }

    // Copy sub-command to message buffer.
    strncpy(ipc_cli_msg, cli_args(&cli), 64);

    // Send a sub-command to CPU2.
    if (IPC_sendCommand(IPC_CPU1_L_CPU2_R, IPC_FLAG_CLI, false, IPC_CLI_CMD, (uint32_t)&ipc_cli_msg, 0)) {
        // Wait for acknowledgment.
        IPC_waitForAck(IPC_CPU1_L_CPU2_R, IPC_FLAG_CLI);
    }

    cli_ok();
}

bool cli_switches(uint32_t switchs, bool state)
{
    bool retval = false;
    // Copy sub-command to message buffer.
    strncpy(ipc_cli_msg, (char*)&state, 1);

    IPC_waitForAck(IPC_CPU1_L_CPU2_R, IPC_FLAG_MESSAGE_CPU1_TO_CPU2);

    // Send a sub-command to CPU2.
    if (IPC_sendCommand(IPC_CPU1_L_CPU2_R, IPC_FLAG_MESSAGE_CPU1_TO_CPU2, false, switchs, (uint32_t)&ipc_cli_msg, 1)) {
        // Wait for acknowledgment.
        //TODO BLOCKING -> NON-BLOCKING !!!!
        IPC_waitForAck(IPC_CPU1_L_CPU2_R, IPC_FLAG_MESSAGE_CPU1_TO_CPU2);
        retval = true;
    }

    return retval;
}

bool cli_switches_non_blocking(uint32_t switchs, bool state)
{
    bool retval = false;

    // Copy sub-command to message buffer.
    strncpy(ipc_cli_msg, (char*)&state, 1);

    // Send a sub-command to CPU2.
    if (IPC_sendCommand(IPC_CPU1_L_CPU2_R, IPC_FLAG_MESSAGE_CPU1_TO_CPU2, false, switchs, (uint32_t)&ipc_cli_msg, 1)) {
        retval = true;
    }

    return retval;
}

/* test copy of data using DMA:
 * from shared memory GSRAM0 with CPU2 (the other core) as master of this memory,
 * to the external RAM,
 * and back from external RAM to shared memory GSRAM1
 * */
#pragma DATA_SECTION(cliDmaTestSourceMemory, "ramgs0");  // map the TX data to memory
#pragma DATA_SECTION(cliDmaTestDestinationMemory, "ramgs1");  // map the RX data to memory
uint16_t cliDmaTestSourceMemory[16];
uint16_t cliDmaTestDestinationMemory[16];
static void cli_dma_test_gsram_ext_ram(void)
{
    int result_total = 0;

    EMIF1_Config test_log_emif1_ram_write = {EXT_RAM_START_ADDRESS_CS2,
                                             CPU_TYPE_ONE,
                                             16,
                                             cliDmaTestSourceMemory};
    EMIF1_Config test_log_emif1_ram_read  = {EXT_RAM_START_ADDRESS_CS2,
                                             CPU_TYPE_ONE,
                                             16,
                                             cliDmaTestDestinationMemory};

    uint16_t start_value  = 0;
    int16_t  times_to_run = 1;
    uint16_t freerun      = 0;

    if (cli_nargs(&cli) > 3) {
        cli_error("to many sub-commands");
        return;
    } else {

    }

    /* set EMFI to use the external RAM */
    emifc_pinmux_setup_memory(MEM_TYPE_RAM);

    sscanf(cli_args(&cli), "%d %d %d", &start_value, &times_to_run, &freerun);

    for(int i  = 0; (i < times_to_run) | (times_to_run < 0); i++)
    {
        int result_this_run = 0;

        /* set CPU1 as master for both memories */
        MemCfg_setGSRAMMasterSel(MEMCFG_SECT_GS0, MEMCFG_GSRAMMASTER_CPU1);
        MemCfg_setGSRAMMasterSel(MEMCFG_SECT_GS1, MEMCFG_GSRAMMASTER_CPU1);

        for(uint8_t j = 0; j < 16; j++)
        {
            cliDmaTestSourceMemory[j] = (j + start_value + i) & 0xFF;
            cliDmaTestDestinationMemory[j]  = 0; /* clear destination/read back memory */
        }

        /* set CPU2, the other core, as master for source memory */
        MemCfg_setGSRAMMasterSel(MEMCFG_SECT_GS0, MEMCFG_GSRAMMASTER_CPU2);

        /* copy data GSRAM0 -> external RAM */
        emifc_cpu_write_memory(&test_log_emif1_ram_write);

        /* copy data external RAM -> GSRAM1 */
        emifc_cpu_read_memory(&test_log_emif1_ram_read);

        /* verify the copy of data from GSRAM0 to GSRAM1, done via the external RAM */
        Serial_debug(DEBUG_ERROR, &cli_serial, "Test run %d\r\n", i);
        for(int i = 0; i < 16; i++)
        {
            Serial_debug(DEBUG_ERROR, &cli_serial, "%02X %02X", cliDmaTestSourceMemory[i], cliDmaTestDestinationMemory[i]);

            if(cliDmaTestSourceMemory[i] != cliDmaTestDestinationMemory[i])
            {
                result_total = 1;
                result_this_run = 1;
                Serial_debug(DEBUG_ERROR, &cli_serial, " E");
            }

            Serial_debug(DEBUG_ERROR, &cli_serial, "\r\n");
        }
        Serial_debug(DEBUG_ERROR, &cli_serial, "Test run %d %s\r\n", i, (result_this_run?"NOT OK":"OK"));

//        if(times_to_run >= 0) /* negative -> forever */
//            i++;
    }

    Serial_debug(DEBUG_ERROR, &cli_serial, "\r\nDMA Ext RAM test %s\r\n", (result_total?"NOT OK":"OK"));
    Serial_printf(&cli_serial, "%d\r\n", result_total);
}

static uint32_t blocking_timer_start;
static uint32_t async_timer_start;
static timer_t async_timer;
static int nasync = 0;

static void cli_tq_blocking(void)
{
    static timer_t test_timer;

    uint32_t duration;

    if (cli_nargs(&cli) == 0) {
        cli_error("Argument error");
        return;
    }

    if (sscanf(cli_args(&cli), "%lu", &duration) == 1) {
        Serial_printf(&cli_serial, "\r\nAdding 'Test timer' with duration %lu to timer queue\r\n", duration);

        timer_init(&test_timer, duration, "Test timer", NULL, NULL, false);
        blocking_timer_start = timer_get_ticks();
        timer_start(&test_timer);

        Serial_printf(&cli_serial, "Busy-waiting for 'Test timer' to elapse ...\r\n");
        while (!timer_elapsed(&test_timer)) {
            // Not the right way to do it but - twiddle your thumbs...
            timerq_tick();
        }
        Serial_printf(&cli_serial, "%s elapsed after %lu ms\r\n", test_timer.name, timer_get_ticks() - blocking_timer_start);
        Serial_printf(&cli_serial, "Done!\r\n");
    }

    cli_ok();
}

static void test_timer_callback(timer_t *tqe)
{
    Serial_printf(&cli_serial, "\r\n%s elapsed after %lu ms\r\n", tqe->name, timer_get_ticks() - async_timer_start);

    if (++nasync >= 5) {
        timer_stop(&async_timer);
    }
}

static void cli_tq_async(void)
{
    uint32_t duration;

    if (cli_nargs(&cli) == 0) {
        cli_error("Argument error");
        return;
    }

    if (sscanf(cli_args(&cli), "%lu", &duration) == 1) {
        Serial_printf(&cli_serial, "\r\nAdding 'Test timer' with duration %lu to timer queue\r\n", duration);

        nasync = 0;
        timer_init(&async_timer, duration, "Test timer", test_timer_callback, NULL, true);
        async_timer_start = timer_get_ticks();
        timer_start(&async_timer);
    }

    cli_ok();
}

static void cli_debug_level(void)
{
    int newDebugLevel;

    if (cli_nargs(&cli) > 1) {
        cli_error("Argument error");
        return;
    }

    if (sscanf(cli_args(&cli), "%X", &newDebugLevel) == 1) {
        Serial_set_debug_level(newDebugLevel);
    }

    Serial_printf(&cli_serial, "%02X\r\n", debug_level);

    cli_ok();
}

static void cli_i2c_test(void)
{
    i2c_test_main();

    cli_ok();
}

static void cli_i2c_scan(void)
{
    struct I2CHandle i2c_device;
    int status;

    /* init struct
     * same struct, same base address, do not need to write a new function
     * */
    temperature_sensor_init_individual_sensor(&i2c_device, 0);
    i2c_device.NumOfDataBytes = 0;

    /* search for devices */
    for(int i = 1; i <= 0x7F; i++)
    {
        /* update slave address */
        i2c_device.SlaveAddr = i;

        /* test device */
        status = I2C_MasterTransmitter(&i2c_device);

        if(SUCCESS == status) {
            if(!Serial_debug(DEBUG_ERROR, &cli_serial, "Found device with address 0x%02X\n\r", i2c_device.SlaveAddr))
                Serial_printf(&cli_serial, "0x%02X\r\n", i2c_device.SlaveAddr);
        }

        DEVICE_DELAY_US(300U);
    }

    cli_ok();
}

static void cli_tempSensor_test(void)
{
    int16_t status = SUCCESS;

    if((status = i2c_test_of_temperature_sensors()) != SUCCESS)
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "Error: Temperature sensor test failed\n\r");
    } else
    {
        Serial_debug(DEBUG_INFO, &cli_serial, "OK - Temperature sensor test\n\r");
    }

    /* for machine reading */
    Serial_printf(&cli_serial, "%02X\n\r", status);

    cli_ok();
}

static void cli_read_temperatures(void)
{
    int16_t read_temperature_value;
    int status = SUCCESS;

    /* read temperature sensor measured temperature */
    if(temperature_sensor_read_temperature(TEMPERATURE_SENSOR_BASE, &read_temperature_value))
    {
        status |= 1 << TEMPERATURE_SENSOR_BASE;

        if(!Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - TEMPERATURE_SENSOR_BASE\r\n"))
            Serial_printf(&cli_serial, "%X\r\n", 1);
    } else
    {
        if(!Serial_debug(DEBUG_INFO, &cli_serial, "Read temperature BASE: %02d %cC\r\n", read_temperature_value, 0xB0))
            Serial_printf(&cli_serial, "%02d\r\n", read_temperature_value);
    }

    /* read temperature sensor measured temperature */
    if(temperature_sensor_read_temperature(TEMPERATURE_SENSOR_MAIN, &read_temperature_value))
    {
        status |= 1 << TEMPERATURE_SENSOR_MAIN;

        if(!Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - TEMPERATURE_SENSOR_MAIN\r\n"))
            Serial_printf(&cli_serial, "%X\r\n", 1);
    } else
    {
        if(!Serial_debug(DEBUG_INFO, &cli_serial, "Read temperature MAIN: %02d %cC\r\n", read_temperature_value, 0xB0))
            Serial_printf(&cli_serial, "%02d\r\n", read_temperature_value);
    }

    /* read temperature sensor measured temperature */
    if (temperature_sensor_read_temperature(TEMPERATURE_SENSOR_MEZZANINE, &read_temperature_value))
    {
        status |= 1 << TEMPERATURE_SENSOR_MEZZANINE;

        if(!Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - TEMPERATURE_SENSOR_MEZZANINE\r\n"))
            Serial_printf(&cli_serial, "%X\r\n", 1);
    } else
    {
        if(!Serial_debug(DEBUG_INFO, &cli_serial, "Read temperature MEZZ: %02d %cC\r\n", read_temperature_value, 0xB0))
            Serial_printf(&cli_serial, "%02d\r\n", read_temperature_value);
    }

    /* read temperature sensor measured temperature */
    if (temperature_sensor_read_temperature(TEMPERATURE_SENSOR_PWR_BANK, &read_temperature_value))
    {
        status |= 1 << TEMPERATURE_SENSOR_PWR_BANK;

        if(!Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - TEMPERATURE_SENSOR_PWR_BANK\r\n"))
            Serial_printf(&cli_serial, "%X\r\n", 1);
    } else
    {
        if(!Serial_debug(DEBUG_INFO, &cli_serial, "Read temperature PWRB: %02d %cC\r\n", read_temperature_value, 0xB0))
            Serial_printf(&cli_serial, "%02d\r\n", read_temperature_value);
    }

    Serial_printf(&cli_serial, "0x%02x\r\n", status);

    cli_ok();
}

static void cli_switch_matrix(void)
{
#ifdef CPU2
    uint16_t cellNr;

    /* no arguments */
    if (cli_nargs(&cli) < 1) {
        cli_error("Argument error [none]");
        return;
    }

    if (cli_nargs(&cli) > 1) {
        cli_error("Argument error [>1 arguments]");
        return;
    }

    /* one argument, channel
     * read and print channel number and phase */
    if (cli_nargs(&cli) == 1 && sscanf(cli_args(&cli), "%d", &cellNr) == 1) {
        if (cellNr >= BAT_30) {
            cli_error("Argument error [bad channel number]");
            return;
        } else {
            if (cellNr == BAT_0) {
                switch_matrix_reset();
                Serial_debug(DEBUG_INFO, &cli_serial, "Cell switch matrix reseted");
            } else {
                if (cellNr >= BAT_15_N) /* BAT_15_N is used as negative polarity for cell 16 */
                    cellNr += 1;

                switch_matrix_reset();
                switch_matrix_connect_cell(cellNr);
            }
        }
    }
#endif

    cli_ok();
}

static void cli_switch_matrix_cont(void)
{
#ifdef CPU2
    if (cli_nargs(&cli) > 0) {
        cli_error("Argument error [too many arguments (>0)]");
        return;
    }

    /* one argument, channel
     * read and print channel number and phase */
    while (1) {
        for (int i = BAT_1; i < BAT_30; i++) {
            int cellNr = i;

            switch_matrix_reset();

            if (i >= BAT_15_N) /* BAT_15_N is used as negative polarity for cell 16 */
                cellNr = i + 1;

            /* effective range 1..15 (BAT_1..BAT_15 and 17..30 (BAT_16..BAT_30) */
            switch_matrix_connect_cell(cellNr);

            Serial_debug(DEBUG_INFO, &cli_serial, "Cell %s\r\n", i);

            DEVICE_DELAY_US(250);
        }
    }
#endif
}

/**
 * @brief Respond with ERROR message
 */
static void cli_error(const char *reason)
{
    if (reason != NULL) {
        Serial_printf(cli._serial, " ERROR - %s", reason);
    } else {
        Serial_printf(cli._serial, " ERROR");
    }

    cli_ok();
}

/**
 * @brief Respond with OK message
 */
void cli_ok(void)
{
    Serial_printf(cli._serial, "\r\n->");
}

bool cli_is_ready(void)
{
    return cli._ready;
}

void cli_ctor(struct Serial *ser)
{
    cli._serial = ser;
    PT_INIT(&cli._pt_cmd);
    cli_init();
    cli._echo = true;
}

/**
 * @brief Initialize command struct.
 *
 * @param cmd   pointer to struct to be initialized
 */
void cli_init(void)
{
    memset(cli._buf, 0, sizeof(cli._buf));
    cli._ready = false;   // indicate "no command received yet"
}

/**
 * @brief Command line parser protothread.
 *
 * @return int
 */
int cli_get_cmd_line(void)
{
    static int c;

    PT_BEGIN(&cli._pt_cmd);

    // Initialize command struct on thread start.
    cli_init();

    // This thread loops forever.
    while (true) {
        // Wait until acknowledged by client.
        PT_YIELD_UNTIL(&cli._pt_cmd, !cli._ready);

        // Initialize command struct.
        cli_init();

        // Get first non-space character.
        PT_YIELD_UNTIL(&cli._pt_cmd, (c = Serial_getchar(cli._serial)) != -1 && (!isspace(c) || c == '\r'));

        // Get rest of command line.
        while (true) {
            // Accept alpha-numerics and space.
            if ((isalnum(c) || c == '-' || c == '.' || c == ' ' || c == '_' || c == '\b' || c == '?' || c == '/') && strlen(cli._buf) < (CMDBUFLEN - 1)) {
                if (c == '\b' && strlen(cli._buf) > 0) {
                    cli._buf[strlen(cli._buf) - 1] = '\0';
                    if (cli._echo) {
                        Serial_printf(cli._serial, "\b \b");
                    }
                } else if (c != '\b') {
                    cli._buf[strlen(cli._buf)] = c;
                    cli._buf[strlen(cli._buf) + 1] = '\0';
                    if (cli._echo) {
                        Serial_printf(cli._serial, "%c", c);
                    }
                }
            } else if (c == '\r') {
                // Add NULL char at end and signal "command line ready" to client.
                if (cli._echo) {
                    Serial_printf(cli._serial, "\r\n");
                }
                cli._ready = true;
                // serial.printf("'%s'\n", buf.data());
                break;
            } else if (c == CAN) {
                // cancel requested - start over.
                if (cli._echo) {
                    while (strlen(cli._buf) > 0) {
                        Serial_printf(cli._serial, "\b \b");
                        cli._buf[strlen(cli._buf) - 1] = '\0';
                    }
                }
                break;
            }

            // Get another character.
            PT_YIELD_UNTIL(&cli._pt_cmd, (c = Serial_getchar(cli._serial)) != -1);
        }
    }

    PT_END(&cli._pt_cmd);
}

void cli_check_for_cmd(void)
{
    if (cli_is_ready()) {
        bool match = false;
        for (int i = 0; cmds[i].name != NULL; ++i) {
            const struct CliCmd *cmd = &cmds[i];
            size_t len = strlen(cmd->name);
            if ((len != 0) && (cli._buf[len] == ' ' || strlen(cli._buf) == len) && strncmp(cli._buf, cmd->name, len) == 0) {
                // Found a match.
                match = true;
                // Save start of arguments.
                cli._args = cli._buf + len;
                // Skip spaces.
                while (cli._args && isspace(*cli._args)) {
                    cli._args++;
                }
                // Count number of arguments.
                cli._nargs = 0;
                const char *p = cli._args;
                while (*p) {
                    // Increment argument count.
                    cli._nargs++;
                    // Skip non-spaces.
                    while (*p && !isspace(*p)) {
                        p++;
                    }
                    // Skip spaces.
                    while (*p && isspace(*p)) {
                        p++;
                    }
                }
                // Call command function.
                (cmd->pfunc)();
                // Break out of loop.
                break;
            }
        }

        if (strlen(cli._buf) == 0) {
            cli_ok();
        } else if (!match) {
            cli_error("Unknown command");
        }

        // Re-init command buffer etc.
        cli_init();
    }
}

void cli_check_for_new_commands_from_UART(void)
{
    // Run command line parser thread.
    cli_get_cmd_line();

    // Check if a command line was received.
    cli_check_for_cmd();
}

/**
 * \brief Low-level SCI receive interrupt handler.
 */
__attribute__((ramfunc)) __interrupt void INT_cli_serial_RX_ISR(void)
{
    Serial_rx_isr(&cli_serial);
}

/**
 * \brief Low-level SCI transmit interrupt handler.
 */
__attribute__((ramfunc)) __interrupt void INT_cli_serial_TX_ISR(void)
{
    Serial_tx_isr(&cli_serial);
}
