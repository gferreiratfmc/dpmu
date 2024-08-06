/*
 * log.c
 *
 *  Created on: 30 nov. 2022
 *      Author: vb
 */

#include <string.h>

#include "board.h"
#include "co_canopen.h"
#include "co_common.h"
#include "co_p401.h"
#include "emifc.h"
#include "error_handling.h"
#include "ext_flash.h"
#include "gen_define.h"
#include "gen_indices.h"
#include "GlobalV.h"
#include "log.h"
#include "main.h"
#include "serial.h"
#include "shared_variables.h"
#include "temperature_sensor.h"
#include "timer.h"


#define SIZE_OF_TIMESTAMP 4 /* in Bytes */

#define MESSAGE_LENGTH (TRANSFER_SIZE * 5 - 2 - 1)  /* size of one transfer (several messages) * number of transfers */
volatile static debug_log_t debug_log_copy;

/* common struct to read logs from external memories */
EMIF1_Config emif1_log_read;

/* for debug log in external RAM */
//static uint32_t debug_log_start_address              = EXT_RAM_START_ADDRESS_CS2;
static uint32_t debug_log_next_free_address          = EXT_RAM_START_ADDRESS_CS2;
static uint32_t debug_log_last_read_address          = EXT_RAM_START_ADDRESS_CS2;
static uint32_t debug_log_last_writen_address        = 0;
static bool     debug_log_address_has_wrapped_around = false;
static uint32_t debug_log_size                       = sizeof(debug_log_t);
static bool     debug_log_active                     = true;

/* for can log in external FLASH */
static uint32_t can_log_start_address               = CAN_LOG_ADDRESS_START;
static uint32_t can_log_last_read_address           = CAN_LOG_ADDRESS_START;
static uint32_t can_log_next_free_address           = CAN_LOG_ADDRESS_START;
static bool     can_log_address_has_wrapped_around  = false;
static uint32_t can_log_size                        = sizeof(debug_log_t);
static bool can_log_possible = false;

static States_t canLogState = { 0 };

RET_T log_debug_log_set_state(uint8_t value)
{

    /* change active state of debug log */
    if(value == 0) {
        debug_log_active = false;
    } else {
        debug_log_active = true;
    }
    return RET_OK;
}

/** \brief function pointer to SDO server read domain event
* \param index - object index
* \param subindex - object subindex
* \param domainBufSize - actual size at domain buffer
* \param transferSize - actual transfered size
*
* \return RET_T
*/
void log_read_domain(UNSIGNED16 index, UNSIGNED8 subindex, UNSIGNED32 domainBufSize, UNSIGNED32 domainTransferedSize)
{
    uint32_t start_address = 0;

    static uint32_t sumOfTransferedBytes = 0;

    //Disable new logs while in transfer
    sharedVars_cpu1toCpu2.debug_log_disable_flag = true;

    // Doublecheck that domainbufsize are not bigger than allocated buf size.
    if (domainBufSize > TRANSFER_SIZE ) {
        domainBufSize = TRANSFER_SIZE;
    }

    /* CAN/CANopen standard uses Bytes, we store 16 bit Words */
    domainBufSize = (domainBufSize + 1) / 2;
    domainTransferedSize = (domainTransferedSize + 1) / 2;

    if(I_DEBUG_LOG == index)
    {
        if( debug_log_last_read_address >= debug_log_next_free_address   ) {
            debug_log_last_read_address = debug_log_last_read_address;
            return;
        }
        start_address = debug_log_last_read_address;
        if(start_address >= (EXT_RAM_START_ADDRESS_CS2 + EXT_RAM_SIZE_CS2))
            start_address = EXT_RAM_START_ADDRESS_CS2;
    }
    if(I_CAN_LOG == index)
    {
//        if( can_log_last_read_address >= can_log_next_free_address   ) {
//            can_log_last_read_address = can_log_last_read_address;
//            return;
//        }
        start_address = can_log_last_read_address;
        if(start_address >= CAN_LOG_ADDRESS_END) {
            start_address = CAN_LOG_ADDRESS_START;
        }
    }


    /* set CPU1 as master for memory */
    MemCfg_setGSRAMMasterSel(MEMCFG_SECT_GS0, MEMCFG_GSRAMMASTER_CPU1);
    //MemCfg_setGSRAMMasterSel(MEMCFG_SECT_GS3, MEMCFG_GSRAMMASTER_CPU1);

    if (I_DEBUG_LOG == index) {

        emif1_log_read.address = start_address;
        emif1_log_read.cpuType = CPU_TYPE_ONE;
        emif1_log_read.data = (uint16_t *)message;
        emif1_log_read.size = domainBufSize;
        //DMA_configBurst(CPU1_EXT_MEM_BASE, TRANSFER_SIZE, 1, 1);
        emifc_cpu_read_memory(&emif1_log_read);
        debug_log_last_read_address = start_address + domainBufSize;
        sumOfTransferedBytes = sumOfTransferedBytes + domainBufSize;
        if( sumOfTransferedBytes >= sizeof(debug_log_t) ) {
            //debug_log_last_read_address = debug_log_last_read_address + sizeof(debug_log_t);
            sumOfTransferedBytes = 0;
        }
        if( (debug_log_next_free_address - debug_log_last_read_address) == 0 ){
            sharedVars_cpu1toCpu2.debug_log_disable_flag = false;
        }

    }

    if (I_CAN_LOG == index) {
        emif1_log_read.address = start_address;
        emif1_log_read.cpuType = CPU_TYPE_ONE;
        emif1_log_read.data = (uint16_t *)message;
        emif1_log_read.size = domainBufSize;
        ext_flash_read_buf(emif1_log_read.address, (uint16_t*)emif1_log_read.data, emif1_log_read.size);
        can_log_last_read_address = start_address + domainBufSize;

        sumOfTransferedBytes = sumOfTransferedBytes + domainBufSize;
        if( sumOfTransferedBytes >= sizeof(debug_log_t) ) {
            //debug_log_last_read_address = debug_log_last_read_address + sizeof(debug_log_t);
            sumOfTransferedBytes = 0;
        }

        //Enable new logs after transfer
        sharedVars_cpu1toCpu2.debug_log_disable_flag = false;
//        if( (can_log_next_free_address - can_log_last_read_address) == 0 ){
//            sharedVars_cpu1toCpu2.debug_log_disable_flag = false;
//        }
    }

//    Serial_debug(DEBUG_INFO, &cli_serial, "%08x  ", start_address);
//    Serial_debug(DEBUG_INFO, &cli_serial, "%04x  ", domainBufSize);
//    for(int i = 0; i < domainBufSize; i++)
//    {
//        Serial_debug(DEBUG_INFO, &cli_serial, "%04x ", message[i]);
//    }
//    Serial_debug(DEBUG_INFO, &cli_serial, "\r\n");
}

