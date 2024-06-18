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

enum short_circuit_states{
    no_short_circuit = 0,
    wait_for_answer_from_other_dpmu_disconnect,
    shared_bus_disconnected,
    shared_bus_not_disconnected,
    reconnect_shared_bus,
    wait_for_answer_from_other_dpmu_reconnect,
    shared_bus_reconnected,
    shared_bus_disconnect_after_reconnect,
    shared_bus_disconnect_after_reconnect_answer,
    shared_bus_not_reconnected,
    clear_error,
    wait_for_error_to_disappear,
};

enum short_circuit_status{
    scs_no_error,
    scs_not_allowed_to_connect_sb,
    scs_short_circuit,
    scs_waiting_for_disconnection,
    scs_waiting_for_disconnection_timeout,
    scs_other_dpmu_not_disconnected,
    scs_other_dpmu_still_connected,
    scs_after_disconnection,
    scs_waiting_for_connection,
    scs_waiting_for_connection_timeout,
    scs_other_dpmu_still_disconnected,
    scs_other_dpmu_after_reconnection,
};

       uint32_t global_error_code         = 0;
static uint32_t global_error_code_handled = 0;
static uint32_t global_error_code_sent    = 0;

/* brief: check if DC bus Voltage level is above max
 *
 * details: checks error flag
 *          turns off switches and DCDC
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
//static void error_dcbus_over_voltage(void)
//{
//    static bool timer_started = false;
//    static uint32_t time_start;
//
//    if(global_error_code & (1UL << ERROR_BUS_OVER_VOLTAGE))
//    {
//        if(!(global_error_code_handled & (1UL << ERROR_BUS_OVER_VOLTAGE)))
//        {
//            if(!timer_started)
//            {
//                time_start = timer_get_ticks() & 0x7fffffff;
//                timer_started = true;
//            }
//
//            /* ms ticks, enough with 127 ms */
//            uint32_t elapsed_time = (timer_get_ticks() & 0x7fffffff) - time_start;
//            //TODO - Timer does not handle wrap around
//
//            /* send EMCY - send it once */
//            if(!(global_error_code_sent & (1UL << ERROR_BUS_OVER_VOLTAGE)))
//            {
//                /* disconnect load */
//                cli_switches(IPC_SWITCHES_QLB, SW_OFF);
//
//                canopen_emcy_send_dcbus_over_voltage(1);
//
//                /* mark it sent */
//                global_error_code_sent |= (1UL << ERROR_BUS_OVER_VOLTAGE);
//            }
//
//            /* check timeout */
//            if(2 < elapsed_time)
//            {
//                /* disconnect all switches
//                 * turn off regulation */
//                IPC_setFlagLtoR(IPC_CPU1_L_CPU2_R, IPC_CPU1_REQUIERS_EMERGECY_SHUT_DOWN);
//
//                /* mark handled */
//                global_error_code_handled |= (1UL << ERROR_BUS_OVER_VOLTAGE);
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
//        if(global_error_code_sent & (1UL << ERROR_BUS_OVER_VOLTAGE))
//            canopen_emcy_send_dcbus_over_voltage(0);
//
//        /* mark as unhandled - nothing need to be done  */
//        global_error_code_sent    &= ~(1UL << ERROR_BUS_OVER_VOLTAGE);
//        global_error_code_handled &= ~(1UL << ERROR_BUS_OVER_VOLTAGE);
//
//        /* clear the timer */
//        timer_started = false;
//    }
//}

/* brief: check if DC bus Voltage level is below min
 *
 * details: checks error flag
 *          turns off switches and DCDC
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
 * presumptions: there is big gap in Voltage between short_circuit and
 *               min_allowed_voltage, no need to have a timer.
 *               A short circuit need immediate reaction!
 *
 */
static void error_dcbus_under_voltage(void)
{
//    static bool timer_started = false;
//    static uint32_t time_start;
    uint8_t pay_load[4];


    if(global_error_code & (1UL << ERROR_BUS_UNDER_VOLTAGE))
    {
        if(!(global_error_code_handled & (1UL << ERROR_BUS_UNDER_VOLTAGE)))
        {
//            if(!timer_started)
//            {
//                time_start = timer_get_ticks() & 0x7fffffff;
//                timer_started = true;
//            }
//
//            /* ms ticks, enough with 127 ms */
//            uint32_t elapsed_time = (timer_get_ticks() & 0x7fffffff) - time_start;
//            //TODO - Time does not handle wrap around

            /* send EMCY - send it once */
            if(!(global_error_code_sent & (1UL << ERROR_BUS_UNDER_VOLTAGE)))
            {
                /* generate necessary payload */
                payload_gen_bus_voltage(0, pay_load);

                canopen_emcy_send_dcbus_under_voltage(1, pay_load);

                /* mark it sent */
                global_error_code_sent |= (1UL << ERROR_BUS_UNDER_VOLTAGE);
            }

//            /* check timeout */
//            if(0 <= elapsed_time)
//            {
                /* disconnect all switches
                 * turn off regulation */
                IPC_setFlagLtoR(IPC_CPU1_L_CPU2_R, IPC_CPU1_REQUIERS_EMERGECY_SHUT_DOWN);

                /* mark handled */
                global_error_code_handled |= (1UL << ERROR_BUS_UNDER_VOLTAGE);
//            }
        } else
        {
            /* there is nothing more we can do
             * let IOP decide next step
             * */
        }
    } else
    {   /* problem resolved */
        /* send EMCY CLEAR - send it once */
        if(global_error_code_sent & (1UL << ERROR_BUS_UNDER_VOLTAGE))
        {
            /* generate necessary payload */
            payload_gen_bus_voltage(0, pay_load);

            canopen_emcy_send_dcbus_under_voltage(0, pay_load);
        }

        /* mark as unhandled - nothing need to be done  */
        global_error_code_sent    &= ~(1UL << ERROR_BUS_UNDER_VOLTAGE);
        global_error_code_handled &= ~(1UL << ERROR_BUS_UNDER_VOLTAGE);

        /* clear the timer */
//        timer_started = false;
    }
}

