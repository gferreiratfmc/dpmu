/*
 * rtc.h
 *
 *  Created on: 4 okt. 2022
 *      Author: us
 */

#ifndef APP_INC_RTC_H_
#define APP_INC_RTC_H_

#include <time.h>
#include <stdint.h>
#include <driverlib.h>

void RTC_Init(void);
void rtc_start_reset(void);
uint8_t rtc_check_start(void);
void rtc_vbat_enable(void);
uint8_t rtc_check_vbat(void);
void rtc_mk_config(void);
uint8_t rtc_check_config(void);
void print_time(void);

int set_time(time_t timestamp);
time_t get_time();

int i2c_read_bytes(uint16_t eeaddr, int len, uint8_t *buf);
int i2c_write_bytes(uint16_t eeaddr, int len, uint8_t *buf);

#endif /* APP_INC_RTC_H_ */