uint8_t log_debug_log_read(
        BOOL_T      execute,
        UNSIGNED8   sdoNr,
        UNSIGNED16  index,
        UNSIGNED8   subIndex
    )
{
    uint32_t sizeToTransfer;

    sizeToTransfer = 2 * (debug_log_next_free_address - debug_log_last_read_address);

    if(  (debug_log_last_read_address < debug_log_next_free_address) && ( sizeToTransfer > 0 ) ) {

        //TODO probably EXT_RAM_START_ADDRESS_CS2 -> debug_log_start_address
        /* set the correct starting address of the external memory */
        emif1_log_read = (EMIF1_Config){
            debug_log_last_read_address, //EXT_RAM_START_ADDRESS_CS2,
            CPU_TYPE_ONE,
            TRANSFER_SIZE,
            (uint16_t*)message
         };

        /* set the CANopen OD object to point to the right domain, the variable message
         * set the CANopen OD object size to MESSAGE_LENGTH
         */
        coOdDomainAddrSet(
                            index,              /**< index of object */
                            subIndex,           /**< subindex of object */
                            message,            /**< pointer to domain */
                            sizeToTransfer  //2 * debug_log_size  /**< domain length */ /* '2x' we use 16 bit Words */
                         );
        sharedVars_cpu1toCpu2.debug_log_disable_flag = true;
        return RET_OK;
    } else {
        coOdDomainAddrSet(
                            index,              /**< index of object */
                            subIndex,           /**< subindex of object */
                            message,            /**< pointer to domain */
                            0 //2 * debug_log_size  /**< domain length */ /* '2x' we use 16 bit Words */
                         );
        return RET_FLASH_EMPTY;
    }


}

void log_debug_log_reset(uint8_t resetType)
{
    // reset number of entries
    // reset position in memory to beginning of RAM portion ???

    switch( (LogResetType_e)resetType ) {
        case LOG_RESET_REWIND:
            debug_log_last_read_address          = EXT_RAM_START_ADDRESS_CS2;
            break;
        case LOG_RESET_FULL:
        default:
            debug_log_next_free_address          = EXT_RAM_START_ADDRESS_CS2;
            debug_log_last_read_address          = EXT_RAM_START_ADDRESS_CS2;
            debug_log_address_has_wrapped_around = false;
            debug_log_size                       = 0;
            break;
    }

    /* activate debug log */
    debug_log_active                     = true;

    /* in CANopen OD mark log state as ON */
    coOdPutObj_u8(I_DEBUG_LOG, S_DEBUG_LOG_STATE, 1);
}

/* brief: returns the size of the log entry
 *
 * details:
 *
 * requirements:
 *
 * argument: log_type - the log type, all logs start with a log type
 *
 * return: the size (Bytes) of type of log entry
 *
 * note: all CAN logs starts with a four Bytes timestamp
 *
 */


uint8_t log_can_log_read(
        BOOL_T      execute,
        UNSIGNED8   sdoNr,
        UNSIGNED16  index,
        UNSIGNED8   subIndex
    )
{


    uint32_t sizeToTransfer;


    if( can_log_last_read_address > can_log_next_free_address) {
        sizeToTransfer = 2 * (  CAN_LOG_ADDRESS_END - can_log_last_read_address);
        sizeToTransfer = sizeToTransfer + 2 * ( can_log_next_free_address - CAN_LOG_ADDRESS_START);
    } else {
        sizeToTransfer = 2 * (can_log_next_free_address - can_log_last_read_address);
    }

    if(  sizeToTransfer > 0 ) {

        //TODO probably EXT_RAM_START_ADDRESS_CS2 -> debug_log_start_address
        /* set the correct starting address of the external memory */
        emif1_log_read = (EMIF1_Config){
            can_log_last_read_address, //EXT_RAM_START_ADDRESS_CS2,
            CPU_TYPE_ONE,
            TRANSFER_SIZE,
            (uint16_t*)message
         };

        /* set the CANopen OD object to point to the right domain, the variable message
         * set the CANopen OD object size to MESSAGE_LENGTH
         */
        coOdDomainAddrSet(
                            index,              /**< index of object */
                            subIndex,           /**< subindex of object */
                            message,            /**< pointer to domain */
                            sizeToTransfer  //2 * debug_log_size  /**< domain length */ /* '2x' we use 16 bit Words */
                         );
        //sharedVars_cpu1toCpu2.debug_log_disable_flag = true;
        return RET_OK;
    } else {
        coOdDomainAddrSet(
                            index,              /**< index of object */
                            subIndex,           /**< subindex of object */
                            message,            /**< pointer to domain */
                            0 //2 * debug_log_size  /**< domain length */ /* '2x' we use 16 bit Words */
                         );
        return RET_FLASH_EMPTY;
    }



//    //TODO probably EXT_FLASH_START_ADDRESS_CS3 -> can_log_start_address
//    /* set the correct starting address of the external memory */
//    emif1_log_read = (EMIF1_Config)
//                     {
//                      EXT_RAM_START_ADDRESS_CS2,
//                      CPU_TYPE_ONE,
//                      TRANSFER_SIZE,
//                      (uint16_t*)message
//                     };
////    emif1_log_read = (EMIF1_Config)
////                     {EXT_FLASH_START_ADDRESS_CS3,
////                      CPU_TYPE_ONE,
////                      TRANSFER_SIZE,
////                      (uint16_t*)message
////                     };
//
//    /* set the CANopen OD object to point to the right domain, the variable message
//     * set the CANopen OD object size to MESSAGE_LENGTH
//     */
//    coOdDomainAddrSet(
//                        index,              /**< index of object */
//                        subIndex,           /**< subindex of object */
//                        message,            /**< pointer to domain */
//                        2 * can_log_size    /**< domain length */ /* '2x' we use 16 bit Words */
//                     );
//
//    return RET_OK;
}

void log_can_log_reset(void)
{
    canLogState.State_Next = StartEraseFlash;
}

void log_erase_entire_flash(void)
{
    canLogState.State_Next = StartEraseEntireFlash;
}

/* store debug log in external RAM
 *
 * assumptions: all log entries are equal in size
 */
