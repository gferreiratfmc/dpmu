/*
 * error_codes.h
 *
 *  Created on: 14 nov. 2022
 *      Author: vb
 */

#ifndef COAPPL_ERROR_CODES_H_
#define COAPPL_ERROR_CODES_H_


#include <stdbool.h>
#include <stdint.h>

#include "type_common.h"



#define EMCY_ERROR_NO_ERRORS                    0x0000 /* not used */
#define EMCY_ERROR_OVER_CURRENT_INPUT           0x2100 /* not used */
#define EMCY_ERROR_OVER_CURRENT                 0x2200
#define EMCY_ERROR_OVER_CURRENT_LOAD            0x2300 /* not used */
#define EMCY_ERROR_BUS_SHORT_CIRCUIT            0x3001
#define EMCY_ERROR_BUS_UNDER_VOLTAGE            0x3002
#define EMCY_ERROR_BUS_OVER_VOLTAGE             0x3003
#define EMCY_ERROR_VOLTAGE_BALANCING            0x3200 /* not used */
#define EMCY_ERROR_TEMPERATURE                  0x4200
#define EMCY_ERROR_LOGGING                      0x6001 /* not used */
#define EMCY_ERROR_SYSTEM_SHUTDOWN              0x6002 /* not used */
#define EMCY_ERROR_REBOOT_WARNING               0x6003 /* not used */
#define EMCY_ERROR_SYSTEM_INITIALIZATION        0x6004 /* not used */
#define EMCY_ERROR_FIRMWARE_UPGRADE             0x6005 /* not used */
#define EMCY_ERROR_CAN_OVERRUN                  0x8110 /* not used */
#define EMCY_ERROR_CAN_PASSIVE                  0x8111 /* not used */
#define EMCY_ERROR_CAN_BUS_OFF                  0x8112 /* not used */
#define EMCY_ERROR_HEARTBEAT                    0x8113 /* not used */
#define EMCY_ERROR_CANB_GENERAL                 0x8120 /* not used */
#define EMCY_ERROR_BUCK_INIT                    0xFF02 /* not used */
#define EMCY_ERROR_BUCK                         0xFF03 /* not used */
#define EMCY_ERROR_BOOST_INIT                   0xFF04 /* not used */
#define EMCY_ERROR_BOOST                        0xFF05 /* not used */
#define EMCY_ERROR_REGENERATE_INIT              0xFF06 /* not used */
#define EMCY_ERROR_REGENERATE                   0xFF07 /* not used */
#define EMCY_ERROR_PULSE_INIT                   0xFF09 /* not used */
#define EMCY_ERROR_PULSE                        0xFF0A /* not used */
#define EMCY_ERROR_BALANCING_INIT               0xFF0C /* not used */
#define EMCY_ERROR_BALANCING                    0xFF0D /* not used */
#define EMCY_ERROR_OPERATIONAL                  0xF105 /* not used */
#define EMCY_ERROR_POWER_SHARING                0xFF16 /* not used */
#define EMCY_ERROR_EXT_PWR_LOSS                 0xFF17 /* not used */
#define EMCY_ERROR_EXT_PWR_LOSS_BOTH            0xFF18 /* not used */
#define EMCY_ERROR_INPUT_POWER_TO_HIGH          0xFF19 /* not used */
#define EMCY_ERROR_CONSUMED_POWER_TO_HIGH       0xFF1A /* not used */
#define EMCY_ERROR_SWITCHING                    0xFF20 /* not used */
#define EMCY_ERROR_SOC_BELOW_LIMIT              0xFF30 /* not used */
#define EMCY_ERROR_SOC_BELOW_SAFETY_THRESHOLD   0xFF31 /* not used */


typedef enum error_codes {
    ERROR_NO_ERRORS = 0,
    ERROR_CONSUMED_POWER_TO_HIGH,
    ERROR_INPUT_POWER_TO_HIGH,
    ERROR_LOAD_OVER_CURRENT,
    ERROR_BUS_SHORT_CIRCUIT,
    ERROR_BUS_UNDER_VOLTAGE,
    ERROR_BUS_OVER_VOLTAGE,
    ERROR_VOLTAGE_BALANCING,
    ERROR_HIGH_TEMPERATURE,
    ERROR_OVER_TEMPERATURE,
    ERROR_SOC_BELOW_LIMIT,
    ERROR_SOC_BELOW_SAFETY_THRESHOLD,
    ERROR_CELL_SOC_BELOW_LIMIT,
    ERROR_SYSTEM_SHUTDOWN,
    ERROR_DISCHARGING,
    ERROR_PRE_CHARGING,
    ERROR_CHARGING,
    ERROR_BALANCING,
    ERROR_OPERATIONAL,
    ERROR_POWER_SHARING,
    ERROR_EXT_PWR_LOSS_MAIN,
    ERROR_EXT_PWR_LOSS_OTHER,
    ERROR_EXT_PWR_LOSS_BOTH,
    ERROR_SUPERCAP_SHORT_CIRCUIT,
    NR_OF_ERROR_CODES   /* MUST BE THE LAST ONE */
} error_codes_t;


extern uint32_t global_error_code;
extern uint32_t error_code_CPU1;

bool connect_other_dpmu_to_shared_bus(void);
int8_t connect_other_dpmu_to_shared_bus_answer(float *remote_bus_voltage);
void error_check_for_errors(void);
static void error_copy_error_codes_from_CPU1_and_CPU2(void);
static void error_dcbus_short_circuit(void);
static void error_load_overcurrent(void);
static void error_no_error(void);
static void error_dcbus_over_voltage(void);
static void error_dcbus_under_voltage(void);
static void error_system_temperature(void);


//enum short_circuit_states{
//    no_short_circuit = 0,
//    wait_for_answer_from_other_dpmu_disconnect,
//    shared_bus_disconnected,
//    shared_bus_not_disconnected,
//    reconnect_shared_bus,
//    wait_for_answer_from_other_dpmu_reconnect,
//    shared_bus_reconnected,
//    shared_bus_disconnect_after_reconnect,
//    shared_bus_disconnect_after_reconnect_answer,
//    shared_bus_not_reconnected,
//    clear_error,
//    wait_for_error_to_disappear,
//};

//enum short_circuit_status{
//    scs_no_error,
//    scs_not_allowed_to_connect_sb,
//    scs_short_circuit,
//    scs_waiting_for_disconnection,
//    scs_waiting_for_disconnection_timeout,
//    scs_other_dpmu_not_disconnected,
//    scs_other_dpmu_still_connected,
//    scs_after_disconnection,
//    scs_waiting_for_connection,
//    scs_waiting_for_connection_timeout,
//    scs_other_dpmu_still_disconnected,
//    scs_other_dpmu_after_reconnection,
//};

#endif /* COAPPL_ERROR_CODES_H_ */
