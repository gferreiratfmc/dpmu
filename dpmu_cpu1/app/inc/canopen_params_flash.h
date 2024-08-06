/*
 * canopen_params_flash.h
 *
 *  Created on: 10 nov. 2023
 *      Author: us
 */

#pragma once

//#ifndef APP_INC_CANOPEN_PARAMS_FLASH_H_
//#define APP_INC_CANOPEN_PARAMS_FLASH_H_

#include <stdint.h>
#include <stddef.h>

#include "savedobjs.h"

// Magic number for objects stored in external flash.
#define OD_MAGIC_NUM    0x534A424Fu      // 'OBJS' as 32-bit little endian unsigned

// Range of OD indexes.
//typedef struct {
//    uint16_t begin;
//    uint16_t end;
//} od_range_t;

// Stored objects in external flash starts with a save_od_info_t struct.
//typedef struct {
//    uint32_t magic;     // magic number
//    uint16_t count;     // number of following objects
//    uint16_t reserved;
//} save_od_info_t;

/**
 * Write parameters to external flash.
 */
//bool write_params_to_ext_flash(const save_od_t *params, uint16_t count);

/**
 * Reads parameters from external flash.
 *
 * Checks for a valid magic number and returns 0 if not found.
 *
 * @param   params  pointer do destination buffer
 *
 * @retval  number of parameters read, or 0 if none
 */
//int read_params_from_ext_flash(save_od_t *params, const od_range_t *excluded);

//#endif /* APP_INC_CANOPEN_PARAMS_FLASH_H_ */