bool log_store_debug_log(unsigned char *pnt)
{
    bool retVal;
    uint16_t size_in_words = sizeof(debug_log_t);
    uint32_t debug_log_write_address = debug_log_next_free_address;


    /* copy the data, do it early
     *
     * the data _must_ be stored in RAMGSx
     * message[] is located in RAMGS0
     */
    for(int i = 0; i < size_in_words; i++)
    {
        message[i] = pnt[i];
    }
    message[size_in_words] = '\0';

    /* check if log is active */
    if(debug_log_active)
    {

        /* calculate next free address */
        debug_log_next_free_address += size_in_words;

        /* check if next free address wraps around in memory */
        if(debug_log_next_free_address > (EXT_RAM_START_ADDRESS_CS2 + EXT_RAM_SIZE_CS2))
        {
            /* address wraps around */
            debug_log_next_free_address = EXT_RAM_START_ADDRESS_CS2;

            /* mark that at least one wrap around has occurred in memory */
            debug_log_address_has_wrapped_around = true;
        }

        if(debug_log_address_has_wrapped_around)
        {
            //debug_log_start_address = debug_log_next_free_address + size_in_words;  /* stores at most all possible log entries - 1 */
            /* check if log start address wraps around in memory */
//            if(debug_log_start_address > (EXT_RAM_START_ADDRESS_CS2 + EXT_RAM_SIZE_CS2))
//            {
//                /* address wraps around */
//                debug_log_start_address = EXT_RAM_START_ADDRESS_CS2;
//            }
        }
        else
        {
            debug_log_size += size_in_words;  /* size will not increase after log has wrapped around */
        }

        EMIF1_Config emif1_ram_log = {debug_log_write_address,
                                      CPU_TYPE_ONE,
                                      size_in_words,
                                      (uint16_t *)message};

        /* write to external RAM */
        emifc_cpu_write_memory(&emif1_ram_log);
        debug_log_last_writen_address = debug_log_write_address;
//        Serial_debug(DEBUG_INFO, &cli_serial, "debug_log_next_free_address %lx  size %08lx\r\n", debug_log_next_free_address, size_in_words);

        retVal = true;
    } else
    {
//        Serial_debug(DEBUG_INFO, &cli_serial, "debug_log not active\r\n");
        retVal = false;
    }

    return retVal;
}



bool can_log_search_free_debug_address( uint32_t *nextFreeAddress ){
    debug_log_t current_saved_log;
    uint32_t addr = CAN_LOG_ADDRESS_START;
    bool freeAdrressFound = false;

    while( addr < CAN_LOG_ADDRESS_END - sizeof(debug_log_t) ) {
        ext_flash_read_buf(addr, (uint16_t *)&current_saved_log, sizeof(debug_log_t) );
        if( current_saved_log.MagicNumber != MAGIC_NUMBER) {
            if( current_saved_log.MagicNumber == 0xFFFFFFFF) {
                freeAdrressFound = true;
                *nextFreeAddress =  addr;
                break;
            }
        }
        addr = addr + sizeof(debug_log_t);
    }
    return freeAdrressFound;
}

void log_can_init(void)
{

    can_log_possible = true;


    if( can_log_search_free_debug_address( &can_log_next_free_address ) != true ) {
        can_log_possible = false;
    }
    can_log_start_address = can_log_next_free_address;
    can_log_last_read_address = can_log_next_free_address;
}




/**
 * @brief   Stores log message in external flash.
 *
 * @param   size    size of log message in bytes
 * @param   pnt     pointer to log message to be stored in flash
 */
void log_store_can_log(uint16_t size, unsigned char *pnt)
{
    // Beware of NULL pointer argument.
    if (NULL == pnt) {
        return;
    }

    const ext_flash_desc_t *current_sector_desc;
    const ext_flash_desc_t *next_sector_desc;

    uint16_t size_in_words = size;

    uint32_t can_log_write_address = can_log_next_free_address;

    // Get pointer to info on current sector.
    current_sector_desc = ext_flash_sector_from_address(can_log_write_address);

    // Calculate next free address by adding size of log message.
    can_log_next_free_address += size_in_words;

    // Get pointer to info on next sector.
    next_sector_desc = ext_flash_sector_from_address(can_log_next_free_address);

    if (current_sector_desc->sector != next_sector_desc->sector) {
        // Check if next free address wraps around in memory.
        if (can_log_next_free_address > CAN_LOG_ADDRESS_END) {
            // Address wraps around.
            can_log_next_free_address = CAN_LOG_ADDRESS_START;

            // Mark that at least one wrap around has occurred in external flash.
            can_log_address_has_wrapped_around = true;

        }

        // Erase next sector.
        ext_flash_erase_sector(can_log_next_free_address);

        // Update 'can_log_size'according to erased sector.
        can_log_size -= next_sector_desc->size;

        // Update start address of log.
        if (can_log_address_has_wrapped_around) {

            can_log_start_address = CAN_LOG_ADDRESS_START;  /* sector 4 erased, use next sector sector 5 */
            can_log_write_address = can_log_start_address;
        }


        /* update size of log */
        if(!can_log_address_has_wrapped_around)
        {
            can_log_size += size_in_words;  /* size will not increase after log has wrapped around */
        }
    }

    /* the data _must_ be stored in RAMGSx
     * message[] is located in RAMGS0
     */
    for (int i = 0; i < (size_in_words); i++) /* 2 words for time stamp and magic number */
    {
        message[i] = pnt[i];
    }
    message[size_in_words] = '\0';

    ext_flash_write_buf(can_log_write_address, (uint16_t *)message, size_in_words);
    debug_log_last_writen_address = can_log_write_address;

}



