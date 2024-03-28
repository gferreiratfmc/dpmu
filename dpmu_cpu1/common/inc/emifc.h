/*
 * emif.h
 *
 *  Created on: 2 jan. 2023
 *      Author: vb
 */

#ifndef APP_INC_EMIFC_H_
#define APP_INC_EMIFC_H_


#include "stdbool.h"
#include "stdint.h"


#define CPU_TYPE_ONE   1
#define CPU_TYPE_TWO   2

typedef struct {
    uint32_t address;
    uint16_t cpuType;
    uint32_t size;
    volatile uint16_t* data;
}EMIF1_Config;

#define MEM_TYPE_RAM   0
#define MEM_TYPE_FLASH 1

#define EXT_RAM_START_ADDRESS_CS2 0x00100000U
#define EXT_RAM_SIZE_CS2 0x20000U

#if defined(USE_WRONG_EXT_FLASH_SIZE)
#define EXT_FLASH_START_ADDRESS_CS3 0x00300000U
#define EXT_FLASH_SIZE_CS3 0x20000U
#endif

//bool emifc_execute();
void emifc_set_cpun_as_master(EMIF1_Config* emif1);
void emifc_realease_cpun_as_master(uint16_t cpuType);
void emifc_cpu_write_memory(EMIF1_Config* emif1);
void emifc_cpu_read_memory(EMIF1_Config* emif1);

//void emifc_pinmux_setup_flash(void);
//uint16_t emifc_write_flash_data(uint32_t startAddr, uint32_t memSize);
//uint16_t emifc_read_flash_data(uint32_t startAddr, uint32_t memSize);
//bool emifc_execute();
void emifc_pinConfiguration();
void emifc_pinmux_setup_memory(uint16_t memType);

uint16_t emifc_read_flash_data(uint32_t startAddr, uint32_t memSize);
uint16_t emifc_write_flash_data(uint32_t startAddr, uint32_t memSize);


#endif /* APP_INC_EMIFC_H_ */
