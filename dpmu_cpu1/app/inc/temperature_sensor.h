/*
 * temperature_sensor.h
 *
 *  Created on: 3 feb. 2023
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#ifndef APP_INC_TEMPERATURE_SENSOR_H_
#define APP_INC_TEMPERATURE_SENSOR_H_

#include <stdint.h>

#include "i2c_com.h"

#define ENDURANCE 1

#define TMP100_TEMP_REG       0b00
#define TMP100_CONFIG_REG     0b01
#define TMP100_TEMP_LOW_REG   0b10
#define TMP100_TEMP_HIGH_REG  0b11

/* configure device for continuous and comparator mode, 1/4 degree C resolution
 *
 * OS/ALERT         [7]     = 0     (for One-Shot measurements, we are in comparator mode)
 * Resolution       [6..5]  = 0b01  (10 bit, 0.25 degree C)
 * Fault Query      [4..3]  = 0b11  (6 consecutive faults)
 * Polarity         [2]     = 0     (default, alert pin is active low)
 * Thermostat mode  [1]     = 0     (comparator mode)
 * Shut Down        [0]     = 0     (continuous mode) */
#define TEMPERATURE_SENSOR_CONFIG_WORD 0b00111000 /* 0x38, 0d56 */


#define TEMPERATURE_SENSOR_BASE      0
#define TEMPERATURE_SENSOR_MAIN      1
#define TEMPERATURE_SENSOR_MEZZANINE 2
#define TEMPERATURE_SENSOR_PWR_BANK  3

#define TEMPERATURE_SENSOR_BASE_ADDRESS      0x4A
#define TEMPERATURE_SENSOR_MAIN_ADDRESS      0x4C

#define TEMPERATURE_SENSOR_MEZZANINE_ADDRESS 0x48
#define TEMPERATURE_SENSOR_PWR_BANK_ADDRESS  0x4E

int16_t temperature_sensor_read_temperature(uint8_t temp_sensor_number, int16_t *readValue);
void temperature_sensor_read_all_temperatures(void);

int16_t temperature_sensor_read_config_reg(uint8_t temp_sensor_number, uint16_t *readBuffer);
int16_t temperature_sensor_write_config_reg(uint8_t temp_sensor_number, uint16_t *writeBuffer);

int16_t temperature_sensor_read_tLow_reg(uint8_t temp_sensor_number, uint16_t *readBuffer);
int16_t temperature_sensor_write_tLow_reg(uint8_t temp_sensor_number, uint16_t *writeBuffer);

int16_t temperature_sensor_read_tHigh_reg(uint8_t temp_sensor_number, uint16_t *readBuffer);
int16_t temperature_sensor_write_tHigh_reg(uint8_t temp_sensor_number, uint16_t *writeBuffer);

int16_t temperature_sensor_reset(uint8_t temp_sensor_number);

int16_t temperature_sensor_configure(uint8_t temp_sensor_number);
void temperature_sensor_init_individual_sensor(
        struct I2CHandle *temperatureSensor,
        uint16_t slave_address);
int16_t temperature_sensors_init(void);
void readAlltemperatures();


extern int16_t temperatureSensorVector[4];
extern int16_t temperatureHotPoint;
extern int8_t temperature_absolute_max_limit;
extern int8_t temperature_high_limit;

#endif /* APP_INC_TEMPERATURE_SENSOR_H_ */