void log_can_state_machine(void) {
    static uint32_t timeStart;
    static const ext_flash_desc_t *flashDescStart, *flashDescEnd;
    static ext_flash_desc_t flashDescToErase;

    switch( canLogState.State_Current ) {

        case Logging:
            log_store_debug_log_to_flash();
            log_debug_read_from_flash();
            break;
        case StartEraseFlash:
            // Call command do erase can log ext flash
            Serial_debug(DEBUG_INFO, &cli_serial, "CAN LOG Flash erase start\r\n");
            timeStart = timer_get_ticks();
            sharedVars_cpu1toCpu2.debug_log_disable_flag = true;

            flashDescStart = ext_flash_sector_from_address( CAN_LOG_ADDRESS_START );
            Serial_debug(DEBUG_INFO, &cli_serial, "CAN_LOG_ADDRESS_START sector:[0x%02X] address:[0x%08p] flash_offset:[0x%08p]\r\n",
                         flashDescStart->sector, flashDescStart->addr + EXT_FLASH_START_ADDRESS_CS3, flashDescStart->addr );

            memcpy( &flashDescToErase, flashDescStart, sizeof(ext_flash_desc_t) );
            // Calculate the address which is required to erase the flash.
            flashDescToErase.addr = flashDescToErase.addr + EXT_FLASH_START_ADDRESS_CS3;


            flashDescEnd = ext_flash_sector_from_address( CAN_LOG_ADDRESS_END );
            Serial_debug(DEBUG_INFO, &cli_serial, "CAN_LOG_ADDRESS_END sector:[0x%02X] address:[0x%08p] flash_offset:[0x%08p]\r\n",
                         flashDescEnd->sector, flashDescEnd->addr + EXT_FLASH_START_ADDRESS_CS3, flashDescEnd->addr );

            canLogState.State_Next = EraseCANLogFlashSector;
            break;

        case EraseCANLogFlashSector:

            Serial_debug(DEBUG_INFO, &cli_serial, "ERASING CAN_LOG_ADDRESS sector:[0x%02X] address:[0x%08p] flash_offset:[0x%08p]\r\n",
                         flashDescToErase.sector, flashDescToErase.addr, (flashDescToErase.addr - EXT_FLASH_START_ADDRESS_CS3) );

            ext_flash_erase_sector_by_descriptor( &flashDescToErase );

            flashDescToErase.sector++;

            look_up_start_address_of_sector( &flashDescToErase );


            canLogState.State_Next = WaitingEraseDone;

        case WaitingEraseDone:
            if( ext_flash_ready() ) {
                if( flashDescToErase.sector > flashDescEnd->sector ) {
                    log_can_init();
                    Serial_debug(DEBUG_INFO, &cli_serial, "External Flash erase stop. Time:[%lu]\r\n", timer_get_ticks() - timeStart);
                    sharedVars_cpu1toCpu2.debug_log_disable_flag = false;
                    canLogState.State_Next = Logging;
                } else {
                    canLogState.State_Next = EraseCANLogFlashSector;
                }
            }
            break;


        case StartEraseEntireFlash:
            // Call command do erase entire flash_chip_erase
            Serial_debug(DEBUG_INFO, &cli_serial, "Entire Flash erase start\r\n");
            timeStart = timer_get_ticks();
            sharedVars_cpu1toCpu2.debug_log_disable_flag = true;

            ext_command_flash_chip_erase();
            canLogState.State_Next = WaitingEntireEraseDone;

            break;

        case WaitingEntireEraseDone:
            if( ext_flash_ready() ) {
                log_can_init();
                Serial_debug(DEBUG_INFO, &cli_serial, "External Flash erase stop. Time:[%lu]\r\n", timer_get_ticks() - timeStart);
                sharedVars_cpu1toCpu2.debug_log_disable_flag = false;
                canLogState.State_Next = Logging;
            }
            break;

        default:
            canLogState.State_Next = Logging;
    }


    canLogState.State_Current = canLogState.State_Next;
}


/* check if there are new debug data to store */
void log_store_debug_log_to_flash(void)
{

    volatile static uint32_t last_debug_log_number = 0;

    timer_time_t ptime;

    if( ( can_log_possible == false ) || ( sharedVars_cpu1toCpu2.debug_log_disable_flag == true ) ) {
        return;
    }

    /* check if there are new debug data to store */
    if (last_debug_log_number != sharedVars_cpu2toCpu1.debug_log.counter)
    {

        /* make local copy of data */
        memcpy((void *)&debug_log_copy, (void *)&sharedVars_cpu2toCpu1.debug_log, sizeof(debug_log_t));
        timer_get_time(&ptime);
        debug_log_copy.MagicNumber = MAGIC_NUMBER;
        debug_log_copy.CurrentTime = ptime.can_time;
        debug_log_copy.BaseBoardTemperature = temperatureSensorVector[TEMPERATURE_SENSOR_BASE];
        debug_log_copy.MainBoardTemperature = temperatureSensorVector[TEMPERATURE_SENSOR_MAIN];
        debug_log_copy.MezzanineBoardTemperature = temperatureSensorVector[TEMPERATURE_SENSOR_MEZZANINE];
        debug_log_copy.PowerBankBoardTemperature = temperatureSensorVector[TEMPERATURE_SENSOR_PWR_BANK];


        /* update last read counter value */
        last_debug_log_number = debug_log_copy.counter;

        /* store data in flash */
        log_store_can_log( sizeof(debug_log_t),  (unsigned char*) &debug_log_copy);
    }
}

/* check if there are new debug data to store */
void log_store_debug_log_to_ram(void)
{

    volatile static uint32_t last_debug_log_number = 0;

    timer_time_t ptime;

    if( sharedVars_cpu1toCpu2.debug_log_disable_flag == true ) {
        return;
    }

    /* check if there are new debug data to store */
    if (last_debug_log_number != sharedVars_cpu2toCpu1.debug_log.counter)
    {

        /* make local copy of data */
        memcpy((void *)&debug_log_copy, (void *)&sharedVars_cpu2toCpu1.debug_log, sizeof(debug_log_t));
        timer_get_time(&ptime);
        debug_log_copy.MagicNumber = 0xDEADFACE;
        debug_log_copy.CurrentTime = ptime.can_time;
        debug_log_copy.BaseBoardTemperature = temperatureSensorVector[TEMPERATURE_SENSOR_BASE];
        debug_log_copy.MainBoardTemperature = temperatureSensorVector[TEMPERATURE_SENSOR_MAIN];
        debug_log_copy.MezzanineBoardTemperature = temperatureSensorVector[TEMPERATURE_SENSOR_MEZZANINE];
        debug_log_copy.PowerBankBoardTemperature = temperatureSensorVector[TEMPERATURE_SENSOR_PWR_BANK];


        /* update last read counter value */
        last_debug_log_number = debug_log_copy.counter;

        /* store data in ram */
        log_store_debug_log((unsigned char*) &debug_log_copy);
    }
}

