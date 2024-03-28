/*
 * payload_gen.h
 *
 *  Created on: 2 jan. 2024
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#ifndef APP_INC_PAYLOAD_GEN_H_
#define APP_INC_PAYLOAD_GEN_H_


#include <stdint.h>

void payload_gen_load_current(uint8_t status, uint8_t ampere[4]);
void payload_gen_bus_voltage(uint8_t status, uint8_t voltage[4]);


#endif /* APP_INC_PAYLOAD_GEN_H_ */
