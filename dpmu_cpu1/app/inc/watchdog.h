/*
 * watchdog.h
 *
 *  Created on: 29 jan. 2024
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#ifndef APP_INC_WATCHDOG_H_
#define APP_INC_WATCHDOG_H_


#ifdef CPU1
#define USE_WATCHDOG
#endif

void watchdog_init(void);
void watchdog_feed(void);


#endif /* APP_INC_WATCHDOG_H_ */