void log_debug_read_from_ram() {
    volatile static uint32_t lastAddressRead = 0;
    debug_log_t readBack;

    EMIF1_Config emif1_ram_debug_log_read = {EXT_RAM_START_ADDRESS_CS2,
                                         CPU_TYPE_ONE,
                                         sizeof(debug_log_t),
                                         (uint16_t*)message};

    if( lastAddressRead != debug_log_last_writen_address) {
        emif1_ram_debug_log_read.address = debug_log_last_writen_address;
        emif1_ram_debug_log_read.cpuType = CPU_TYPE_ONE;
        emif1_ram_debug_log_read.size = sizeof(debug_log_t);
        emif1_ram_debug_log_read.data = (uint16_t*)message;

        memset(&readBack, 0, sizeof(debug_log_t) );
        memset((void *)message, 0, TRANSFER_SIZE );

        emifc_cpu_read_memory(&emif1_ram_debug_log_read);

        memcpy((void *)&readBack, (void *)message,  sizeof(debug_log_t));

        lastAddressRead = debug_log_last_writen_address;
        //Serial_debug(DEBUG_INFO, &cli_serial, "\033[2J");
        Serial_debug(DEBUG_INFO, &cli_serial, "\r\nVoltages:\r\n");
        Serial_debug(DEBUG_INFO, &cli_serial, "Vbus:[%03d] ", readBack.Vbus);
        Serial_debug(DEBUG_INFO, &cli_serial, "AvgVbus:[%03d] ", readBack.AvgVbus);
        Serial_debug(DEBUG_INFO, &cli_serial, "VStore:[%03d] ", readBack.VStore);
        Serial_debug(DEBUG_INFO, &cli_serial, "AvgVStore:[%03d] ", readBack.AvgVStore);
        Serial_debug(DEBUG_INFO, &cli_serial, "\r\nCurrents:\r\n");
        Serial_debug(DEBUG_INFO, &cli_serial, "IF_1:[%03d] ", readBack.IF_1);
        Serial_debug(DEBUG_INFO, &cli_serial, "ISen1:[%03d] ", readBack.ISen1);
        Serial_debug(DEBUG_INFO, &cli_serial, "ISen2:[%03d] ", readBack.ISen2);
        Serial_debug(DEBUG_INFO, &cli_serial, "I_Dab2:[%03d] ", readBack.I_Dab2);
        Serial_debug(DEBUG_INFO, &cli_serial, "I_Dab3:[%03d]\r\n", readBack.I_Dab3);
        Serial_debug(DEBUG_INFO, &cli_serial, "\r\nRegulate Vars:\r\n");
        Serial_debug(DEBUG_INFO, &cli_serial, "AvgVstore:[%03d] ", readBack.RegulateAvgVStore);
        Serial_debug(DEBUG_INFO, &cli_serial, "AvgVbus:[%03d] ", readBack.RegulateAvgVbus);
        Serial_debug(DEBUG_INFO, &cli_serial, "AvgInputCurrent:[%03d] ", readBack.RegulateAvgInputCurrent);
        Serial_debug(DEBUG_INFO, &cli_serial, "AvgOutpurCurrent:[%03d] ", readBack.RegulateAvgOutputCurrent);
        Serial_debug(DEBUG_INFO, &cli_serial, "Iref:[%03d]\r\n", readBack.RegulateIRef);

        Serial_debug(DEBUG_INFO, &cli_serial, "\r\nTemperatures:\r\n");
        Serial_debug(DEBUG_INFO, &cli_serial, "Base:   [%02d] ", readBack.BaseBoardTemperature);
        Serial_debug(DEBUG_INFO, &cli_serial, "Main:   [%02d] ", readBack.MainBoardTemperature);
        Serial_debug(DEBUG_INFO, &cli_serial, "Mezz:   [%02d] ", readBack.MezzanineBoardTemperature);
        Serial_debug(DEBUG_INFO, &cli_serial, "PWRBANK:[%02d] \r\n", readBack.PowerBankBoardTemperature);
        Serial_debug(DEBUG_INFO, &cli_serial, "\r\nOthers:\r\n");
        Serial_debug(DEBUG_INFO, &cli_serial, "Counter:     [%05d] ", readBack.counter);
        Serial_debug(DEBUG_INFO, &cli_serial, "CurrentState:[%02d] ", readBack.CurrentState);
        Serial_debug(DEBUG_INFO, &cli_serial, "Elapsed_time:[%08d] ", readBack.elapsed_time);
        Serial_debug(DEBUG_INFO, &cli_serial, "Time:[%08lu] \r\n", readBack.CurrentTime);

        Serial_debug(DEBUG_INFO, &cli_serial, "\r\nCell Voltages:");
        Serial_debug(DEBUG_INFO, &cli_serial, "\r\n");

        for(int c=0; c<NUMBER_OF_CELLS; c++){
            Serial_debug(DEBUG_INFO, &cli_serial, "%2d:[%03d] ", c+1, readBack.cellVoltage[c]);
            if( (c+1) % 6 == 0 ) {
                Serial_debug(DEBUG_INFO, &cli_serial, "\r\n");
            }
        }
        Serial_debug(DEBUG_INFO, &cli_serial, "\r\n=============================\r\n");
    }

}



