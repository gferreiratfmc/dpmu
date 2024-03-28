/*
 * log.h
 *
 *  Created on: 30 nov. 2022
 *      Author: vb
 */

#ifndef COAPPL_LOG_H_
#define COAPPL_LOG_H_

#include "driverlib.h"
#include "co_odaccess.h"
#include "co_datatype.h"

#define CAN_LOG_MAGIC       0x55AAu

#define FIRST_LOG_SECTOR    EXT_FLASH_SA4
#define LAST_LOG_SECTOR     EXT_FLASH_SA34


typedef enum{

    type_measurement = 0,
    type_canA,
    type_canB,
    type_error_severity_1,
    type_error_severity_2,
    /* More to be added*/

}LogType_e;


typedef struct {
    uint32_t address;
    unsigned char  data[8];
}LogTypeCAN_t;

typedef struct{
   uint32_t data;
}LogTypeGeneric_t;


typedef struct{
    uint32_t      timestamp;
    LogType_e   log_type;
    union{
        LogTypeGeneric_t log_generic;
        LogTypeCAN_t     log_can;
    } log_data;
    uint16_t      nl;
}LogFrame_t;

extern unsigned char  message[];

RET_T log_debug_log_set_state(uint8_t value);
void log_read_domain(UNSIGNED16 index, UNSIGNED8 subindex, UNSIGNED32 domainBufSize, UNSIGNED32 domainTransferedSize);
uint8_t log_debug_log_read(
        BOOL_T      execute,
        UNSIGNED8   sdoNr,
        UNSIGNED16  index,
        UNSIGNED8   subIndex
    );
void log_debug_log_reset(void);

uint8_t log_can_log_read(
        BOOL_T      execute,
        UNSIGNED8   sdoNr,
        UNSIGNED16  index,
        UNSIGNED8   subIndex
    );
void log_can_log_reset(void);

/* functions to store the logs to external memories */
bool log_store_debug_log(unsigned char *pnt);
void log_can_init(void);
void log_store_can_log(uint8_t size, unsigned char *pnt);

void getObjData(
        CO_CONST CO_OBJECT_DESC_T *pDesc,
        void     *pObj,  /**< pointer for description index */
        UNSIGNED16  index,
        UNSIGNED8   subIndex
    );

void log_store_pdo_ds401(uint8_t port);
void log_store_new_state(uint16_t state);
void log_store_debug_log_to_ram(void);
void log_debug_read_from_ram(void);

#endif /* COAPPL_LOG_H_ */
