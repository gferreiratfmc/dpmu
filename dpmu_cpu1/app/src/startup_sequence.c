/*
 * startup_sequence.c
 *
 *  Created on: 25 nov. 2022
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#include "stdbool.h"
#include "stdint.h"

//#include "charge.h"
#include "cli_cpu1.h"
#include "co.h"
#include "co_canopen.h"
#include "common.h"
#include "convert.h"
#include "dpmu_type.h"
#include "error_handling.h"
#include "gen_indices.h"
#include "log.h"
#include "main.h"
#include "startup_sequence.h"
#include "timer.h"
#include "usr_401.h"
#include "watchdog.h"

#define INITIALIZATION_DONE 1       /* TODO - T.B.D */
#define DISCHARGE_TIMEOUT   1000UL  /* 1 second at tick rate 1 millisecond */    /* TODO - T.B.D */

static void enable_inrush()
{
    /* execute in-rush limiter
     * wait for the other command to finish in CPU2, ACK in IPC flag
     * */
//    cli_switches(IPC_SWITCHES_QIRS, SW_ON);
    /* set the state machine of the controlling algorithm to SOFTSTARTINIT */
    sharedVars_cpu1toCpu2.iop_operation_request_state = 2; //TODO change to proper common "enum variable"
    IPC_setFlagLtoR(IPC_CPU1_L_CPU2_R, IPC_IOP_REQUEST_CHANGE_OF_STATE);
}

static void wait_for_inrush_to_complete()
{
    int old_time = timer_get_seconds();
    int i = 20;

    /* wait for inrush current limiter to finish
     * wait maximum 'i' seconds
     * */
    while (i && (!IPC_isFlagBusyRtoL(IPC_CPU1_L_CPU2_R, IPC_SWITCHES_QIRS_DONE)))
    {
        /* keep communication:
         *  with IOP in case of CPU2 locking error
         *  with UART for possible debugging
         *  with CPU2 in case of "message handling"
         */
        cli_check_for_new_commands_from_UART(); /* commands through UART */

        /* waiting for command from CPU2
         * will receive the execution time from CPU2 */
        check_cpu2_dbg();                       /* debug messages from CPU2 */

        while (coCommTask() == CO_TRUE)
            ;

        /* do not feed the WD more often than ones a second
         * preparation for using windowed WD
         * */
        int new_time = timer_get_seconds();
        if(old_time != new_time)
        {
            old_time = new_time;
            watchdog_feed();
            i--;
        }
    }
    IPC_ackFlagRtoL(IPC_CPU1_L_CPU2_R, IPC_SWITCHES_QIRS_DONE);
}

static inline void disable_inrush(void)
{
    //cli_switches_comm(IPC_SWITCHES_QIRS, SW_OFF);
}

/* brief: connects with other DPMU through the shared bus, Qsb
 *
 * details: sends request to other DPMU to connect to its shared bus
 *          checks the answer from the other DPMU
 *          if okay, set Qsb to conductive mode
 *
 * requirements:
 *
 * argument: none
 *
 * return: none
 *
 * note:
 *
 * presumptions: shared bus is disconnected, Qsb is non-conductive
 */
