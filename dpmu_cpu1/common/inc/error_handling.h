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
    NR_OF_ERROR_CODES   /* MUST BE THE LAST ONE */
} error_codes_t;

#define EMCY_ERROR_NO_ERRORS                    0x0000 /* not used */
#define EMCY_ERROR_OVER_CURRENT_INPUT           0x2100 /* not used */
#define EMCY_ERROR_OVER_CURRENT                 0x2200
#define EMCY_ERROR_OVER_CURRENT_LOAD            0x2300 /* not used */
#define EMCY_ERROR_BUS_SHORT_CIRCUIT            0x3001
#define EMCY_ERROR_BUS_UNDER_VOLTAGE            0x3002
#define EMCY_ERROR_BUS_OVER_VOLTAGE             0x3003
#define EMCY_ERROR_VOLTAGE_BALANCING            0x3200 /* not used */
#define EMCY_ERROR_TEMPERATURE                  0x4200
#define EMCY_ERROR_LOGGING                      0x6001 /* not used, used in EMOTAS stack*/
#define EMCY_ERROR_SYSTEM_SHUTDOWN              0x6002 /* not used */
#define EMCY_ERROR_REBOOT_WARNING               0x6003
#define EMCY_ERROR_SYSTEM_INITIALIZATION        0x6004 /* not used */
#define EMCY_ERROR_FIRMWARE_UPGRADE             0x6005 /* not used */
#define EMCY_ERROR_CAN_OVERRUN                  0x8110 /* not used */
#define EMCY_ERROR_CAN_PASSIVE                  0x8111 /* not used */
#define EMCY_ERROR_CAN_BUS_OFF                  0x8112 /* not used */
#define EMCY_ERROR_HEARTBEAT                    0x8113 /* not used */
#define EMCY_ERROR_CANB_GENERAL                 0x8120
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
#define EMCY_ERROR_OPERATIONAL                  0xF105
#define EMCY_ERROR_POWER_SHARING                0xFF16
#define EMCY_ERROR_EXT_PWR_LOSS                 0xFF17
#define EMCY_ERROR_EXT_PWR_LOSS_BOTH            0xFF18
#define EMCY_ERROR_INPUT_POWER_TO_HIGH          0xFF19
#define EMCY_ERROR_CONSUMED_POWER_TO_HIGH       0xFF1A
#define EMCY_ERROR_SWITCHING                    0xFF20 /* not used */
#define EMCY_ERROR_SOC_BELOW_LIMIT              0xFF30
#define EMCY_ERROR_SOC_BELOW_SAFETY_THRESHOLD   0xFF31


extern uint32_t global_error_code;

bool connect_other_dpmu_to_shared_bus(void);
int8_t connect_other_dpmu_to_shared_bus_answer(float *remote_bus_voltage);
void error_check_for_errors(void);


#endif /* COAPPL_ERROR_CODES_H_ */