void log_debug_read_from_flash() {
    volatile static uint32_t lastAddressRead = 0;
    debug_log_t readBack;


    if( lastAddressRead != debug_log_last_writen_address) {


        memset(&readBack, 0, sizeof(debug_log_t) );
        memset((void *)message, 0, TRANSFER_SIZE );

        //emifc_cpu_read_memory(&emif1_ram_debug_log_read);
        ext_flash_read_buf( debug_log_last_writen_address, (uint16_t*)message, sizeof(debug_log_t));

        memcpy((void *)&readBack, (void *)message,  sizeof(debug_log_t));

        lastAddressRead = debug_log_last_writen_address;
        //Serial_debug(DEBUG_INFO, &cli_serial, "\033[2J");
        Serial_debug(DEBUG_INFO, &cli_serial, "\r\nVoltages:\r\n");
        Serial_debug(DEBUG_INFO, &cli_serial, "Vbus:[%03d] ", readBack.Vbus);
        Serial_debug(DEBUG_INFO, &cli_serial, "AvgVbus:[%03d] ", readBack.AvgVbus);
        Serial_debug(DEBUG_INFO, &cli_serial, "VStore:[%03d] ", readBack.VStore);
        Serial_debug(DEBUG_INFO, &cli_serial, "AvgVStore:[%03d] ", readBack.AvgVStore);
        Serial_debug(DEBUG_INFO, &cli_serial, "\r\nCurrents:\r\n");
        Serial_debug(DEBUG_INFO, &cli_serial, "IF_1:[%03d] ", readBack.IF_1);
        Serial_debug(DEBUG_INFO, &cli_serial, "ISen1:[%03d] ", readBack.ISen1);
        Serial_debug(DEBUG_INFO, &cli_serial, "ISen2:[%03d] ", readBack.ISen2);
        Serial_debug(DEBUG_INFO, &cli_serial, "I_Dab2:[%03d] ", readBack.I_Dab2);
        Serial_debug(DEBUG_INFO, &cli_serial, "I_Dab3:[%03d]\r\n", readBack.I_Dab3);
        Serial_debug(DEBUG_INFO, &cli_serial, "\r\nRegulate Vars:\r\n");
        Serial_debug(DEBUG_INFO, &cli_serial, "AvgVstore:[%03d] ", readBack.RegulateAvgVStore);
        Serial_debug(DEBUG_INFO, &cli_serial, "AvgVbus:[%03d] ", readBack.RegulateAvgVbus);
        Serial_debug(DEBUG_INFO, &cli_serial, "AvgInputCurrent:[%03d] ", readBack.RegulateAvgInputCurrent);
        Serial_debug(DEBUG_INFO, &cli_serial, "AvgOutpurCurrent:[%03d] ", readBack.RegulateAvgOutputCurrent);
        Serial_debug(DEBUG_INFO, &cli_serial, "Iref:[%03d]\r\n", readBack.RegulateIRef);

        Serial_debug(DEBUG_INFO, &cli_serial, "\r\nTemperatures:\r\n");
        Serial_debug(DEBUG_INFO, &cli_serial, "Base:   [%02d] ", readBack.BaseBoardTemperature);
        Serial_debug(DEBUG_INFO, &cli_serial, "Main:   [%02d] ", readBack.MainBoardTemperature);
        Serial_debug(DEBUG_INFO, &cli_serial, "Mezz:   [%02d] ", readBack.MezzanineBoardTemperature);
        Serial_debug(DEBUG_INFO, &cli_serial, "PWRBANK:[%02d] \r\n", readBack.PowerBankBoardTemperature);
        Serial_debug(DEBUG_INFO, &cli_serial, "\r\nOthers:\r\n");
        Serial_debug(DEBUG_INFO, &cli_serial, "Counter:     [%05d] ", readBack.counter);
        Serial_debug(DEBUG_INFO, &cli_serial, "CurrentState:[%02d] ", readBack.CurrentState);
        Serial_debug(DEBUG_INFO, &cli_serial, "Elapsed_time:[%08d] ", readBack.elapsed_time);
        Serial_debug(DEBUG_INFO, &cli_serial, "Time:[%08lu] \r\n", readBack.CurrentTime);

        Serial_debug(DEBUG_INFO, &cli_serial, "\r\nCell Voltages:");
        Serial_debug(DEBUG_INFO, &cli_serial, "\r\n");

        for(int c=0; c<NUMBER_OF_CELLS; c++){
            Serial_debug(DEBUG_INFO, &cli_serial, "%2d:[%03d] ", c+1, readBack.cellVoltage[c]);
            if( (c+1) % 6 == 0 ) {
                Serial_debug(DEBUG_INFO, &cli_serial, "\r\n");
            }
        }

        Serial_debug(DEBUG_INFO, &cli_serial, "\r\ncan_log_next_free_address:[0x%08p] \r\n", can_log_next_free_address);
        Serial_debug(DEBUG_INFO, &cli_serial, "can_log_last_read_address:[0x%08p] \r\n", can_log_last_read_address);
        Serial_debug(DEBUG_INFO, &cli_serial, "can_log_start_address:    [0x%08p] \r\n", can_log_start_address);

        Serial_debug(DEBUG_INFO, &cli_serial, "\r\n=============================\r\n");
    }

}


/* base on coOdGetObjSize() */
void getObjData(
        CO_CONST CO_OBJECT_DESC_T *pDesc,
        void     *pObj,  /**< pointer for description index */
        UNSIGNED16  index,
        UNSIGNED8   subIndex
    )
{
    switch (pDesc->dType)
    {
        case CO_DTYPE_U8_CONST:
        case CO_DTYPE_BOOL_CONST:
            coOdGetObj_u8(index, subIndex, pObj);
            break;
        case CO_DTYPE_U16_CONST:
            coOdGetObj_u16(index, subIndex, pObj);
            break;
        case CO_DTYPE_U32_CONST:
            coOdGetObj_u32(index, subIndex, pObj);
            break;
        case CO_DTYPE_R32_CONST:
            coOdGetObj_r32(index, subIndex, pObj);
            break;
        case CO_DTYPE_U8_VAR:
        case CO_DTYPE_BOOL_VAR:
            coOdGetObj_u8(index, subIndex, pObj);
            break;
        case CO_DTYPE_U16_VAR:
            coOdGetObj_u16(index, subIndex, pObj);
            break;
        case CO_DTYPE_U32_VAR:
            coOdGetObj_u32(index, subIndex, pObj);
            break;
        case CO_DTYPE_R32_VAR:
            coOdGetObj_r32(index, subIndex, pObj);
            break;
        case CO_DTYPE_U8_PTR:
        case CO_DTYPE_BOOL_PTR:
            /* table initialized ? */
            coOdGetObj_u8(index, subIndex, pObj);
            break;
        case CO_DTYPE_U16_PTR:
//            /* table initialized ? */
//            if (pOdPtr_u16 != NULL)  {
//                size = 2u;
//            }
            break;
        case CO_DTYPE_U32_PTR:
//            /* table initialized ? */
//            if (pOdPtr_u32 != NULL)  {
//                size = 4u;
//            }
            break;
        case CO_DTYPE_I8_CONST:
            coOdGetObj_i8(index, subIndex, pObj);
            break;
        case CO_DTYPE_I16_CONST:
            coOdGetObj_i16(index, subIndex, pObj);
            break;
        case CO_DTYPE_I32_CONST:
            coOdGetObj_i32(index, subIndex, pObj);
            break;
        case CO_DTYPE_I8_VAR:
            coOdGetObj_i8(index, subIndex, pObj);
            break;
        case CO_DTYPE_I16_VAR:
            coOdGetObj_i16(index, subIndex, pObj);
            break;
        case CO_DTYPE_I32_VAR:
            coOdGetObj_i32(index, subIndex, pObj);
            break;
        case CO_DTYPE_I8_PTR:
//            /* table initialized ? */
//            if (pOdPtr_i8 != NULL)  {
//                SIZE = 1U;
//            }
            break;
        case CO_DTYPE_I16_PTR:
//            /* table initialized ? */
//            if (pOdPtr_i16 != NULL)  {
//                size = 2u;
//            }
            break;
        case CO_DTYPE_I32_PTR:
//            /* table initialized ? */
//            if (pOdPtr_i32 != NULL)  {
//                size = 4u;
//            }
            break;
        case CO_DTYPE_R32_PTR:
//            /* table initialized ? */
//            if (pOdPtr_r32 != NULL)  {
//                size = 4u;
//            }
            break;
        case CO_DTYPE_VS_PTR:
//            if (pOdPtr_VS_ActLen != NULL)  {
//                size = pOdPtr_VS_ActLen[pDesc->tableIdx];
//            }
            break;
        case CO_DTYPE_VS_CONST:
//            if (pOdConst_VS_MaxLen != NULL)  {
//                size = pOdConst_VS_MaxLen[pDesc->tableIdx];
//            }
            break;
        case CO_DTYPE_OS_PTR:
//            if (pOdPtr_OS_ActLen != NULL)  {
//                size = pOdPtr_OS_ActLen[pDesc->tableIdx];
//            }
            break;
        case CO_DTYPE_DOMAIN:
//            if (pOdPtr_Domain_ActLen != NULL)  {
//                size = pOdPtr_Domain_ActLen[pDesc->tableIdx];
//            }
            break;
        case CO_DTYPE_U8_SDO_SERVER:
        case CO_DTYPE_U8_SDO_CLIENT:
        case CO_DTYPE_U8_TPDO:
        case CO_DTYPE_U8_RPDO:
        case CO_DTYPE_U8_TMAP:
        case CO_DTYPE_U8_RMAP:
        case CO_DTYPE_U8_SYNC:
        case CO_DTYPE_U8_EMCY:
        case CO_DTYPE_U8_ERRCTRL:
        case CO_DTYPE_U8_NETWORK:
        case CO_DTYPE_U8_NMT:
        case CO_DTYPE_U8_SRDO:
        case CO_DTYPE_U8_GFC:
            coOdGetObj_u8(index, subIndex, pObj);
            break;
        case CO_DTYPE_U16_TPDO:
        case CO_DTYPE_U16_RPDO:
        case CO_DTYPE_U16_ERRCTRL:
        case CO_DTYPE_U16_EMCY:
        case CO_DTYPE_U16_NMT:
        case CO_DTYPE_U16_NETWORK:
        case CO_DTYPE_U16_SRDO:
        case CO_DTYPE_U16_FLYMA:
            coOdGetObj_u16(index, subIndex, pObj);
            break;
        case CO_DTYPE_U32_SDO_SERVER:
        case CO_DTYPE_U32_SDO_CLIENT:
        case CO_DTYPE_U32_TPDO:
        case CO_DTYPE_U32_TMAP:
        case CO_DTYPE_U32_RPDO:
        case CO_DTYPE_U32_RMAP:
        case CO_DTYPE_U32_ERRCTRL:
        case CO_DTYPE_U32_SYNC:
        case CO_DTYPE_U32_EMCY:
        case CO_DTYPE_U32_TIME:
        case CO_DTYPE_U32_STORE:
        case CO_DTYPE_U32_SRD:
        case CO_DTYPE_U32_NETWORK:
        case CO_DTYPE_U32_NMT:
        case CO_DTYPE_U32_SRDO:
            coOdGetObj_u32(index, subIndex, pObj);
            break;
        default:
            ;
    }
}

