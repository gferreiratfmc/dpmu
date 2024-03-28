/*
 * startup_sequence.h
 *
 *  Created on: 25 nov. 2022
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#ifndef APP_INC_STARTUP_SEQUENCE_H_
#define APP_INC_STARTUP_SEQUENCE_H_


#include <stdbool.h>
#include <stdint.h>

bool dpmu_type_allowed_to_use_shared_bus(void);
bool connect_shared_bus(bool got_remote_voltage, uint32_t remote_voltage);
void startup_sequence(void);


#endif /* APP_INC_STARTUP_SEQUENCE_H_ */
