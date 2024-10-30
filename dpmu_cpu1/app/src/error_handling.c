/*
 * error_codes.c
 *
 *  Created on: 15 aug. 2023
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#include <stdbool.h>

#include "canopen_emcy.h"
#include "cli_cpu1.h"
#include "common.h"
#include "convert.h"
#include "error_handling.h"
#include "gen_indices.h"
#include "hal.h"
#include "ipc.h"
#include "payload_gen.h"
#include "shared_variables.h"
#include "startup_sequence.h"
#include "temperature_sensor.h"
#include "timer.h"
#include "usr_401.h"


uint32_t global_error_code         = 0;
uint32_t error_code_CPU1           = 0;
static uint32_t global_error_code_handled = 0;
static uint32_t global_error_code_sent    = 0;


/* brief: call static functions to check if there are errors to signal
 *        and handle
 *
 * details:
 *
 * requirements:
 *
 * argument: none
 *
 * return: none
 *
 * note: some of the errors might have been signaled and handle elsewhere,
 *       e.g. temperature error/warning
 *       non-blocking
 *
 * presumptions:
 *
 */
void error_check_for_errors(void)
{
    static bool test_update_of_error_codes = true;
    if(test_update_of_error_codes)
    {
        uint32_t code;
        code = 1ul << ERROR_EXT_PWR_LOSS_OTHER;
        global_error_code = code;

        error_copy_error_codes_from_CPU1_and_CPU2();
        error_load_overcurrent();
        error_dcbus_short_circuit();
        error_boost_short_circuit();
        error_dcbus_over_voltage();
        error_dcbus_under_voltage();
        error_system_temperature();
        error_no_error();
    }
}


/* brief: check if DC bus Voltage level is above max
 * details: checks error flag
 *          signal IOP with EMCY
 */
static void error_dcbus_over_voltage(void) {
    if(global_error_code & (1UL << ERROR_BUS_OVER_VOLTAGE))
    {
        if(!(global_error_code_handled & (1UL << ERROR_BUS_OVER_VOLTAGE)))
        {
            /* mark handled */
            global_error_code_handled |= (1UL << ERROR_BUS_OVER_VOLTAGE);

            /* send EMCY - send it once */
            canopen_emcy_send_dcbus_over_voltage(1);

            /* mark it sent */
            global_error_code_sent    |= (1UL << ERROR_BUS_OVER_VOLTAGE);
            global_error_code_handled |= (1UL << ERROR_BUS_OVER_VOLTAGE);

            Serial_printf( &cli_serial, "************* Load over voltage detected\r\n");
        }
    } else
    {
        /* send EMCY CLEAR - send it once */
        if(global_error_code_sent & (1 << ERROR_BUS_OVER_VOLTAGE)) {
            canopen_emcy_send_dcbus_over_voltage(0);
            Serial_printf( &cli_serial, "************* Load over voltage clear\r\n");
        }

        /* mark as unhandled - nothing need to be done  */
        global_error_code_sent    &= ~(1UL << ERROR_BUS_OVER_VOLTAGE);
        global_error_code_handled &= ~(1UL << ERROR_BUS_OVER_VOLTAGE);
    }
}


/* brief: check if DC bus Voltage level is below min
 *
 * details: checks error flag
 *          signal IOP with EMCY
 *
 * note: non-blocking
 */
static void error_dcbus_under_voltage(void) {

    if(global_error_code & (1UL << ERROR_BUS_UNDER_VOLTAGE))
    {
        if(!(global_error_code_handled & (1UL << ERROR_BUS_UNDER_VOLTAGE)))
        {
            /* mark handled */
            global_error_code_handled |= (1UL << ERROR_BUS_UNDER_VOLTAGE);

            /* send EMCY - send it once */
            canopen_emcy_send_dcbus_under_voltage(1);

            /* mark it sent */
            global_error_code_sent    |= (1UL << ERROR_BUS_UNDER_VOLTAGE);
            global_error_code_handled |= (1UL << ERROR_BUS_UNDER_VOLTAGE);

            Serial_printf( &cli_serial, "************* DC Bus under voltage detected\r\n");
        }
    } else
    {
        /* send EMCY CLEAR - send it once */
        if(global_error_code_sent & (1 << ERROR_BUS_UNDER_VOLTAGE)) {
            canopen_emcy_send_dcbus_under_voltage(0);
            Serial_printf( &cli_serial, "************* DC Bus under voltage clear\r\n");
        }

        /* mark as unhandled - nothing need to be done  */
        global_error_code_sent    &= ~(1UL << ERROR_BUS_UNDER_VOLTAGE);
        global_error_code_handled &= ~(1UL << ERROR_BUS_UNDER_VOLTAGE);
    }
}


