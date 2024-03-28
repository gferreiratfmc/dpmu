/*
 * test_debug_log.h
 *
 *  Created on: 24 maj 2023
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#ifndef APP_INC_DEBUG_LOG_H_
#define APP_INC_DEBUG_LOG_H_


#include "GlobalV.h"


#define SIZE_OF_DEBUG_LOG 0xFFFF

void debug_log_init(void);
void debug_log_save_to_debug_log(void);

void debug_log_test(void);


#endif /* APP_INC_DEBUG_LOG_H_ */