/* return: reading of error flag as true/false */
static inline bool short_circuit_flag_set_in_cpu2(void)
{
    return (global_error_code & (1UL << ERROR_BUS_SHORT_CIRCUIT)? true : false);
}

/* brief: execute an emergency turn off and inform IOP
 *
 * details: turns off switches and DCDC
 *          signal IOP with EMCY
 *
 * requirements:
 *
 * argument: status - the status Byte in our CANopen EMCY message
 *
 * return: none
 *
 * note: non-blocking
 *
 * presumptions:
 *
 */
static void short_circuit_emergency_turn_off(uint8_t status)
{
    uint8_t payload[4] = {0};
    uint32_t cmd = CANB_EMERGENCY_TURN_OFF;
    uint32_t addr = 0;
    uint32_t data = (uint32_t)sharedVars_cpu2toCpu1.voltage_at_dc_bus;

    /* disconnect all switches
     * turn off regulation */
    IPC_setFlagLtoR(IPC_CPU1_L_CPU2_R, IPC_CPU1_REQUIERS_EMERGECY_SHUT_DOWN);

    /* send EMCY */
    payload_gen_bus_voltage(status, payload);
    canopen_emcy_send_dcbus_short_curcuit(status, payload);

    /* inform other DUPMU/s to run emergency turn off */
    IPC_sendCommand(IPC_CPU1_L_CPU2_R, IPC_CANB_OUTGOING, false, cmd, addr, data);

    /* mark handled */
    global_error_code_handled |= (1UL << ERROR_BUS_SHORT_CIRCUIT);
}

static bool disconnect_other_dpmu_from_shared_bus(void)
{
    uint32_t cmd = CANB_DISCONNECT_SHARED_BUS;
    uint32_t addr = 0;
    uint32_t data = (uint32_t)sharedVars_cpu2toCpu1.voltage_at_dc_bus;

    return IPC_sendCommand(IPC_CPU1_L_CPU2_R, IPC_CANB_OUTGOING, false, cmd, addr, data);
}

static int8_t disconnect_other_dpmu_from_shared_bus_answer(void)
{
    uint32_t cmd;
    uint32_t addr;
    uint32_t data;
    int8_t return_value = -1;   /* command not received */

    if(IPC_readCommand(IPC_CPU1_L_CPU2_R, IPC_CANB_INCOMING, false, &cmd, &addr, &data) != false)
    {
        /* 0 disconnected
         * 1 connected
         */
        switch (cmd) {
            case CANB_DISCONNECT_SHARED_BUS:
                return_value = 0;
                break;
            case CANB_CONNECT_SHARED_BUS:
                return_value = 1;
                break;
            default:
                break;
        }
        IPC_ackFlagRtoL(IPC_CPU1_L_CPU2_R, IPC_CANB_INCOMING);
    }

    return return_value;
}

bool connect_other_dpmu_to_shared_bus(void)
{
    uint32_t cmd = CANB_CONNECT_SHARED_BUS;
    uint32_t addr = 0;
    uint32_t data = (uint32_t)sharedVars_cpu2toCpu1.voltage_at_dc_bus;

    return IPC_sendCommand(IPC_CPU1_L_CPU2_R, IPC_CANB_OUTGOING, false, cmd, addr, data);
}

int8_t connect_other_dpmu_to_shared_bus_answer(float *remote_bus_voltage)
{
    uint32_t cmd;
    uint32_t addr;
    uint32_t data;
    int8_t return_value = -1;   /* command not received */

    if(IPC_readCommand(IPC_CPU1_L_CPU2_R, IPC_CANB_INCOMING, false, &cmd, &addr, &data) != false)
    {
        /* 0 disconnected
         * 1 connected
         */
        switch (cmd) {
            case CANB_DISCONNECT_SHARED_BUS:
                return_value = 0;
                *remote_bus_voltage = (float)data;
                break;
            case CANB_CONNECT_SHARED_BUS:
                return_value = 1;
                *remote_bus_voltage = (float)data;
                break;
            default:
                break;
        }

        IPC_ackFlagRtoL(IPC_CPU1_L_CPU2_R, IPC_CANB_INCOMING);
    }

    return return_value;
}

/* brief: check if DC bus Voltage level is below short circuit detection level
 *
 * details: checks error flag
 *          turns off switches and DCDC
 *          signal IOP with EMCY
 *          tries to reconnect the DPMUs through shared bus
 *
 * requirements:
 *
 * argument: none
 *
 * return: none
 *
 * note: non-blocking
 *
 * presumptions: all timeouts are in system tick, 1 ms
 *
 */