/* brief: check if Current output to load is above max
 *
 * details: checks error flag
 *          signal IOP with EMCY
 *
 * requirements:
 *
 * argument: none
 *
 * return: none
 *
 * note: non-blocking
 *
 * presumptions:
 *
 */
static void error_load_overcurrent(void)
{
    if(global_error_code & (1UL << ERROR_LOAD_OVER_CURRENT))
    {
        if(!(global_error_code_handled & (1UL << ERROR_LOAD_OVER_CURRENT)))
        {
            /* mark handled */
            global_error_code_handled |= (1UL << ERROR_LOAD_OVER_CURRENT);

            /* send EMCY - send it once */
            canopen_emcy_send_load_overcurrent(1);

            /* mark it sent */
            global_error_code_sent    |= (1UL << ERROR_LOAD_OVER_CURRENT);
            global_error_code_handled |= (1UL << ERROR_LOAD_OVER_CURRENT);

            Serial_printf( &cli_serial, "************* Load over current detected\r\n");
        }
    } else
    {
        /* send EMCY CLEAR - send it once */
        if(global_error_code_sent & (1 << ERROR_LOAD_OVER_CURRENT)) {
            canopen_emcy_send_load_overcurrent(0);
            Serial_printf( &cli_serial, "************* Load over current clear\r\n");
        }

        /* mark as unhandled - nothing need to be done  */
        global_error_code_sent    &= ~(1UL << ERROR_LOAD_OVER_CURRENT);
        global_error_code_handled &= ~(1UL << ERROR_LOAD_OVER_CURRENT);
    }
}



static void error_dcbus_short_circuit(void)
{
    if(global_error_code & (1UL << ERROR_BUS_SHORT_CIRCUIT))
    {
        if(!(global_error_code_handled & (1UL << ERROR_BUS_SHORT_CIRCUIT)))
        {
            /* mark handled */
            global_error_code_handled |= (1UL << ERROR_BUS_SHORT_CIRCUIT);

            /* send EMCY - send it once */
            canopen_emcy_send_dcbus_short_curcuit(1);

            /* mark it sent */
            global_error_code_sent    |= (1UL << ERROR_BUS_SHORT_CIRCUIT);
            global_error_code_handled |= (1UL << ERROR_BUS_SHORT_CIRCUIT);

            Serial_printf( &cli_serial, "************* DC BUS short circuit detected\r\n");
        }
    } else
    {
        /* send EMCY CLEAR - send it once */
        if(global_error_code_sent & (1 << ERROR_BUS_SHORT_CIRCUIT)) {
            canopen_emcy_send_dcbus_short_curcuit(0);
            Serial_printf( &cli_serial, "************* DC BUS short circuit clear\r\n");
        }

        /* mark as unhandled - nothing need to be done  */
        global_error_code_sent    &= ~(1UL << ERROR_BUS_SHORT_CIRCUIT);
        global_error_code_handled &= ~(1UL << ERROR_BUS_SHORT_CIRCUIT);
    }
}


static void error_boost_short_circuit(void)
{
    if(global_error_code & (1UL << ERROR_DISCHARGING))
    {
        if(!(global_error_code_handled & (1UL << ERROR_DISCHARGING)))
        {
            /* mark handled */
            global_error_code_handled |= (1UL << ERROR_DISCHARGING);

            /* send EMCY - send it once */
            canopen_emcy_send_boost_short_circuit(1);

            /* mark it sent */
            global_error_code_sent    |= (1UL << ERROR_DISCHARGING);
            global_error_code_handled |= (1UL << ERROR_DISCHARGING);

            Serial_printf( &cli_serial, "************* BOOST short circuit detected\r\n");
        }
    } else
    {
        /* send EMCY CLEAR - send it once */
        if(global_error_code_sent & (1 << ERROR_DISCHARGING)) {
            canopen_emcy_send_boost_short_circuit(0);
            Serial_printf( &cli_serial, "************* BOOST short circuit clear\r\n");
        }

        /* mark as unhandled - nothing need to be done  */
        global_error_code_sent    &= ~(1UL << ERROR_DISCHARGING);
        global_error_code_handled &= ~(1UL << ERROR_DISCHARGING);
    }
}