bool connect_shared_bus(bool got_remote_voltage, uint32_t remote_voltage)
{
    bool connect = true;

    if(!dpmu_type_allowed_to_use_shared_bus())
        return false;

    //TODO update with correct values
    //TODO update with correct transformation of ADC <=> VOltage
    //TODO update logic according to above both TODOs

    /* Voltage level on internal DC Bus */
    float voltage_at_dc_bus = sharedVars_cpu2toCpu1.voltage_at_dc_bus;

    /* check voltage level of DC Bus on redundant DPMU */
    uint8_t voltage_at_dc_bus_reduntant;
    if(got_remote_voltage)
        voltage_at_dc_bus_reduntant = remote_voltage;
//    else
//    //TODO add asking of voltage at DC bus on redundant DPMU, handled over CANB */
//    voltage_at_dc_bus_reduntant = other_dpmu_read_voltage();

    if(!( (voltage_at_dc_bus_reduntant >= (voltage_at_dc_bus - ACCEPTED_MARGIN_ON_DC_BUS)) &&
          (voltage_at_dc_bus_reduntant <= (voltage_at_dc_bus + ACCEPTED_MARGIN_ON_DC_BUS)) ))
        connect = false;   /* TODO - Error state */

    if(connect)
    {
        /* connect the other/others DPMU/s, Qsb */
        //TODO inform other DPMU to connect its share
        //TODO check answer from other DPMU
//        if(other_dpmu_connect_share_bus())
//        {
            cli_switches(IPC_SWITCHES_QSB, SW_ON);

            /* check if load switch is in conductive mode */
            if(!(switches_read_states() & (1 << SWITCHES_QLB))) {
                ;   /* TODO - Error state */
            }
//        } else
//        {
//            /* do nothing !!
//             * Other DPMU is having some kind of problem
//             * if it cannot connect to the shared bus
//             */
//
//            connect = false;
//        }
    }
    /* no 'else' statement here, should have been non-conductive before */

    return connect;
}

void connect_load_bus()
{
    /* check if we are allowed to use the LOAD bus */
    if (dpmu_type_allowed_to_use_load_bus())
    {
        /* connect load bus */
        //cli_switches_comm(IPC_SWITCHES_QLB, SW_ON);
    }
}

/* brief: connects with other DPMU through the shared bus, Qsb
 *
 * details: sends request to other DPMU to connect to its shared bus
 *          checks the answer from the other DPMU
 *          if not okay, set Qsb to non-conductive mode
 *
 * requirements:
 *
 * argument: none
 *
 * return: none
 *
 * note:
 *
 * presumptions: shared bus is connected, Qsb is conductive
 */
bool connect_other_dpmu(void)
{
    bool time_start = false;
    int16_t elapsed_time;

    static uint8_t connected = true;
    int answer;
    float remote_bus_voltage = 0;

    if(!dpmu_type_allowed_to_use_shared_bus())
    {
        /* we are of a type of DPMU that is not allowed to
         * be connected to the shared bus - disconnect
         * */
        //cli_switches_comm(IPC_SWITCHES_QSB, SW_OFF);

        return false;
    }

    time_start = timer_get_ticks() & 0x7fffffff;

    /* try to reconnect and see if error returns */
    /* change state if we could send the IPC command to CPU2 */
    while(!connect_other_dpmu_to_shared_bus())
    {
        elapsed_time = (timer_get_ticks() & 0x7fffffff) - time_start;
        //TODO - Time does not handle wrap around

        /* check timeout */
        if(200 < elapsed_time)
        {   /* we can't wait forever for this message to be sent
             * continue anyway
             */
            connected = false;
            break;
        }
    }

    if(connected)
    {
        /* restart the timer */
        time_start = timer_get_ticks() & 0x7fffffff;

        while(-1 == (answer = connect_other_dpmu_to_shared_bus_answer(&remote_bus_voltage)))
        {
            /* check timeout */
            if(200 < elapsed_time)
            {   /* we can't wait forever for this message to be sent
                 * continue to next state anyway
                 */
                break;
            }
        }

        /* other DPMU is connected */
        if(1 == answer)
        {
            /* we do nothing
             *
             * the other DPMU should have compared the Voltage levels
             * on the DC buses for both units and only have connected
             * if the difference were within tolerance
             *
             * we sent our Voltage level in the request to connect
             */
        }

        /* other DPMU is not connected */
        if(0 == answer)
        {
            /* the other DPMU could not connect
             * as a precaution we disconnect
             * */
            //cli_switches_comm(IPC_SWITCHES_QSB, SW_OFF);
            connected = false;
        }

        /* we got no answer */
        if(-1 == answer)
        {
            /* we could not get a reply
             * as a precaution we disconnect
             * */
            //cli_switches_comm(IPC_SWITCHES_QSB, SW_OFF);
        }
    }

    return connected;
}

