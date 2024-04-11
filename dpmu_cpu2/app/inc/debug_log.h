/*
 * debug_log.h
 *
 *  Created on: 18 de mar de 2024
 *      Author: gferreira
 */

#ifndef APP_INC_DEBUG_LOG_H_
#define APP_INC_DEBUG_LOG_H_

#include <stdint.h>

#define LOG_PERIOD_INITIALIZE 100
#define LOG_PERIOD_CHARGE 500
#define LOG_PERIOD_TRICKLE_CHARGE 1000
#define LOG_PERIOD_REGULATE 1000
#define LOG_PERIOD_IDLE 5//000


uint32_t SetDebugLogPeriod(void);
void DisableDebugLog(void);
void EnableDebugLog(void);
void UpdateDebugLog(void);
void ForceUpdateDebugLog(void);
void UpdateDebugLogFake(void);


#endif /* APP_INC_DEBUG_LOG_H_ */