/* brief: check if temperatures are above threshold
 *
 * details: checks error flag
 *
 */
static void error_system_temperature(void)
{
    if(global_error_code & (1UL << ERROR_OVER_TEMPERATURE))
    {

        if(!(global_error_code_handled & (1UL << ERROR_OVER_TEMPERATURE)))
        {
            /* mark handled */
            global_error_code_handled |= (1UL << ERROR_OVER_TEMPERATURE);

            /* send EMCY - send it once */
            canopen_emcy_send_temperature_error(temperatureHotPoint);
            sharedVars_cpu1toCpu2.temperatureMaxLimitReachedFlag = true;
            /* mark it sent */
            global_error_code_sent    |= (1UL << ERROR_OVER_TEMPERATURE);
            global_error_code_handled |= (1UL << ERROR_OVER_TEMPERATURE);

            Serial_printf( &cli_serial, "*************Over temperature detected temperatureHotPoint[%d]\r\n", temperatureHotPoint);
        }
    } else
    {
        /* send EMCY CLEAR - send it once */
        if(global_error_code_sent & (1 << ERROR_OVER_TEMPERATURE)) {
            canopen_emcy_send_temperature_ok(temperatureHotPoint);
            Serial_printf( &cli_serial, "*************Normal temperature temperatureHotPoint[%d]\r\n", temperatureHotPoint);
            sharedVars_cpu1toCpu2.temperatureMaxLimitReachedFlag = false;
        }

        /* mark as unhandled - nothing need to be done  */
        global_error_code_sent    &= ~(1UL << ERROR_OVER_TEMPERATURE);
        global_error_code_handled &= ~(1UL << ERROR_OVER_TEMPERATURE);
    }
}



/* brief: check if there are any internal operational errors
 *
 * details: checks error flag
 *          signal IOP with EMCY
 *
 * requirements:
 *
 * argument: none
 *
 * return: none
 *
 * note: e.g. I2C communication error
 *       non-blocking
 *
 * presumptions:
 *
 */
//static void error_operational(void)
//{
//    if(global_error_code & (1UL << ERROR_OPERATIONAL))
//    {
//        if(!(global_error_code_handled & (1UL << ERROR_OPERATIONAL)))
//        {
//            bool error_persist = false;
//
//            /* test error */
//            //TODO Test if error persist
//
//            if(error_persist)
//            {
//                /* disconnect all switches
//                 * turn off regulation */
//                IPC_setFlagLtoR(IPC_CPU1_L_CPU2_R, IPC_CPU1_REQUIERS_EMERGECY_SHUT_DOWN);
//
//                /* mark handled */
//                global_error_code_handled |= (1UL << ERROR_OPERATIONAL);
//
//                /* send EMCY
//                 * can change argument to any meaningful code != '0'
//                 * */
//                canopen_emcy_send_operational_error(1);
//
//                /* mark it sent */
//                global_error_code_sent |= (1UL << ERROR_OPERATIONAL);
//            }
//        } else
//        {
//            /* there is nothing more we can do
//             * let IOP decide next step
//             * */
//        }
//    } else
//    {
//        /* send EMCY CLEAR - send it once */
//        if(global_error_code_sent & (1UL << ERROR_OPERATIONAL))
//            canopen_emcy_send_operational_error(0);
//
//        /* mark as unhandled - nothing need to be done  */
//        global_error_code_sent    &= ~(1UL << ERROR_OPERATIONAL);
//        global_error_code_handled &= ~(1UL << ERROR_OPERATIONAL);
//    }
//}






/* brief: check if there are any errors
 *        if not, send EMCY OK
 *
 * details: make sure all previous errors have got their EMCY error clear
 *          messages sent before sending EMCY OK.
 *          We do this by checking 'global_error_code_sent'.
 *
 * requirements:
 *
 * argument: none
 *
 * return: none
 *
 * note: non-blocking
 *
 * presumptions:
 *
 */