void startup_sequence(void)
{
    int old_time = timer_get_seconds();

    /* save timer start value for discharging load */
    uint32_t load_discharge_timer_start_value = timer_get_ticks();

    /* set the state machine of the controlling algorithm to INITIALIZATION */
    /* should already be in this state, do it anyway! */
    //TODO should this be a emergency turn off to guarantee everything is turned off, except CANB
    sharedVars_cpu1toCpu2.iop_operation_request_state = 1; //TODO change to proper common "enum variable"
    IPC_setFlagLtoR(IPC_CPU1_L_CPU2_R, IPC_IOP_REQUEST_CHANGE_OF_STATE);

    /*** retrieve initialization data from IOP ***/
    do {
        cli_check_for_new_commands_from_UART(); /* commands through UART */

        check_cpu2_dbg();                       /* debug messages from CPU" */

        while (coCommTask() == CO_TRUE);

        /* do not feed the WD more often than ones a second
         * preparation for using windowed WD
         * */
        int new_time = timer_get_seconds();
        if(old_time != new_time)
        {
            old_time = new_time;
            watchdog_feed();
        }
    } while(CO_NMT_STATE_OPERATIONAL != coNmtGetState());

    /* check if we are using battery */
    if(dpmu_type_having_battery())
        sharedVars_cpu1toCpu2.having_battery = true;
    else
        sharedVars_cpu1toCpu2.having_battery = false;

    /* wait predefined time for letting the Load to be discharged, ticks [ms] */
    while((timer_get_ticks() - load_discharge_timer_start_value) < DISCHARGE_TIMEOUT);

    /*** load is now supposed to be discharged ***/
    /*   if not, increase DISCHARGE_TIMEOUT      */

    /* connect load bus */
    connect_load_bus();

    /* connect shared bus */
    //cli_switches_comm(IPC_SWITCHES_QSB, SW_ON);

    /* check if load switch is in conductive mode */
    if (!(switches_read_states() & (1 << SWITCHES_QLB)))
    {
        ; /* TODO - Error state */
    }

    /* execute in-rush limiter
     * wait for the other command to finish in CPU2, ACK in IPC flag
     * */
    enable_inrush();

    /* wait for inrush current limiter to finish */
    wait_for_inrush_to_complete();

    /*** DC bus and all capacitances needed is now charged to TARGET_VOLTAGE_AT_DC_BUS ***/

    /* connect input bus
     * wait for the other command to finish in CPU2, ACK in IPC flag
     * */
//    while(IPC_isFlagBusyRtoL(IPC_CPU1_L_CPU2_R, IPC_FLAG_MESSAGE_CPU1_TO_CPU2));
//    {
//        /* for debug propose, execution time will be sent from CPU2 */
//        check_cpu2_dbg();
//    }
//    while(!cli_switches(IPC_SWITCHES_QINB, SW_ON));
    cli_switches_comm(IPC_SWITCHES_QINB, SW_ON);
    //TODO race condition with CPU2

    /* check if input switch is in conductive mode */
//    while(IPC_isFlagBusyRtoL(IPC_CPU1_L_CPU2_R, IPC_FLAG_MESSAGE_CPU1_TO_CPU2));
//    {
//        /* for debug propose, execution time will be sent from CPU2 */
//        check_cpu2_dbg();
//    }
//    IPC_waitForAck(IPC_CPU1_L_CPU2_R, IPC_FLAG_MESSAGE_CPU1_TO_CPU2);
    if(! (switches_read_states() & (1 << SWITCHES_QINB)) ) {
        ;   /* TODO - Error state */
    }

    /* disconnect in-rush circuitry */
    /* might have been done in CPU2, do it here anyway! */
    disable_inrush();

    /* set the state machine of the controlling algorithm to INITIALIZATION */
    sharedVars_cpu1toCpu2.iop_operation_request_state = 1; //TODO change to proper common "enum variable"
    IPC_setFlagLtoR(IPC_CPU1_L_CPU2_R, IPC_IOP_REQUEST_CHANGE_OF_STATE);

    /* ask other DPMU to connect to the shared bus */
//    connect_shared_bus(false, 0);
    connect_other_dpmu();
}
