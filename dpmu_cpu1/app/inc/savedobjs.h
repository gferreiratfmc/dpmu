/**
 * @file savedobjs.h
 *
 * Generated by mksaveobjsarray.py from CSV file ..\app\device_profile\DPMU_001.csv
 *
 * Date: 2024-02-06 02:02
 */

#pragma once

#include <stdint.h>
#include "co_datatype.h"

#define ONE_BYTE    1
#define TWO_BYTES   2
#define THREE_BYTES 3
#define FOUR_BYTES  4

// Stuff copied from Emotas example.
typedef struct {
    uint16_t index;
    uint8_t subIndex;
    uint32_t size;
    uint32_t value;
} save_od_t;

extern const save_od_t saveObj[];
extern save_od_t savedData[];

uint32_t get_num_saved_objs(void);