//void log_store_pdo_ds401(uint8_t port)
//{
//    CO_CONST CO_OBJECT_DESC_T *pObjDesc;
//    unsigned char log_sent_data[8] ={0};
//
//    log_sent_data[0] = '4';
//    coOdGetObjDescPtr(0x6000u + CO_401_PROFILE_OFFSET, port, &pObjDesc);
//    getObjData(pObjDesc, &log_sent_data[4], 0x6000u + CO_401_PROFILE_OFFSET, port);
//    //log_store_can_log(8, log_sent_data);
//}
//
//void log_store_new_state(uint16_t state)
//{
//    unsigned char log_state_data[8] ={0};
//
//    log_state_data[0] = 'S';
//    log_state_data[1] = (state >> 0) & 0xff;
//    log_state_data[2] = (state >> 8) & 0xff;
//    //log_store_can_log(3, log_state_data);
//}

//static uint16_t find_first_log_entry_in_next_sector(uint32_t can_log_start_address)
//{
//    ext_flash_desc_t sector_info;
//    look_up_start_address_of_sector(&sector_info);
//    int present_sector = sector_info.sector;
//
//    uint16_t address = can_log_start_address;
//    uint16_t log_entry_size;
//    uint8_t log_type;
//    uint16_t log_type_address;
//
//    // TODO: start by searching for magic number
//    /* first log entry */
//    log_type_address = address + 3;   /* two words, four Bytes, for timestamp and magic number */
//    memcpy(&log_type, (const void*)log_type_address, 1);
//    log_entry_size = log_size_of_CAN_log_entry(log_type);
//
//    while(1) {
//        /* next log entry */
//        address += log_entry_size;
//        log_type_address = address + 3;   /* two words, four Bytes */
//        memcpy(&log_type, (const void*)log_type_address, 1);
//        if(0xffff != log_type)
//        {
//            /* OK - we have a log entry here
//             * probably an error state
//             * */
//        }
//        log_entry_size = log_size_of_CAN_log_entry(log_type);
//
//        look_up_start_address_of_sector(&sector_info);
//        int next_sector = sector_info.sector;
//
//        if(present_sector != next_sector)
//        {
//            /* we have entered next sector
//             * we have found the address of the first log entry in the next sector
//             * */
//
//            //TODO handle wrap around
//            break;
//        }
//    }
//
//    return address;
//}


//static uint16_t log_size_of_CAN_log_entry(uint8_t log_type)
//{
//    uint16_t size_in_Bytes;
//    uint16_t size_in_words;
//
//    /* returns number of Bytes */
//    switch (log_type) {
//    case '4':   // stored by log_store_pdo_ds401()
//    case 'E': /* EMCY */
//    case 'R': /* SDO upload */
//    case 'W': /* SDO download */
//        size_in_Bytes = 8 + SIZE_OF_TIMESTAMP;
//        break;
//
//    case 'P': /* PDO upload */                  // NOTE: Currently not used!
//        size_in_Bytes = 8 + SIZE_OF_TIMESTAMP;  // TODO: not decided
//        break;
//
//    case 'S': /* state of DPMU state machine */ // NOTE: Currently not used!
//        size_in_Bytes = 6 + SIZE_OF_TIMESTAMP;
//        break;
//
//    default:    // TODO: decide what to do here!
//        break;
//    }
//
//    /* DMA requires 16 bit words
//     * convert size to smallest size that fits the size of the data to write
//     */
//    if (size_in_Bytes & 0b01)
//        /* add a empty byte to match whole 16 bit words */
//        size_in_words = size_in_Bytes / 2 + 1;
//    else
//        size_in_words = size_in_Bytes / 2;
//
//    return size_in_words;
//}

