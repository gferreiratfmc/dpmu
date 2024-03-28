/*
 * crc16.h
 *
 *  Created on: 30 nov. 2020
 *      Author: us
 */

#ifndef INC_CRC16_H_
#define INC_CRC16_H_

#include <stdint.h>

void crc16_update_byte(uint16_t *crc, unsigned char data);
void crc16_update(uint16_t *crc, unsigned char *data, uint16_t len);

#endif /* INC_CRC16_H_ */
