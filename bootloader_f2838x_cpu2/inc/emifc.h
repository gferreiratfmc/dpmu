#ifndef APP_INC_EMIFC_H_
#define APP_INC_EMIFC_H_

#include "stdint.h"

#define MEM_TYPE_RAM   0
#define MEM_TYPE_FLASH 1

#define CPU_TYPE_ONE   1
#define CPU_TYPE_TWO   2

typedef struct {
    uint32_t address;
    uint16_t cpuType;
    uint32_t size;
    uint16_t* data;
}EMIF1_Config;


#define EXT_RAM_START_ADDRESS_CS2 0x00100000U
#define EXT_RAM_SIZE_CS2 0x20000U



#define EXT_FLASH_START_ADDRESS_CS3 0x00300000U
#define EXT_FLASH_SIZE_CS3 0x20000U


void emifc_configuration();
void emifc_set_cpun_as_master(EMIF1_Config* emif1);
void emifc_realease_cpun_as_master(uint16_t cpuType);
void emifc_cpu_read_memory(EMIF1_Config* emif1);
void emifc_cpu_write_memory(EMIF1_Config* emif1);
void emifc_configure_cpu2();
#endif /* APP_INC_EMIFC_H_ */