//void log_can_init_hb(void)
//{
//    bool good_log;
//
//    // reset number of entries
//    // reset position in memory to beginning of RAM portion ???
//
//    // If no entry found, set to initial values.
//    can_log_start_address              = CAN_LOG_ADDRESS_START;
//    can_log_next_free_address          = CAN_LOG_ADDRESS_START;
//    can_log_address_has_wrapped_around = false;
//    can_log_size                       = 0;
//
//    good_log = can_log_search_log();
//    if ((!good_log) || (can_log_start_address == can_log_next_free_address)) {
//        /* reset pointers and size
//         * needed for log error
//         */
//        can_log_start_address              = CAN_LOG_ADDRESS_START;
//        can_log_next_free_address          = CAN_LOG_ADDRESS_START;
//        can_log_address_has_wrapped_around = false;
//        can_log_size                       = 0;
//
//        /* for some reason, if we erase the sectors in opposite order we get a
//         * problem,  first word in first write attempt in first can log sector
//         * will no happen, it will continue to keep the value 0xff
//         */
//        // Erase log part of external flash.
//        for (ext_flash_sector_t sector = LAST_LOG_SECTOR; sector >= FIRST_LOG_SECTOR; --sector) {
//            ext_flash_erase_sector(ex_flash_info[sector].addr);
//        }
//    }
//}
//

//
//bool can_log_search_log(void)
//{
//    bool noError = true; //TODO implement the checks for errors !
//    uint16_t tmp;
//    uint32_t addr = can_log_start_address;
//    bool log_start_address_set = false;
//    bool possible_wrap_around_detected = false;
//
//    /* search for first consecutive block of log entries
//     * could at maximum be two blocks of consecutive log entries,
//     * one in the end of the log that stretches over the log area and continues
//     * at the beginning of log area -> one block in the end and one block in the
//     * beginning
//     * */
//    do
//    {
//        /* read address - for magic number */
//        ext_flash_read_buf(addr, &tmp, 1);
//
//        /* is it Magic Number */
//        if(tmp == CAN_LOG_MAGIC)
//        {
//            /* save presumed start address of log
//             * at this stage, presume no wrap around, we handle that later
//             * */
//            if(!log_start_address_set)
//            {
//                can_log_start_address = addr;
//                log_start_address_set = true;
//            }
//
//            /* read size of log entry */
//            ext_flash_read_buf(addr + 2, &tmp, 1);  /* skip timestamp */
//            addr += tmp;
//            can_log_next_free_address = addr;
//
//            /* error if the last log entry stretches
//             * over the end of the log area
//             * */
//            if(can_log_next_free_address > CAN_LOG_ADDRESS_END)
//            {
//                noError = false;
//                return noError;
//            }
//        }
//        else
//        {
//            /* try at next possible address */
//            addr++;
//        }
//
//        /* have we found the end of the first consecutive block of log entries
//         * tmp != CAN_LOG_MAGIC - can_log_next_free_address does not contain
//         *                        Magic Number, possibly found end of log
//         *                        log could stretch over end of log area (wrap
//         *                        around)
//         * start_address_set - we have found at least one log entry
//         * */
//        if(tmp != CAN_LOG_MAGIC && log_start_address_set)
//        {
//            break;
//        }
//    } while(addr < CAN_LOG_ADDRESS_END); /* search up to the full log area */
//
//    /* we have found one consecutive block of log entries
//     * does there exist one more?
//     *
//     * the log can start in the middle of the log area, wrap around the end
//     * and continue from the beginning
//     * */
//    if(addr < CAN_LOG_ADDRESS_END)
//    {
//        if(tmp != CAN_LOG_MAGIC && log_start_address_set)
//        {
//            do
//            {
//                /* read address - for magic number */
//                ext_flash_read_buf(addr, &tmp, 1);
//
//                /* is it Magic Number */
//                if(tmp == CAN_LOG_MAGIC)
//                {
//                    /* save presumed start address of log */
//                    if(!possible_wrap_around_detected)
//                    {
//                        /* the log starts here
//                         * the log has wrapped around
//                         * the log ends somewhere before this address
//                         * */
//                        can_log_start_address = addr;
//                        possible_wrap_around_detected = true;
//                    }
//
//                    /* read size of log entry */
//                    ext_flash_read_buf(addr + 2, &tmp, 1);  /* skip timestamp */
//                    addr += tmp;
//
//                    /* keep code commented out as reference of difference
//                     *
//                     * the real end of log appears before this block of
//                     * consecutive log entries and updated earlier
//                     * */
////                    can_log_next_free_address = addr;
//
//                    /* error if the last log entry stretches
//                     * over the end of the log area
//                     * */
//                    if(addr > CAN_LOG_ADDRESS_END)
//                    {
//                        noError = false;
//                        return noError;
//                    }
//                }
//                else
//                {
//                    /* try at next possible address */
//                    addr++;
//                }
//
//                /* have we found the end of the second consecutive blocks of log
//                 * entries
//                 * tmp != CAN_LOG_MAGIC - next possible log entry does not
//                 *                        contain Magic Number, found end of
//                 *                        second block of consecutive log
//                 *                        entries, log stretches over end of log
//                 *                        area (wrap around)
//                 * */
//                if(tmp != CAN_LOG_MAGIC)
//                {
//                    break;
//                }
//            } while(addr < CAN_LOG_ADDRESS_END);
//        }
//    }
//
//    /* we have found two consecutive block of log entries
//     * does there exist one more? if so we have an error
//     *
//     * the log can start in the middle of the log area, wrap around the end
//     * and continue from the beginning
//     * */
//    if(addr < CAN_LOG_ADDRESS_END)
//    {
//        if(possible_wrap_around_detected)
//        {
//            do
//            {
//                /* read address - for magic number */
//                ext_flash_read_buf(addr, &tmp, 1);
//
//                /* is it Magic Number */
//                if(tmp == CAN_LOG_MAGIC)
//                {
//                    /* we have found three consecutive blocks of log entries
//                     * maximum is consecutive blocks of log entries
//                     * the log can start in the middle of the log area,
//                     * wrap around the end and continue from the beginning,
//                     * meaning two consecutive blocks of log entries
//                     * */
//                    noError = false;
//                    return noError;
//                }
//                else
//                {
//                    /* try at next possible address */
//                    addr++;
//                }
//            } while(addr < CAN_LOG_ADDRESS_END);
//        }
//    }
//
//    return noError;
//}
//