static void error_no_error(void)
{
    static bool message_sent = false;

    if((0 == (global_error_code & (~(1UL << ERROR_NO_ERRORS)) )) &&
       (0 == global_error_code_handled) &&
       (0 == global_error_code_sent))
    {
        /* send EMCY OK - send it once */
        if(!message_sent)
        {
            canopen_emcy_send_no_errors();
            message_sent = true;
        }
    } else
    {
        message_sent = false;
    }
}

static void error_copy_error_codes_from_CPU1_and_CPU2()
{
    global_error_code = global_error_code | sharedVars_cpu2toCpu1.error_code;
    global_error_code = global_error_code | error_code_CPU1;

}




///* brief: check if error_flag is set
// *
// * details: checks error flag
// *          turns off switches and DCDC
// *          signal IOP with EMCY
// *
// * requirements:
// *
// * argument: error_flag - the error flag to check
// *                        the bit number (1 << error_flag)
// *           timeout    - the time needed to disregard any short transients
// *                        zero if not used
// *           canopen_emcy_message_func - name of the function that will send the
// *                                       CANopen EMCY message
// *           payload_gen_func - name of function that will generate the payload,
// *                              last four Bytes of the CANopen EMCY message,
// *                              NULL if not used
// *
// * return: none
// *
// * note: non-blocking
// *
// * presumptions:
// *
// */
//bool timer_started[NR_OF_ERROR_CODES];
//uint32_t time_start[NR_OF_ERROR_CODES];
//static void error_check_error_flag_generic(uint32_t error_flag,
//                                           uint8_t emcy_error_byte,
//                                           uint16_t timeout,
//           void (*canopen_emcy_message_func)(uint16_t emcy_error_code,
//                                             uint8_t emcy_error_byte,
//                                             uint8_t status,
//                                             uint8_t payload[4]),
//           uint16_t emcy_error_code,
//           void (*payload_gen_func)(uint8_t status, uint8_t payload[4]))
//{
//    uint8_t pay_load[4];
//
//    if(global_error_code & (1UL << error_flag))
//    {
//        if(!(global_error_code_handled & (1UL << error_flag)))
//        {
//            if(!timer_started[error_flag])
//            {
//                time_start[error_flag] = timer_get_ticks() & 0x7fffffff;
//                timer_started[error_flag] = true;
//            }
//
//            /* ms ticks, enough with 127 ms */
//            uint32_t elapsed_time = (timer_get_ticks() & 0x7fffffff) - time_start[error_flag];
//            //TODO - Timer does not handle wrap around
//
//            /* send EMCY - send it once */
//            if(!(global_error_code_sent & (1UL << error_flag)))
//            {
//                /* disconnect load */
//                cli_switches(IPC_SWITCHES_QLB, SW_OFF);
//
//                /* generate necessary payload */
//                payload_gen_func(1, pay_load);
//
//                /* this function will handle any extra checks and/or add
//                 * status Bytes in the last four Bytes of the CANopen EMCY
//                 * payload */
//                canopen_emcy_message_func(emcy_error_code, emcy_error_byte, 1, pay_load);
////                canopen_emcy_send_dcbus_over_voltage(1);
//
//                /* mark it sent */
//                global_error_code_sent |= (1UL << error_flag);
//            }
//
//            //TODO what shall the timeout be? in milliseconds
//            /* check timeout */
//            if(timeout <= elapsed_time)
//            {
//                /* disconnect all switches
//                 * turn off regulation */
//                IPC_setFlagLtoR(IPC_CPU1_L_CPU2_R, IPC_CPU1_REQUIERS_EMERGECY_SHUT_DOWN);
//
//                /* mark handled */
//                global_error_code_handled |= (1UL << error_flag);
//            }
//        } else
//        {
//            ;
//            /* there is nothing more we can do
//             * let IOP decide next step
//             * */
//
//            /* clear the timer */
//            timer_started[error_flag] = false;
//        }
//    } else
//    {
//        /* send EMCY CLEAR - send it once */
//        if(global_error_code_sent & (1UL << error_flag))
//        {
//            /* generate necessary payload */
//            payload_gen_func(0, pay_load);
//
//            canopen_emcy_message_func(emcy_error_code, emcy_error_byte, 0, pay_load);
//        }
//
//        /* mark as unhandled - nothing need to be done  */
//        global_error_code_sent    &= ~(1UL << error_flag);
//        global_error_code_handled &= ~(1UL << error_flag);
//
//        /* clear the timer */
//        timer_started[error_flag] = false;
//    }
//}