static void error_dcbus_short_circuit(void)
{
    static bool timer_started = false;
    static uint32_t time_start;
    int16_t elapsed_time;

    static uint8_t state = no_short_circuit;
    uint8_t payload[4] = {0};

    switch(state)
    {
    /* wait for error to occure
     * if correct DPMU type: disconnect shared bus
     *                  ask the other/s DPMU/s to disconnect from the shared bus
     * if wrong DPMU type: we are not allowed to use the shared bus
     *                     never connected to the shared bus
     *                     nothing to reconnect to
     * non-blocking -> timeout
     * */
    case no_short_circuit:
        if(short_circuit_flag_set_in_cpu2())
        {
            if(!(global_error_code_handled & (1UL << ERROR_BUS_SHORT_CIRCUIT)))
            {
                /* Turn OFF QSHARE (GLOAD_3)
                 * Do it here, regardless if needed to be here or not
                 * this is the place for the fastest response
                 * */
                cli_switches(IPC_SWITCHES_QSB, SW_OFF);

                /* check if correct DPMU type to use shared bus
                 * if not, we cannot try to reconnect -> emergency error
                 * */
                if(!dpmu_type_allowed_to_use_shared_bus())
                {
                    /* if we are not allowed to use the shared bus and get a
                     * short circuit, there will be no reason to check for
                     * short circuit after disconnecting from something we are
                     * not connected to
                     *
                     * meaning: full scale problem, emergency shutdown
                     */
                    short_circuit_emergency_turn_off(scs_not_allowed_to_connect_sb);

                    /* next state -> wait for error to disappear */
                    state = wait_for_error_to_disappear;
                } else
                {
                    if(!timer_started)
                    {
                        time_start = timer_get_ticks() & 0x7fffffff;
                        timer_started = true;
                    }

                    elapsed_time = (timer_get_ticks() & 0x7fffffff) - time_start;
                    //TODO - Time does not handle wrap around

                    /* send disconnect to the other/others DPMU/s, Qsb */
                    /* change state if we could send the IPC command to CPU2 */
                    if(disconnect_other_dpmu_from_shared_bus())
                    {
                        state = wait_for_answer_from_other_dpmu_disconnect;
                    } else
                    {
                        /* check timer
                         * stop trying send command after timer has expired
                         *  */
                        if(200 < elapsed_time)
                            state = wait_for_answer_from_other_dpmu_disconnect;
                    }

                    //TODO Mark as for continuing with reduced effect
                    //     Probably as signal to CPU2 to change some parameter
//                    sharedVars_cpu1toCpu2.available_power_budget_dc_input = xxx;

                    /* send EMCY - send it once */
                    if(!(global_error_code_sent & (1UL << ERROR_BUS_SHORT_CIRCUIT)))
                    {
                        payload_gen_bus_voltage(scs_short_circuit, payload);
                        canopen_emcy_send_dcbus_short_curcuit(scs_short_circuit, payload);

                        /* mark it sent */
                        global_error_code_sent |= (1UL << ERROR_BUS_SHORT_CIRCUIT);
                    }
                }
            }
        } else
        {
            /* send EMCY CLEAR - send it once */
            if(global_error_code_sent & (1UL << ERROR_BUS_SHORT_CIRCUIT))
            {
                payload_gen_bus_voltage(scs_no_error, payload);
                canopen_emcy_send_dcbus_short_curcuit(scs_no_error, payload);
            }

            /* mark as unhandled - nothing need to be done  */
            global_error_code_sent    &= ~(1UL << ERROR_BUS_SHORT_CIRCUIT);
            global_error_code_handled &= ~(1UL << ERROR_BUS_SHORT_CIRCUIT);
        }

        /* if we leave this state we should release the timer */
        if(wait_for_answer_from_other_dpmu_reconnect != state)
            timer_started = false;

        break;

    /* wait for answer from other DPMU/s if it/they have disconnected from the
     * shared bus
     *
     * here we do not need to check if error persist, we have done all we can,
     * disconnected ourself from the shared bus and asked the other/s DPMU/s to
     * do the same
     *
     * non-blocking -> timeout
     * */
    case wait_for_answer_from_other_dpmu_disconnect:
    {
        int answer;

        if(!timer_started)
        {
            time_start = timer_get_ticks() & 0x7fffffff;
            timer_started = true;
        }

        elapsed_time = (timer_get_ticks() & 0x7fffffff) - time_start;
        //TODO timer not handling wrap around

        answer = disconnect_other_dpmu_from_shared_bus_answer();

        /* check if we got answer
         * -1 -> no answer
         */
        switch(answer)
        {
        case -1: /* no answer */
            break;

        case 0: /* answer disconnected */
            /* next state -> shared bus disconnected */
            state = shared_bus_disconnected;
            break;

        case 1: /* the other DPMU/s is/are still connected */
            /* send information to IOP */
            payload_gen_bus_voltage(scs_other_dpmu_still_connected, payload);
            canopen_emcy_send_dcbus_short_curcuit(scs_other_dpmu_still_connected, payload);

            /* wait for what to do next */
            state = shared_bus_not_disconnected;
            break;

        default:
            break;
        }

        /* check timer */
        if(200 < elapsed_time)
        {
            /* send information to IOP and wait for what to do next */
            payload_gen_bus_voltage(scs_waiting_for_disconnection_timeout, payload);
            canopen_emcy_send_dcbus_short_curcuit(scs_waiting_for_disconnection_timeout, payload);

            /* next state -> shared bus disconnected */
            state = wait_for_error_to_disappear;
        }

        /* if we leave this state we should release the timer */
        if(wait_for_answer_from_other_dpmu_disconnect != state)
            timer_started = false;

        break;
    }

    /* other DPMU/s has/have disconnected from the shared bus
     * wait for error to resolve
     * non-blocking -> timeout
     * */
    case shared_bus_disconnected:
        if(!timer_started)
        {
            time_start = timer_get_ticks() & 0x7fffffff;
            timer_started = true;
        }

        elapsed_time = (timer_get_ticks() & 0x7fffffff) - time_start;
        //TODO - Time does not handle wrap around

        /* wait and see if error disappears */
        if(short_circuit_flag_set_in_cpu2())
        {
            /* check timeout */
            if(2 < elapsed_time)
            {   /* error persist */
                /* disconnect all switches
                 * turn off regulation */
                short_circuit_emergency_turn_off(scs_after_disconnection);

                /* next state -> wait for error to disappear */
                state = wait_for_error_to_disappear;
            }
        } else
        {   /* disconnecting from shared bus solves the problem */

            /* next state -> reconnect shared bus */
            state = reconnect_shared_bus;
        }

        /* if we leave this state we should release the timer */
        if(shared_bus_disconnected != state)
            timer_started = false;

        break;

    /* shared bus not disconnected at the other DPMU/s
     * if error has disappeared -> continue with reduced effect
     * continue with reduced effect
     * */
    case shared_bus_not_disconnected:
        //TODO Mark as for continuing with reduced effect
        //     Probably as signal to CPU2 to change some parameter
//            sharedVars_cpu1toCpu2.available_power_budget_dc_input = xxx;

//        canopen_emcy_send_power_sharing_error(1);

        if(short_circuit_flag_set_in_cpu2())
            state = wait_for_error_to_disappear;
        else
            state = clear_error;

        /* mark handled */
        global_error_code_handled |= (1UL << ERROR_BUS_SHORT_CIRCUIT);

        break;

    /* error has not returned
     * try to reconnect the shared bus, start with the other DPMU/s
     * */
    case reconnect_shared_bus:
        if(!timer_started)
        {
            time_start = timer_get_ticks() & 0x7fffffff;
            timer_started = true;
        }

        elapsed_time = (timer_get_ticks() & 0x7fffffff) - time_start;
        //TODO - Time does not handle wrap around

        if(short_circuit_flag_set_in_cpu2())
        {   /* error returns
             * returned during switching to this state
             * error came back after disconnecting shared bus
             * very unlikely to reach this if-statement
             * */
            short_circuit_emergency_turn_off(scs_after_disconnection);

            /* next state -> shared bus not reconnected */
            state = wait_for_error_to_disappear;
        } else
        {
            /* try to reconnect and see if error returns */
            /* change state if we could send the IPC command to CPU2 */
            if(connect_other_dpmu_to_shared_bus())
            {
                state = wait_for_answer_from_other_dpmu_reconnect;
            } else
            {
                /* check timeout */
                if(200 < elapsed_time)
                {   /* we can't wait forever for this message to be sent
                     * continue to next state anyway
                     */
                    state = wait_for_answer_from_other_dpmu_reconnect;
                }
            }
        }

        /* if we leave this state we should release the timer */
        if(reconnect_shared_bus != state)
            timer_started = false;

        break;

    /* wait for answer from other DPMU/s if it/they have reconnected to the
     * shared bus
     * non-blocking -> timeout
     * */
    case wait_for_answer_from_other_dpmu_reconnect:
    {
        float remote_bus_voltage = 0;
        int answer;

        if(!timer_started)
        {
            time_start = timer_get_ticks() & 0x7fffffff;
            timer_started = true;
        }

        elapsed_time = (timer_get_ticks() & 0x7fffffff) - time_start;
        //TODO - Time does not handle wrap around

        if(short_circuit_flag_set_in_cpu2())
        {   /* error returns
             * returned during switching to this state
             * error came back during reconnecting shared bus
             * very unlikely to reach this if-statement
             * */
            short_circuit_emergency_turn_off(scs_waiting_for_connection);

            /* next state -> shared bus not reconnected */
            state = wait_for_error_to_disappear;
        } else
        {
            answer = connect_other_dpmu_to_shared_bus_answer(&remote_bus_voltage);

            /* check if we got answer
             * -1 -> no answer
             */
            switch(answer)
            {
            case -1: /* no answer */
                break;

            case 0: /* the other DPMU/s is/are still disconnected */
                /* send information to IOP */
                payload_gen_bus_voltage(scs_other_dpmu_still_disconnected, payload);
                canopen_emcy_send_dcbus_short_curcuit(scs_other_dpmu_still_disconnected, payload);

                /* wait for what to do next */
                state = shared_bus_not_reconnected;
                break;

            case 1: /* the other DPMU/s is/are connected */
            {
                bool connect = true;

                /* Voltage level on internal DC Bus */
                float voltage_at_dc_bus = sharedVars_cpu2toCpu1.voltage_at_dc_bus;

                /* check Voltage delta between DPMUs
                 * part of payload in its/their answer
                 */
                if(!( (remote_bus_voltage >= (voltage_at_dc_bus - ACCEPTED_MARGIN_ON_DC_BUS)) &&
                      (remote_bus_voltage <= (voltage_at_dc_bus + ACCEPTED_MARGIN_ON_DC_BUS)) ))
                    connect = false;   /* TODO - Error state */

                if(connect)
                {
                    /* Turn ON QSHARE (GLOAD_3) */
                    cli_switches(IPC_SWITCHES_QSB, SW_ON);

                    /* next state -> shared bus disconnected */
                    state = shared_bus_reconnected;
                } else
                {
                    state = shared_bus_disconnect_after_reconnect;
                }
                break;
            }

            default:
                break;
            }

            /* check timer */
            if(200 < elapsed_time)
            {
                payload_gen_bus_voltage(scs_waiting_for_connection_timeout, payload);
                canopen_emcy_send_dcbus_short_curcuit(scs_waiting_for_connection_timeout, payload);

                /* next state -> shared bus disconnected */
                state = shared_bus_not_reconnected;
            }
        }

        /* if we leave this state we should release the timer */
        if(wait_for_answer_from_other_dpmu_reconnect != state)
            timer_started = false;

        break;
    }

    /* shared bus reconnected at the other DPMU/s
     * wait for error to return
     * non-blocking -> timeout
     * */
    case shared_bus_disconnect_after_reconnect:
    {
        if(!timer_started)
        {
            time_start = timer_get_ticks() & 0x7fffffff;
            timer_started = true;
        }

        elapsed_time = (timer_get_ticks() & 0x7fffffff) - time_start;
        //TODO - Time does not handle wrap around

        /* wait and see if error reappears */
        if(short_circuit_flag_set_in_cpu2())
        {   /* error persist
             * returned during switching to this state
             * error came back during reconnecting shared bus
             * very unlikely to reach this if-statement
             * */
            short_circuit_emergency_turn_off(scs_waiting_for_disconnection);

            /* next state -> shared bus not reconnected */
            state = wait_for_error_to_disappear;
        } else
        {
            /* send disconnect to the other/others DPMU/s, Qsb */
            /* change state if we could send the IPC command to CPU2 */
            if(disconnect_other_dpmu_from_shared_bus())
            {
                /* could not send command to CPU2 */
                state = shared_bus_disconnect_after_reconnect_answer;
                //TODO the other dpmu will return with an answer
            } else
            {
                /* check timer
                 * stop trying send command after timer has expired
                 *  */
                if(200 < elapsed_time)
                    state = shared_bus_disconnect_after_reconnect_answer;
            }
        }

        /* if we leave this state we should release the timer */
        if(wait_for_answer_from_other_dpmu_reconnect != state)
            timer_started = false;

        break;
    }

    /* wait for answer from other DPMU/s if it/they have disconnected from the
     * shared bus
     * wait for error to return
     * non-blocking -> timeout
     * */
    case shared_bus_disconnect_after_reconnect_answer:
    {
        int answer;

        if(!timer_started)
        {
            time_start = timer_get_ticks() & 0x7fffffff;
            timer_started = true;
        }

        elapsed_time = (timer_get_ticks() & 0x7fffffff) - time_start;
        //TODO timer not handling wrap around

        /* wait and see if error reappears */
        if(short_circuit_flag_set_in_cpu2())
        {   /* error returns
             * returned during switching to this state
             * error came back during reconnecting shared bus
             * very unlikely to reach this if-statement
             * */
            short_circuit_emergency_turn_off(scs_waiting_for_disconnection);

            /* next state -> shared bus not reconnected */
            state = wait_for_error_to_disappear;
        } else
        {
            answer = disconnect_other_dpmu_from_shared_bus_answer();

            /* check if we got answer
             * -1 -> no answer
             */
            switch(answer)
            {
            case -1: /* no answer */
                break;

            case 0: /* answer disconnected */
                /* next state -> shared bus disconnected */
                state = clear_error;
                break;

            case 1: /* the other DPMU/s is/are still connected */
                /* send information to IOP */
                payload_gen_bus_voltage(scs_other_dpmu_still_connected, payload);
                canopen_emcy_send_dcbus_short_curcuit(scs_waiting_for_disconnection_timeout, payload);

                /* wait for what to do next */
                state = shared_bus_not_disconnected;
                break;

            default:
                break;
            }

            /* check timer */
            if(200 < elapsed_time)
            {
                /* send information to IOP and wait for what to do next */
                payload_gen_bus_voltage(scs_waiting_for_disconnection_timeout, payload);
                canopen_emcy_send_dcbus_short_curcuit(scs_waiting_for_disconnection_timeout, payload);

                /* next state -> shared bus disconnected */
                state = shared_bus_not_disconnected;
            }
        }

        /* if we leave this state we should release the timer */
        if(wait_for_answer_from_other_dpmu_disconnect != state)
            timer_started = false;

        break;
    }

    /* shared bus reconnected at the other DPMU/s
     * wait for error to return
     * non-blocking -> timeout
     * */
    case shared_bus_reconnected:
        if(!timer_started)
        {
            time_start = timer_get_ticks() & 0x7fffffff;
            timer_started = true;
        }

        elapsed_time = (timer_get_ticks() & 0x7fffffff) - time_start;
        //TODO - Time does not handle wrap around

        /* wait and see if error reappears */
        if(short_circuit_flag_set_in_cpu2())
        {   /* error returns
             * continue without shared bus / other DPMU
             * */
            /* Turn OFF QSHARE (GLOAD_3) */
            cli_switches(IPC_SWITCHES_QSB, SW_OFF);

            /* send disconnect to the other/others DPMU/s, Qsb */
            /* change state if we could send the IPC command to CPU2 */
            if(disconnect_other_dpmu_from_shared_bus())
            {
                /* could not send command to CPU2 */
                state = shared_bus_not_reconnected;
                //TODO the other dpmu will return with an answer
            } else
            {
                /* check timer
                 * stop trying send command after timer has expired
                 *  */
                if(100 < elapsed_time)
                    state = shared_bus_not_reconnected;
            }
        } else
        {
            /* check timeout */
            if(200 < elapsed_time)
            {   /* problem resolved */
                /* next state -> clear error */
                state = clear_error;
            }
        }

        /* if we leave this state we should release the timer */
        if(wait_for_answer_from_other_dpmu_reconnect != state)
            timer_started = false;

        break;

    /* shared bus not reconnected at the other DPMU/s
     * continue with reduced effect
     * */
    case shared_bus_not_reconnected:
        //TODO Mark as for continuing with reduced effect
        //     Probably as signal to CPU2 to change some parameter
//        sharedVars_cpu1toCpu2.available_power_budget_dc_input = xxx;

//        canopen_emcy_send_power_sharing_error(1);

        if(short_circuit_flag_set_in_cpu2())
            state = wait_for_error_to_disappear;
        else
            state = clear_error;

        /* mark handled */
        global_error_code_handled |= (1UL << ERROR_BUS_SHORT_CIRCUIT);

        break;

    /* problem solved
     * clear error
     * */
    case clear_error:
        /* problem resolved */
        /* send EMCY CLEAR - send it once */
        if(global_error_code_sent & (1UL << ERROR_BUS_SHORT_CIRCUIT))
        {
            payload_gen_bus_voltage(scs_no_error, payload);
            canopen_emcy_send_dcbus_short_curcuit(scs_no_error, payload);
        }

        /* mark as unhandled - nothing need to be done  */
        global_error_code_sent    &= ~(1UL << ERROR_BUS_SHORT_CIRCUIT);
        global_error_code_handled &= ~(1UL << ERROR_BUS_SHORT_CIRCUIT);

        /* clear the timer */
        timer_started = false;

        /* next state -> no short circuit */
        state = no_short_circuit;
        break;

    /* nothing more to do
     * wait for the error to disappear
     * meanwhile, the IOP can give instructions and follow what happens
     * */
    case wait_for_error_to_disappear:
        /* there is nothing more we can do
         * let IOP decide next step
         * */

        if(!short_circuit_flag_set_in_cpu2())
        {   /* problem resolved */
            /* next state -> clear error */
            state = clear_error;
        }
        break;
    default:
        break;
    }
}

