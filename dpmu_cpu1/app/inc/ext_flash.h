/*
 * ext_flash.h
 *
 *  Created on: 16 mars 2023
 *      Author: us
 */

#ifndef APP_INC_EXT_FLASH_H_
#define APP_INC_EXT_FLASH_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Start of memory area selected by EMIF1 CS3.
#define EXT_FLASH_START_ADDRESS_CS3 0x00300000U

// End of memory area selected by EMIF1 CS3.
#define EXT_FLASH_END_ADDRESS_CS3   0x0037FFFFU

// Accessible size of memory area selected by EMIF1 CS3. This is 512k x 16 = 1MB.
#define EXT_FLASH_SIZE_CS3          (EXT_FLASH_END_ADDRESS_CS3 - EXT_FLASH_START_ADDRESS_CS3 + 1)

// Real size of the external flash connected to EMIF1 CS3. This is 1M x 16 = 2MB.
#define EXT_FLASH_SIZE              (2 * EXT_FLASH_SIZE_CS3)

// Flash logging area
#define CAN_LOG_ADDRESS_START   (EXT_FLASH_START_ADDRESS_CS3 + 0x8000 /*ext_flash_info[FIRST_LOG_SECTOR].addr*/)
#define CAN_LOG_ADDRESS_END     (EXT_FLASH_START_ADDRESS_CS3 + EXT_FLASH_SIZE_CS3)


// Application variables logging area
//#define APP_VARS_EXT_FLASH_SIZE 0x2000
#define APP_VARS_EXT_FLASH_SIZE 0x200
//#define APP_VARS_EXT_FLASH_ADDRESS_START   ( EXT_FLASH_START_ADDRESS_CS3 )
#define APP_VARS_EXT_FLASH_ADDRESS_START   ( EXT_FLASH_START_ADDRESS_CS3 ) + 0x100
#define APP_VARS_EXT_FLASH_ADDRESS_END     ( APP_VARS_EXT_FLASH_ADDRESS_START + (APP_VARS_EXT_FLASH_SIZE - 1) )


typedef enum {
    EXT_FLASH_BUF_WRITE_DONE = 0,
    EXT_FLASH_BUF_WAITING,
    EXT_FLASH_BUF_WRITE_ONGOING,
    EXT_FLASH_BUF_WRITE_TIME_OUT
} ext_flash_buff_write_status_t;

typedef enum {
    EXT_FLASH_SA0 = 0,
    EXT_FLASH_SA1,
    EXT_FLASH_SA2,
    EXT_FLASH_SA3,
    EXT_FLASH_SA4,
    EXT_FLASH_SA5,
    EXT_FLASH_SA6,
    EXT_FLASH_SA7,
    EXT_FLASH_SA8,
    EXT_FLASH_SA9,
    EXT_FLASH_SA10,
    EXT_FLASH_SA11,
    EXT_FLASH_SA12,
    EXT_FLASH_SA13,
    EXT_FLASH_SA14,
    EXT_FLASH_SA15,
    EXT_FLASH_SA16,
    EXT_FLASH_SA17,
    EXT_FLASH_SA18,
    EXT_FLASH_SA19,
    EXT_FLASH_SA20,
    EXT_FLASH_SA21,
    EXT_FLASH_SA22,
    EXT_FLASH_SA23,
    EXT_FLASH_SA24,
    EXT_FLASH_SA25,
    EXT_FLASH_SA26,
    EXT_FLASH_SA27,
    EXT_FLASH_SA28,
    EXT_FLASH_SA29,
    EXT_FLASH_SA30,
    EXT_FLASH_SA31,
    EXT_FLASH_SA32,
    EXT_FLASH_SA33,
    EXT_FLASH_SA34,
    EXT_FLASH_SA_LAST
} ext_flash_sector_t;

/**
 * A structure describing the layout etc. of the external flash sectors.
 */
typedef struct ext_flash_desc_s
{
    ext_flash_sector_t sector;      // sector (in range EXT_FLASH_SA0..EXT_FLASH_SA34)
    uint32_t addr;                  // start offset TODO: maybe rename this from 'addr' to 'offset'
    size_t size;                    // sector size
} ext_flash_desc_t;

/**
 * Look up inf regarding the sector
 * start address and size
 * returns false if sector was not found
 */
void look_up_start_address_of_sector(ext_flash_desc_t *sector_desc);



/**
 * Converts address to flash sector.
 */
const ext_flash_desc_t *ext_flash_sector_from_address(uint32_t addr);

///**
// * Converts address to flash sector.
// */
//ext_flash_sector_t ext_flash_sector_from_address(uint32_t addr);

/**
 * Reads single 16b word from flash address.
 */
uint16_t ext_flash_read_word(uint32_t addr);

/**
 * Reads buffer of 16b words from flash address.
 */
void ext_flash_read_buf(uint32_t addr, uint16_t *buf, size_t len);

/**
 * Programs single 16b word to Flash address.
 */
void ext_flash_write_word(uint32_t addr, uint16_t data);

/**
 * Programs buffer of 16b words to Flash address.
 */
void ext_flash_write_buf(uint32_t addr, uint16_t *buf, size_t len);

/**
 * Erases entire Flash memory space.
 */
void ext_flash_chip_erase(void);

/**
 * Erases addressed Flash sector.
 */
//void ext_flash_erase_sector(uint32_t addr);
void ext_flash_erase_sector_by_descriptor(ext_flash_desc_t *sector_desc);
void ext_flash_erase_sector(uint32_t sa);
void start_ext_flash_erase_sector(uint32_t sa);
bool verify_ext_flash_erase_sector_done();

/**
 * Resets the external flash.
 */
void ext_flash_reset(void);

/**
 * Configures EMIF1 registers for accessing external flash on CS3.
 */
void ext_flash_config(void);

/**
 * Tests external flash API.
 */
void ext_flash_test(void);

/**
 * Verifies if the flash is ready after an erase
 */
bool ext_flash_ready(void);

/**
 * Asserts command to fully erase the flash memory
 */
void ext_command_flash_chip_erase(void);

/**
 * Initialize the parameters for the non ext_flash_non_blocking_read_buf function
 */
void ext_flash_init_non_blocking_read(uint32_t addr, uint16_t *bufferAddr, size_t len);


/**
 * Initialize the parameters for the non ext_flash_non_blocking_write_buf function
 */
void ext_flash_init_non_blocking_write(uint32_t addr, uint16_t *bufferAddr, size_t len);

/**
 * Reads buffer of 16b words from flash address without blocking in a while loop.
 */
bool ext_flash_non_blocking_read_buf();

/**
 * Write buffer of 16b words from flash address without blocking in a while loop.
 * Returns status of the writing.
 *      EXT_FLASH_BUF_WAITING,
 *      EXT_FLASH_BUF_WRITE_ONGOING,
 * When done OK returns EXT_FLASH_BUF_WRITE_DONE
 * When done NOT OK returns EXT_FLASH_BUF_WRITE_TIME_OUT
 */
ext_flash_buff_write_status_t ext_flash_non_blocking_write_buf();

// Information base describing the external flash.
extern const ext_flash_desc_t ex_flash_info[];

// Data buffer mapped to external flash. (CS3 area is max 1MB.)
extern uint16_t g_ext_flash_data[EXT_FLASH_SIZE_CS3];



#endif /* APP_INC_EXT_FLASH_H_ */
