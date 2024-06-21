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

        error_copy_error_codes_from_CPU2();
        error_load_overcurrent();
        error_dcbus_short_circuit();
        error_dcbus_over_voltage();
        error_dcbus_under_voltage();

        //    error_check_error_flag_generic(ERROR_BUS_OVER_VOLTAGE,
//                                   EMCY_ERROR_CODE_VOLTAGE,
//                                   2,
//                                   canopen_emcy_send_generic,
//                                   EMCY_ERROR_BUS_OVER_VOLTAGE,
//                                   payload_gen_bus_voltage);
//
////    error_check_error_flag_generic(ERROR_BUS_UNDER_VOLTAGE,
////                                   EMCY_ERROR_CODE_VOLTAGE,
////                                   0,
////                                   canopen_emcy_send_generic,
////                                   EMCY_ERROR_BUS_OVER_VOLTAGE,
////                                   payload_gen_bus_over_voltage);
//


//    error_system_temperature();
//
//    error_operational();
//
//    error_power_line_failure();
//
//    error_state_of_charge();
//
//    error_check_error_flag_generic(ERROR_LOAD_OVER_CURRENT,
//                                   EMCY_ERROR_CODE_CURRENT,
//                                   2,
//                                   canopen_emcy_send_generic,
//                                   EMCY_ERROR_OVER_CURRENT_LOAD,
//                                   payload_gen_load_current);
//
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
            canopen_emcy_send_load_overcurrent(1);

            /* mark it sent */
            global_error_code_sent    |= (1UL << ERROR_BUS_OVER_VOLTAGE);
            global_error_code_handled |= (1UL << ERROR_BUS_OVER_VOLTAGE);

            Serial_printf( &cli_serial, "************* Load over voltage detected\r\n");
        }
    } else
    {
        /* send EMCY CLEAR - send it once */
        if(global_error_code_sent & (1 << ERROR_BUS_OVER_VOLTAGE)) {
            canopen_emcy_send_load_overcurrent(0);
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

/* brief: check if temperatures are above thresholds
 *
 * details: checks error flag
 *          turns off switches and DCDC
 *
 * requirements:
 *
 * argument: none
 *
 * return: none
 *
 * note: EMCY messages are handled in
 *       temperature_sensor_read_all_temperatures()
 *       non-blocking
 *
 * presumptions:
 *
 */
static void error_system_temperature(void)
{
    static bool timer_started = false;
    static uint32_t time_start;

    /* !!! EMCY messages is handled in temperature_sensor_read_all_temperatures() !!! */
    if(global_error_code & (1UL << ERROR_OVER_TEMPERATURE))
    {
        /* According to specification:
         * Only inform IOP (done in temperature_sensor_read_all_temperatures())
         * Do not do anything more
         * IOP takes decision on what to do next
         * */
        if(!(global_error_code_handled & (1UL << ERROR_OVER_TEMPERATURE)))
        {
            if(!timer_started)
            {
                time_start = timer_get_ticks() & 0x7fffffff;
                timer_started = true;
            }

            /* ms ticks, enough with 127 ms */
            int16_t elapsed_time = (timer_get_ticks() & 0x7fffffff) - time_start;
            //TODO - Time does not handle wrap around

            /* check timeout */
            if(2 < elapsed_time)
            {
//                static bool message_sent = false;

                /* disconnect all switches
                 * turn off regulation */
                IPC_setFlagLtoR(IPC_CPU1_L_CPU2_R, IPC_CPU1_REQUIERS_EMERGECY_SHUT_DOWN);

                global_error_code_handled |= (1UL << ERROR_OVER_TEMPERATURE);

                /* clear the timer */
                timer_started = false;
            }
        } else
        {
            /* there is nothing more we can do
             * let IOP decide next step
             * */

            /* clear the timer */
            timer_started = false;
        }
    } else
    {
        /* mark as unhandled - nothing need to be done  */
        global_error_code_handled &= ~(1UL << ERROR_OVER_TEMPERATURE);

        /* clear the timer */
        timer_started = false;
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
static void error_operational(void)
{
    if(global_error_code & (1UL << ERROR_OPERATIONAL))
    {
        if(!(global_error_code_handled & (1UL << ERROR_OPERATIONAL)))
        {
            bool error_persist = false;

            /* test error */
            //TODO Test if error persist

            if(error_persist)
            {
                /* disconnect all switches
                 * turn off regulation */
                IPC_setFlagLtoR(IPC_CPU1_L_CPU2_R, IPC_CPU1_REQUIERS_EMERGECY_SHUT_DOWN);

                /* mark handled */
                global_error_code_handled |= (1UL << ERROR_OPERATIONAL);

                /* send EMCY
                 * can change argument to any meaningful code != '0'
                 * */
                canopen_emcy_send_operational_error(1);

                /* mark it sent */
                global_error_code_sent |= (1UL << ERROR_OPERATIONAL);
            }
        } else
        {
            /* there is nothing more we can do
             * let IOP decide next step
             * */
        }
    } else
    {
        /* send EMCY CLEAR - send it once */
        if(global_error_code_sent & (1UL << ERROR_OPERATIONAL))
            canopen_emcy_send_operational_error(0);

        /* mark as unhandled - nothing need to be done  */
        global_error_code_sent    &= ~(1UL << ERROR_OPERATIONAL);
        global_error_code_handled &= ~(1UL << ERROR_OPERATIONAL);
    }
}






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

static void error_copy_error_codes_from_CPU2()
{
    global_error_code |= sharedVars_cpu2toCpu1.error_code;
}



//static bool disconnect_other_dpmu_from_shared_bus(void)
//{
//    uint32_t cmd = CANB_DISCONNECT_SHARED_BUS;
//    uint32_t addr = 0;
//    uint32_t data = (uint32_t)sharedVars_cpu2toCpu1.voltage_at_dc_bus;
//
//    return IPC_sendCommand(IPC_CPU1_L_CPU2_R, IPC_CANB_OUTGOING, false, cmd, addr, data);
//}

//static int8_t disconnect_other_dpmu_from_shared_bus_answer(void)
//{
//    uint32_t cmd;
//    uint32_t addr;
//    uint32_t data;
//    int8_t return_value = -1;   /* command not received */
//
//    if(IPC_readCommand(IPC_CPU1_L_CPU2_R, IPC_CANB_INCOMING, false, &cmd, &addr, &data) != false)
//    {
//        /* 0 disconnected
//         * 1 connected
//         */
//        switch (cmd) {
//            case CANB_DISCONNECT_SHARED_BUS:
//                return_value = 0;
//                break;
//            case CANB_CONNECT_SHARED_BUS:
//                return_value = 1;
//                break;
//            default:
//                break;
//        }
//        IPC_ackFlagRtoL(IPC_CPU1_L_CPU2_R, IPC_CANB_INCOMING);
//    }
//
//    return return_value;
//}
//
//bool connect_other_dpmu_to_shared_bus(void)
//{
//    uint32_t cmd = CANB_CONNECT_SHARED_BUS;
//    uint32_t addr = 0;
//    uint32_t data = (uint32_t)sharedVars_cpu2toCpu1.voltage_at_dc_bus;
//
//    return IPC_sendCommand(IPC_CPU1_L_CPU2_R, IPC_CANB_OUTGOING, false, cmd, addr, data);
//}
//
//int8_t connect_other_dpmu_to_shared_bus_answer(float *remote_bus_voltage)
//{
//    uint32_t cmd;
//    uint32_t addr;
//    uint32_t data;
//    int8_t return_value = -1;   /* command not received */
//
//    if(IPC_readCommand(IPC_CPU1_L_CPU2_R, IPC_CANB_INCOMING, false, &cmd, &addr, &data) != false)
//    {
//        /* 0 disconnected
//         * 1 connected
//         */
//        switch (cmd) {
//            case CANB_DISCONNECT_SHARED_BUS:
//                return_value = 0;
//                *remote_bus_voltage = (float)data;
//                break;
//            case CANB_CONNECT_SHARED_BUS:
//                return_value = 1;
//                *remote_bus_voltage = (float)data;
//                break;
//            default:
//                break;
//        }
//
//        IPC_ackFlagRtoL(IPC_CPU1_L_CPU2_R, IPC_CANB_INCOMING);
//    }
//
//    return return_value;
//}


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


///* brief: check if there are external power failures
// *
// * details: checks error flags
// *          turns off switches and DCDC
// *          signal IOP with EMCY
// *          one power line failure
// *          two power line failures
// *
// * requirements:
// *
// * argument: none
// *
// * return: none
// *
// * note: non-blocking
// *
// * presumptions:
// *
// * Skriv om så att man:
// *  - testar ena linan för sig
// *      och subtraherar dess effekt från tillgänglig effekt
// *      vid återgång återställer man effekten
// *  - testar andra linan för sig
// *      och subtraherar dess effekt från tillgänglig effekt
// *      vid återgång återställer man effekten
// *  - om båda är kaputt går man till nödläge
// */
//static void error_power_line_failure(void)
//{
//    uint16_t pwr_loss_error = 0;
//
//    if(global_error_code & (1UL << ERROR_EXT_PWR_LOSS_MAIN))
//        pwr_loss_error |= 0b01;
//    if(global_error_code & (1UL << ERROR_EXT_PWR_LOSS_OTHER))
//        pwr_loss_error |= 0b10;
//
//    switch(pwr_loss_error)
//    {
//    case 0: /* no pwr loss error */
//        sharedVars_cpu1toCpu2.use_power_budget_dc_input = 1;
//        sharedVars_cpu1toCpu2.use_power_budget_dc_shared = 1;
//        sharedVars_cpu1toCpu2.safe_parking_allowed = 0;
//
//        /* clear flag */
//        global_error_code &= ~(1UL << ERROR_EXT_PWR_LOSS_BOTH);
//        break;
//
//    case 1: /* we have lost power from our INPUT bus */
//        sharedVars_cpu1toCpu2.use_power_budget_dc_input = 0;
//        sharedVars_cpu1toCpu2.use_power_budget_dc_shared = 1;
//        sharedVars_cpu1toCpu2.safe_parking_allowed = 0;
//
//        /* clear flag */
//        global_error_code &= ~(1UL << ERROR_EXT_PWR_LOSS_BOTH);
//        break;
//
//    case 2: /* the other DPMU have lost power from its INPUT bus */
//        sharedVars_cpu1toCpu2.use_power_budget_dc_input = 1;
//        sharedVars_cpu1toCpu2.use_power_budget_dc_shared = 0;
//        sharedVars_cpu1toCpu2.safe_parking_allowed = 0;
//
//        /* clear flag */
//        global_error_code &= ~(1UL << ERROR_EXT_PWR_LOSS_BOTH);
//        break;
//
//    case 3: /* both  we and the other DPMU have lost power from our INPUT buses */
//        sharedVars_cpu1toCpu2.use_power_budget_dc_input = 0;
//        sharedVars_cpu1toCpu2.use_power_budget_dc_shared = 0;
//        sharedVars_cpu1toCpu2.safe_parking_allowed = 1;
//
//        /* set flag */
//        global_error_code |= (1UL << ERROR_EXT_PWR_LOSS_BOTH);
//        break;
//    }
//
//    if(pwr_loss_error == 0b01)
//    {
//        if(!(global_error_code_handled & (1UL << ERROR_EXT_PWR_LOSS_MAIN)))
//        {
//            /* send EMCY - send it once */
//            canopen_emcy_send_power_line_failure(1);
//
//            /* mark it sent */
//            global_error_code_sent |= (1UL << ERROR_EXT_PWR_LOSS_MAIN);
//
//            /* mark handled */
//            global_error_code_handled |= (1UL << ERROR_EXT_PWR_LOSS_MAIN);
//        } else
//        {
//            /* there is nothing more we can do
//             * let IOP decide next step
//             * till then, continue as normal, but with reduced effect
//             * */
//        }
//    } else
//    {
//        /* send EMCY CLEAR - send it once */
//        if(global_error_code_sent & (1UL << ERROR_EXT_PWR_LOSS_MAIN))
//            canopen_emcy_send_power_line_failure(0);
//
//        global_error_code_sent    &= ~(1UL << ERROR_EXT_PWR_LOSS_MAIN);
//        global_error_code_handled &= ~(1UL << ERROR_EXT_PWR_LOSS_MAIN);
//    }
//
//    if(pwr_loss_error == 0b10)
//    {
//        if(!(global_error_code_handled & (1UL << ERROR_EXT_PWR_LOSS_OTHER)))
//        {
//            /* send EMCY - send it once */
//            canopen_emcy_send_power_line_failure(1);
//
//            /* mark it sent */
//            global_error_code_sent |= (1UL << ERROR_EXT_PWR_LOSS_OTHER);
//
//            /* mark handled */
//            global_error_code_handled |= (1UL << ERROR_EXT_PWR_LOSS_OTHER);
//        } else
//        {
//            /* there is nothing more we can do
//             * let IOP decide next step
//             * till then, continue as normal, but with reduced effect
//             * */
//        }
//    } else
//    {
//        /* send EMCY CLEAR - send it once */
//        if(global_error_code_sent & (1UL << ERROR_EXT_PWR_LOSS_OTHER))
//            canopen_emcy_send_power_line_failure(0);
//
//        global_error_code_sent    &= ~(1UL << ERROR_EXT_PWR_LOSS_OTHER);
//        global_error_code_handled &= ~(1UL << ERROR_EXT_PWR_LOSS_OTHER);
//    }
//
//    if(pwr_loss_error == 0b11)
//    {
//        if(!(global_error_code_handled & (1UL << ERROR_EXT_PWR_LOSS_BOTH)))
//        {
//            /* send EMCY - send it once */
//            canopen_emcy_send_power_line_failure_both(1);
//
//            /* mark it sent */
//            global_error_code_sent |= (1UL << ERROR_EXT_PWR_LOSS_BOTH);
//
//            /* mark handled */
//            global_error_code_handled |= (1UL << ERROR_EXT_PWR_LOSS_BOTH);
//        } else
//        {
//            ;
//            /* there is nothing more we can do
//             * let IOP decide next step
//             * */
//        }
//    } else
//    {
//        /* send EMCY CLEAR - send it once */
//        if(global_error_code_sent & (1UL << ERROR_EXT_PWR_LOSS_BOTH))
//            canopen_emcy_send_power_line_failure_both(0);
//
//        /* mark as unhandled - nothing need to be done  */
//        global_error_code_sent    &= ~(1UL << ERROR_EXT_PWR_LOSS_BOTH);
//        global_error_code_handled &= ~(1UL << ERROR_EXT_PWR_LOSS_BOTH);
//    }
//}


///* brief: check State of Charge error
// *
// * details: checks error flag
// *          turns off switches and DCDC
// *          signal IOP with EMCY
// *
// * requirements:
// *
// * argument: none
// *
// * return: none
// *
// * note: non-blocking
// *
// * presumptions:
// *
// */
//static void error_state_of_charge(void)
//{
//    static bool timer_started = false;
//    static uint32_t time_start;
//
//    if(global_error_code & (1UL << ERROR_SOC_BELOW_SAFETY_THRESHOLD))
//    {
//        if(!(global_error_code_handled & (1UL << ERROR_SOC_BELOW_SAFETY_THRESHOLD)))
//        {
//            if(!timer_started)
//            {
//                time_start = timer_get_ticks() & 0x7fffffff;
//                timer_started = true;
//            }
//
//            /* ms ticks, enough with 127 ms */
//            int16_t elapsed_time = (timer_get_ticks() & 0x7fffffff) - time_start;
//            //TODO - Time does not handle wrap around
//
//            /* a short timeout might be needed
//             * in the meaning or inaccuracy of measurement
//             * send if enough measurements/mean value is giving this error */
//
//            if(100 < elapsed_time)
//            {
//                /* mark handled */
//                global_error_code_handled |= (1UL << ERROR_SOC_BELOW_SAFETY_THRESHOLD);
//
//                /* send EMCY */
//                canopen_emcy_send_state_of_charge_safety_error(1, sharedVars_cpu2toCpu1.remaining_energy_to_min_soc_energy_bank);
//
//                /* mark it sent */
//                global_error_code_sent |= (1UL << ERROR_SOC_BELOW_SAFETY_THRESHOLD);
//
//                /* mark handled */
//                global_error_code_handled |= (1UL << ERROR_SOC_BELOW_SAFETY_THRESHOLD);
//
//                /* clear the timer */
//                timer_started = false;
//            }
//        } else
//        {
//            /* there is nothing more we can do
//             * let IOP decide next step
//             * */
//
//            /* clear the timer */
//            timer_started = false;
//        }
//    } else
//    {
//        /* send EMCY CLEAR - send it once */
//        if(global_error_code_sent & (1UL << ERROR_SOC_BELOW_SAFETY_THRESHOLD))
//            canopen_emcy_send_state_of_charge_safety_error(0, sharedVars_cpu2toCpu1.soc_energy_bank);
//
//        /* mark as unhandled - nothing need to be done  */
//        global_error_code_sent    &= ~(1UL << ERROR_SOC_BELOW_SAFETY_THRESHOLD);
//        global_error_code_handled &= ~(1UL << ERROR_SOC_BELOW_SAFETY_THRESHOLD);
//
//        /* clear the timer */
//        timer_started = false;
//    }
//
//    /* ERROR_SOC_BELOW_LIMIT
//     * not enough Voltage for boosting to S_TARGET_VOLTAGE_AT_DC_BUS
//     * */
//    if(global_error_code & (1UL << ERROR_SOC_BELOW_LIMIT))
//    {
//        if(!(global_error_code_handled & (1UL << ERROR_SOC_BELOW_LIMIT)))
//        {
//            bool error_persist = false;
//
//            /* test error */
//            /*TODO Test if error persist
//             * in the meaning or inaccuracy of measurement
//             * send if enough measurements/mean value is giving this error
//             * */
//
//            if(error_persist)
//            {
//                /* mark handled */
//                global_error_code_handled |= (1UL << ERROR_SOC_BELOW_LIMIT);
//
//                /* send EMCY */
//                //TODO change to Voltage over energy bank
//                canopen_emcy_send_state_of_charge_min_error(1, sharedVars_cpu2toCpu1.soc_energy_bank);
//
//                /* mark it sent */
//                global_error_code_sent |= (1UL << ERROR_SOC_BELOW_LIMIT);
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
//        if(global_error_code_sent & (1UL << ERROR_SOC_BELOW_LIMIT))
//            canopen_emcy_send_state_of_charge_min_error(0, sharedVars_cpu2toCpu1.soc_energy_bank);
//
//        /* mark as unhandled - nothing need to be done  */
//        global_error_code_sent    &= ~(1UL << ERROR_SOC_BELOW_LIMIT);
//        global_error_code_handled &= ~(1UL << ERROR_SOC_BELOW_LIMIT);
//    }
//}