/* brief: check if Current output to load is above max
 *
 * details: checks error flag
 *          turns off switches and DCDC
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
            /* disconnect all switches
             * turn off regulation */
//            IPC_setFlagLtoR(IPC_CPU1_L_CPU2_R, IPC_CPU1_REQUIERS_EMERGECY_SHUT_DOWN);

            /* mark handled */
            global_error_code_handled |= (1UL << ERROR_LOAD_OVER_CURRENT);

            /* send EMCY - send it once */
            canopen_emcy_send_load_overcurrent(1);

            /* mark it sent */
            global_error_code_sent    |= (1UL << ERROR_LOAD_OVER_CURRENT);
            global_error_code_handled |= (1UL << ERROR_LOAD_OVER_CURRENT);

            Serial_debug(DEBUG_INFO, &cli_serial, "************* Load over current detected\r\n");
        } else
        {
            ;
            /* there is nothing more we can do
             * let IOP decide next step
             * */
        }
    } else
    {
        /* send EMCY CLEAR - send it once */
        if(global_error_code_sent & (1 << ERROR_LOAD_OVER_CURRENT)) {
            canopen_emcy_send_load_overcurrent(0);
            Serial_debug(DEBUG_INFO, &cli_serial, "************* Load over current clear\r\n");
        }

        /* mark as unhandled - nothing need to be done  */
        global_error_code_sent    &= ~(1UL << ERROR_LOAD_OVER_CURRENT);
        global_error_code_handled &= ~(1UL << ERROR_LOAD_OVER_CURRENT);
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

/* brief: check if there are external power failures
 *
 * details: checks error flags
 *          turns off switches and DCDC
 *          signal IOP with EMCY
 *          one power line failure
 *          two power line failures
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
 * Skriv om så att man:
 *  - testar ena linan för sig
 *      och subtraherar dess effekt från tillgänglig effekt
 *      vid återgång återställer man effekten
 *  - testar andra linan för sig
 *      och subtraherar dess effekt från tillgänglig effekt
 *      vid återgång återställer man effekten
 *  - om båda är kaputt går man till nödläge
 */
static void error_power_line_failure(void)
{
    uint16_t pwr_loss_error = 0;

    if(global_error_code & (1UL << ERROR_EXT_PWR_LOSS_MAIN))
        pwr_loss_error |= 0b01;
    if(global_error_code & (1UL << ERROR_EXT_PWR_LOSS_OTHER))
        pwr_loss_error |= 0b10;

    switch(pwr_loss_error)
    {
    case 0: /* no pwr loss error */
        sharedVars_cpu1toCpu2.use_power_budget_dc_input = 1;
        sharedVars_cpu1toCpu2.use_power_budget_dc_shared = 1;
        sharedVars_cpu1toCpu2.safe_parking_allowed = 0;

        /* clear flag */
        global_error_code &= ~(1UL << ERROR_EXT_PWR_LOSS_BOTH);
        break;

    case 1: /* we have lost power from our INPUT bus */
        sharedVars_cpu1toCpu2.use_power_budget_dc_input = 0;
        sharedVars_cpu1toCpu2.use_power_budget_dc_shared = 1;
        sharedVars_cpu1toCpu2.safe_parking_allowed = 0;

        /* clear flag */
        global_error_code &= ~(1UL << ERROR_EXT_PWR_LOSS_BOTH);
        break;

    case 2: /* the other DPMU have lost power from its INPUT bus */
        sharedVars_cpu1toCpu2.use_power_budget_dc_input = 1;
        sharedVars_cpu1toCpu2.use_power_budget_dc_shared = 0;
        sharedVars_cpu1toCpu2.safe_parking_allowed = 0;

        /* clear flag */
        global_error_code &= ~(1UL << ERROR_EXT_PWR_LOSS_BOTH);
        break;

    case 3: /* both  we and the other DPMU have lost power from our INPUT buses */
        sharedVars_cpu1toCpu2.use_power_budget_dc_input = 0;
        sharedVars_cpu1toCpu2.use_power_budget_dc_shared = 0;
        sharedVars_cpu1toCpu2.safe_parking_allowed = 1;

        /* set flag */
        global_error_code |= (1UL << ERROR_EXT_PWR_LOSS_BOTH);
        break;
    }

    if(pwr_loss_error == 0b01)
    {
        if(!(global_error_code_handled & (1UL << ERROR_EXT_PWR_LOSS_MAIN)))
        {
            /* send EMCY - send it once */
            canopen_emcy_send_power_line_failure(1);

            /* mark it sent */
            global_error_code_sent |= (1UL << ERROR_EXT_PWR_LOSS_MAIN);

            /* mark handled */
            global_error_code_handled |= (1UL << ERROR_EXT_PWR_LOSS_MAIN);
        } else
        {
            /* there is nothing more we can do
             * let IOP decide next step
             * till then, continue as normal, but with reduced effect
             * */
        }
    } else
    {
        /* send EMCY CLEAR - send it once */
        if(global_error_code_sent & (1UL << ERROR_EXT_PWR_LOSS_MAIN))
            canopen_emcy_send_power_line_failure(0);

        global_error_code_sent    &= ~(1UL << ERROR_EXT_PWR_LOSS_MAIN);
        global_error_code_handled &= ~(1UL << ERROR_EXT_PWR_LOSS_MAIN);
    }

    if(pwr_loss_error == 0b10)
    {
        if(!(global_error_code_handled & (1UL << ERROR_EXT_PWR_LOSS_OTHER)))
        {
            /* send EMCY - send it once */
            canopen_emcy_send_power_line_failure(1);

            /* mark it sent */
            global_error_code_sent |= (1UL << ERROR_EXT_PWR_LOSS_OTHER);

            /* mark handled */
            global_error_code_handled |= (1UL << ERROR_EXT_PWR_LOSS_OTHER);
        } else
        {
            /* there is nothing more we can do
             * let IOP decide next step
             * till then, continue as normal, but with reduced effect
             * */
        }
    } else
    {
        /* send EMCY CLEAR - send it once */
        if(global_error_code_sent & (1UL << ERROR_EXT_PWR_LOSS_OTHER))
            canopen_emcy_send_power_line_failure(0);

        global_error_code_sent    &= ~(1UL << ERROR_EXT_PWR_LOSS_OTHER);
        global_error_code_handled &= ~(1UL << ERROR_EXT_PWR_LOSS_OTHER);
    }

    if(pwr_loss_error == 0b11)
    {
        if(!(global_error_code_handled & (1UL << ERROR_EXT_PWR_LOSS_BOTH)))
        {
            /* send EMCY - send it once */
            canopen_emcy_send_power_line_failure_both(1);

            /* mark it sent */
            global_error_code_sent |= (1UL << ERROR_EXT_PWR_LOSS_BOTH);

            /* mark handled */
            global_error_code_handled |= (1UL << ERROR_EXT_PWR_LOSS_BOTH);
        } else
        {
            ;
            /* there is nothing more we can do
             * let IOP decide next step
             * */
        }
    } else
    {
        /* send EMCY CLEAR - send it once */
        if(global_error_code_sent & (1UL << ERROR_EXT_PWR_LOSS_BOTH))
            canopen_emcy_send_power_line_failure_both(0);

        /* mark as unhandled - nothing need to be done  */
        global_error_code_sent    &= ~(1UL << ERROR_EXT_PWR_LOSS_BOTH);
        global_error_code_handled &= ~(1UL << ERROR_EXT_PWR_LOSS_BOTH);
    }
}

/* brief: check State of Charge error
 *
 * details: checks error flag
 *          turns off switches and DCDC
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
static void error_state_of_charge(void)
{
    static bool timer_started = false;
    static uint32_t time_start;

    if(global_error_code & (1UL << ERROR_SOC_BELOW_SAFETY_THRESHOLD))
    {
        if(!(global_error_code_handled & (1UL << ERROR_SOC_BELOW_SAFETY_THRESHOLD)))
        {
            if(!timer_started)
            {
                time_start = timer_get_ticks() & 0x7fffffff;
                timer_started = true;
            }

            /* ms ticks, enough with 127 ms */
            int16_t elapsed_time = (timer_get_ticks() & 0x7fffffff) - time_start;
            //TODO - Time does not handle wrap around

            /* a short timeout might be needed
             * in the meaning or inaccuracy of measurement
             * send if enough measurements/mean value is giving this error */

            if(100 < elapsed_time)
            {
                /* mark handled */
                global_error_code_handled |= (1UL << ERROR_SOC_BELOW_SAFETY_THRESHOLD);

                /* send EMCY */
                canopen_emcy_send_state_of_charge_safety_error(1, sharedVars_cpu2toCpu1.remaining_energy_to_min_soc_energy_bank);

                /* mark it sent */
                global_error_code_sent |= (1UL << ERROR_SOC_BELOW_SAFETY_THRESHOLD);

                /* mark handled */
                global_error_code_handled |= (1UL << ERROR_SOC_BELOW_SAFETY_THRESHOLD);

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
        /* send EMCY CLEAR - send it once */
        if(global_error_code_sent & (1UL << ERROR_SOC_BELOW_SAFETY_THRESHOLD))
            canopen_emcy_send_state_of_charge_safety_error(0, sharedVars_cpu2toCpu1.soc_energy_bank);

        /* mark as unhandled - nothing need to be done  */
        global_error_code_sent    &= ~(1UL << ERROR_SOC_BELOW_SAFETY_THRESHOLD);
        global_error_code_handled &= ~(1UL << ERROR_SOC_BELOW_SAFETY_THRESHOLD);

        /* clear the timer */
        timer_started = false;
    }

    /* ERROR_SOC_BELOW_LIMIT
     * not enough Voltage for boosting to S_TARGET_VOLTAGE_AT_DC_BUS
     * */
    if(global_error_code & (1UL << ERROR_SOC_BELOW_LIMIT))
    {
        if(!(global_error_code_handled & (1UL << ERROR_SOC_BELOW_LIMIT)))
        {
            bool error_persist = false;

            /* test error */
            /*TODO Test if error persist
             * in the meaning or inaccuracy of measurement
             * send if enough measurements/mean value is giving this error
             * */

            if(error_persist)
            {
                /* mark handled */
                global_error_code_handled |= (1UL << ERROR_SOC_BELOW_LIMIT);

                /* send EMCY */
                //TODO change to Voltage over energy bank
                canopen_emcy_send_state_of_charge_min_error(1, sharedVars_cpu2toCpu1.soc_energy_bank);

                /* mark it sent */
                global_error_code_sent |= (1UL << ERROR_SOC_BELOW_LIMIT);
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
        if(global_error_code_sent & (1UL << ERROR_SOC_BELOW_LIMIT))
            canopen_emcy_send_state_of_charge_min_error(0, sharedVars_cpu2toCpu1.soc_energy_bank);

        /* mark as unhandled - nothing need to be done  */
        global_error_code_sent    &= ~(1UL << ERROR_SOC_BELOW_LIMIT);
        global_error_code_handled &= ~(1UL << ERROR_SOC_BELOW_LIMIT);
    }
}

/* brief: check if error_flag is set
 *
 * details: checks error flag
 *          turns off switches and DCDC
 *          signal IOP with EMCY
 *
 * requirements:
 *
 * argument: error_flag - the error flag to check
 *                        the bit number (1 << error_flag)
 *           timeout    - the time needed to disregard any short transients
 *                        zero if not used
 *           canopen_emcy_message_func - name of the function that will send the
 *                                       CANopen EMCY message
 *           payload_gen_func - name of function that will generate the payload,
 *                              last four Bytes of the CANopen EMCY message,
 *                              NULL if not used
 *
 * return: none
 *
 * note: non-blocking
 *
 * presumptions:
 *
 */
bool timer_started[NR_OF_ERROR_CODES];
uint32_t time_start[NR_OF_ERROR_CODES];
static void error_check_error_flag_generic(uint32_t error_flag,
                                           uint8_t emcy_error_byte,
                                           uint16_t timeout,
           void (*canopen_emcy_message_func)(uint16_t emcy_error_code,
                                             uint8_t emcy_error_byte,
                                             uint8_t status,
                                             uint8_t payload[4]),
           uint16_t emcy_error_code,
           void (*payload_gen_func)(uint8_t status, uint8_t payload[4]))
{
    uint8_t pay_load[4];

    if(global_error_code & (1UL << error_flag))
    {
        if(!(global_error_code_handled & (1UL << error_flag)))
        {
            if(!timer_started[error_flag])
            {
                time_start[error_flag] = timer_get_ticks() & 0x7fffffff;
                timer_started[error_flag] = true;
            }

            /* ms ticks, enough with 127 ms */
            uint32_t elapsed_time = (timer_get_ticks() & 0x7fffffff) - time_start[error_flag];
            //TODO - Timer does not handle wrap around

            /* send EMCY - send it once */
            if(!(global_error_code_sent & (1UL << error_flag)))
            {
                /* disconnect load */
                cli_switches(IPC_SWITCHES_QLB, SW_OFF);

                /* generate necessary payload */
                payload_gen_func(1, pay_load);

                /* this function will handle any extra checks and/or add
                 * status Bytes in the last four Bytes of the CANopen EMCY
                 * payload */
                canopen_emcy_message_func(emcy_error_code, emcy_error_byte, 1, pay_load);
//                canopen_emcy_send_dcbus_over_voltage(1);

                /* mark it sent */
                global_error_code_sent |= (1UL << error_flag);
            }

            //TODO what shall the timeout be? in milliseconds
            /* check timeout */
            if(timeout <= elapsed_time)
            {
                /* disconnect all switches
                 * turn off regulation */
                IPC_setFlagLtoR(IPC_CPU1_L_CPU2_R, IPC_CPU1_REQUIERS_EMERGECY_SHUT_DOWN);

                /* mark handled */
                global_error_code_handled |= (1UL << error_flag);
            }
        } else
        {
            ;
            /* there is nothing more we can do
             * let IOP decide next step
             * */

            /* clear the timer */
            timer_started[error_flag] = false;
        }
    } else
    {
        /* send EMCY CLEAR - send it once */
        if(global_error_code_sent & (1UL << error_flag))
        {
            /* generate necessary payload */
            payload_gen_func(0, pay_load);

            canopen_emcy_message_func(emcy_error_code, emcy_error_byte, 0, pay_load);
        }

        /* mark as unhandled - nothing need to be done  */
        global_error_code_sent    &= ~(1UL << error_flag);
        global_error_code_handled &= ~(1UL << error_flag);

        /* clear the timer */
        timer_started[error_flag] = false;
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
//        test_update_of_error_codes = false;

        error_copy_error_codes_from_CPU2();
        error_load_overcurrent();
//
//    error_dcbus_over_voltage();
//    error_check_error_flag_generic(ERROR_BUS_OVER_VOLTAGE,
//                                   EMCY_ERROR_CODE_VOLTAGE,
//                                   2,
//                                   canopen_emcy_send_generic,
//                                   EMCY_ERROR_BUS_OVER_VOLTAGE,
//                                   payload_gen_bus_voltage);
//
//    error_dcbus_under_voltage();
////    error_check_error_flag_generic(ERROR_BUS_UNDER_VOLTAGE,
////                                   EMCY_ERROR_CODE_VOLTAGE,
////                                   0,
////                                   canopen_emcy_send_generic,
////                                   EMCY_ERROR_BUS_OVER_VOLTAGE,
////                                   payload_gen_bus_over_voltage);
//
//    error_dcbus_short_circuit();


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
