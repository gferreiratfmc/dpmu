/*
 * ext_flash.c
 *
 *  Created on: 16 mars 2023
 *      Author: us
 */

#include <stdio.h>
#include <assert.h>

#include "driverlib.h"
#include "device.h"
#include "emif.h"
#include "serial.h"
#include "GlobalV.h"
#include "gpio.h"
#include "ext_flash.h"

/**
 * Definitions.
 */
#define MEM_RW_ITER                 0x1U
#define BUFFER_WORDS                256
#define EXT_FLASH_RESET_DELAY       50      // 50 us delay for active low RESET
#define EXT_FLASH_BUSY_DELAY        5       // 1 us delay between Flash Command and Read/Busy# valid
#define EXT_FLASH_RESET             33      // GPIO pin connected to external flash RESET pin
#define EXT_FLASH_READY             36      // GPIO pin connected to external flash RDY/BSY pin
#define EXT_FLASH_A19               91

#define EXT_FLASH_WRITE_MAX_COUNT_TIME_OUT 10

/**
 * Macro definitions.
 */
#define FLASH_SEQ(addr, data)   do { *(uint16_t *)(EXT_FLASH_START_ADDRESS_CS3 + (addr)) = (data); } while (0)

/**
 * External references.
 */
extern struct Serial cli_serial;

uint32_t non_blocking_read_addr;
uint16_t *non_blocking_read_buff_ptr;
size_t non_blocking_read_count;
size_t   non_blocking_read_len;

uint32_t non_blocking_write_addr;
uint16_t *non_blocking_write_buff_ptr;
size_t non_blocking_write_count;
size_t   non_blocking_write_len;
bool non_blocking_write_buff_start_flag;

/**
 * Global data.
 */
const ext_flash_desc_t ex_flash_info[] = {
    EXT_FLASH_SA0,   0x000000, 0x002000,
    EXT_FLASH_SA1,   0x002000, 0x001000,
    EXT_FLASH_SA2,   0x003000, 0x001000,
    EXT_FLASH_SA3,   0x004000, 0x004000,
    EXT_FLASH_SA4,   0x008000, 0x008000,
    EXT_FLASH_SA5,   0x010000, 0x008000,
    EXT_FLASH_SA6,   0x018000, 0x008000,
    EXT_FLASH_SA7,   0x020000, 0x008000,
    EXT_FLASH_SA8,   0x028000, 0x008000,
    EXT_FLASH_SA9,   0x030000, 0x008000,
    EXT_FLASH_SA10,  0x038000, 0x008000,
    EXT_FLASH_SA11,  0x040000, 0x008000,
    EXT_FLASH_SA12,  0x048000, 0x008000,
    EXT_FLASH_SA13,  0x050000, 0x008000,
    EXT_FLASH_SA14,  0x058000, 0x008000,
    EXT_FLASH_SA15,  0x060000, 0x008000,
    EXT_FLASH_SA16,  0x068000, 0x008000,
    EXT_FLASH_SA17,  0x070000, 0x008000,
    EXT_FLASH_SA18,  0x078000, 0x008000,
    EXT_FLASH_SA19,  0x080000, 0x008000,
    EXT_FLASH_SA20,  0x088000, 0x008000,
    EXT_FLASH_SA21,  0x090000, 0x008000,
    EXT_FLASH_SA22,  0x098000, 0x008000,
    EXT_FLASH_SA23,  0x0A0000, 0x008000,
    EXT_FLASH_SA24,  0x0A8000, 0x008000,
    EXT_FLASH_SA25,  0x0B0000, 0x008000,
    EXT_FLASH_SA26,  0x0B8000, 0x008000,
    EXT_FLASH_SA27,  0x0C0000, 0x008000,
    EXT_FLASH_SA28,  0x0C8000, 0x008000,
    EXT_FLASH_SA29,  0x0D0000, 0x008000,
    EXT_FLASH_SA30,  0x0D8000, 0x008000,
    EXT_FLASH_SA31,  0x0E0000, 0x008000,
    EXT_FLASH_SA32,  0x0E8000, 0x008000,
    EXT_FLASH_SA33,  0x0F0000, 0x008000,
    EXT_FLASH_SA34,  0x0F8000, 0x008000
};

#pragma DATA_SECTION(g_ext_flash_data, ".em1_cs3");
uint16_t g_ext_flash_data[EXT_FLASH_SIZE_CS3];


/**
 * Local data.
 */
//static uint32_t l_active_page = 0;

void look_up_start_address_of_sector(ext_flash_desc_t *sector_desc)
{
    /* add virtual address offset of external flash */
    sector_desc->addr = ex_flash_info[sector_desc->sector].addr +
                                                    EXT_FLASH_START_ADDRESS_CS3;
    sector_desc->size = ex_flash_info[sector_desc->sector].size;
}

static inline uint32_t a19(uint32_t addr)
{
    return addr & 0x80000 ? 1 : 0;
}

static inline uint32_t set_a19(uint32_t addr)
{
    uint32_t page = a19(addr);

    GPIO_writePin(EXT_FLASH_A19, page);

    return page;
}

/**
 * enter_CFI - Enter Flash CFI mode to access device information
 */
static inline void enter_CFI(void)
{
    set_a19(0);

    FLASH_SEQ(0x55, 0x98);
}

/**
 * exit_CFI - Exit Flash CFI mode for normal operation
 */
static inline void exit_CFI(void)
{
    set_a19(0);

    FLASH_SEQ(0x00, 0xF0);
}

/**
 * Converts address to flash descriptor pointer.
 */
const ext_flash_desc_t *ext_flash_sector_from_address(uint32_t addr)
{
    static ext_flash_sector_t map[] = {
            EXT_FLASH_SA0,
            EXT_FLASH_SA0,
            EXT_FLASH_SA1,
            EXT_FLASH_SA2,
            EXT_FLASH_SA3,
            EXT_FLASH_SA3,
            EXT_FLASH_SA3,
            EXT_FLASH_SA3
    };

    uint32_t k = (addr & 0xff000) >> 12;
    ext_flash_sector_t sector;

    if (k < 0x08) {
        sector = map[k];
    } else {
        sector = (ext_flash_sector_t)((k >> 3) + EXT_FLASH_SA3);
    }

    return &ex_flash_info[sector];
}

/**
 * Reads single 16b word from flash address.
 */
uint16_t ext_flash_read_word(uint32_t addr)
{
    set_a19(addr);

    uint16_t data = *(uint16_t *)addr;

    return data;
}

/**
 * Reads buffer of 16b words from flash address.
 */
void ext_flash_read_buf(uint32_t addr, uint16_t *buf, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        uint16_t data = ext_flash_read_word(addr + i);
        *buf++ = data;
    }
}

/**
 * Initialize the parameters for the non ext_flash_non_blocking_read_buf function
 */
void ext_flash_init_non_blocking_read(uint32_t addr, uint16_t *bufferAddr, size_t len)
{
    non_blocking_read_count = 0;
    non_blocking_read_addr = addr;
    non_blocking_read_len = len;
    non_blocking_read_buff_ptr = bufferAddr;
}

/**
 * Reads buffer of 16b words from flash address without blocking in a while loop.
 * Function should be used in external loop like a state machine.
 * Before calling this funciton it shall be initialized calling ext_flash_init_non_blocking_read
 */
bool ext_flash_non_blocking_read_buf()
{
    uint16_t data;
    bool ret = false;
    if( non_blocking_read_len == 0 || non_blocking_read_buff_ptr == 0 || non_blocking_read_addr == 0) {
        return false;
    }
    if( non_blocking_read_count  < non_blocking_read_len ) {
        data = ext_flash_read_word(non_blocking_read_addr);
//        Serial_debug(DEBUG_INFO, &cli_serial, "Non block flash read addr:[0x%08p] data:[0x%04X]\r\n", non_blocking_read_addr, data);
        *non_blocking_read_buff_ptr = data;
        non_blocking_read_count++;
        non_blocking_read_buff_ptr++;
        non_blocking_read_addr++;
    } else {
        non_blocking_read_buff_ptr = 0;
        non_blocking_read_len = 0;
        non_blocking_read_addr = 0;
        ret = true;
    }
    return ret;
}




/**
 * ext_flash_write_word - Program single 16b word to Flash address
 */
void ext_flash_write_word(uint32_t addr, uint16_t data)
{
    // Make sure the flash is erased, otherwise function will hang during write.
    if (ext_flash_read_word(addr) != 0xffff) {
        return;
    }
    set_a19(0);

    FLASH_SEQ(0x555, 0xAA);
    FLASH_SEQ(0x2AA, 0x55);
    FLASH_SEQ(0X555, 0xA0);

    set_a19(addr);

    *(uint16_t* ) addr = data;

    DEVICE_DELAY_US(EXT_FLASH_BUSY_DELAY); // Wait for Ready/Busy# signal to become valid

    while ( !ext_flash_ready() ) {
        // Wait for Flash to complete command
        //TODO locking
    }
}


/**
 * ext_flash_write_word - Program single 16b word to Flash address
 */
void ext_flash_non_blocking_write_word(uint32_t addr, uint16_t data)
{
    // Make sure the flash is erased, otherwise function will hang during write.
    if (ext_flash_read_word(addr) != 0xffff) {
        return;
    }
    set_a19(0);

    FLASH_SEQ(0x555, 0xAA);
    FLASH_SEQ(0x2AA, 0x55);
    FLASH_SEQ(0X555, 0xA0);

    set_a19(addr);

    *(uint16_t* ) addr = data;

}

void ext_command_flash_chip_erase(void) {
    set_a19(0);

    FLASH_SEQ(0x555, 0xAA);
    FLASH_SEQ(0x2AA, 0x55);
    FLASH_SEQ(0x555, 0x80);
    FLASH_SEQ(0x555, 0xAA);
    FLASH_SEQ(0x2AA, 0x55);
    FLASH_SEQ(0x555, 0x10);

    DEVICE_DELAY_US(EXT_FLASH_BUSY_DELAY); // Wait for Ready/Busy# signal to become valid

}

bool ext_flash_ready() {

    if ( GPIO_readPin(EXT_FLASH_READY) == 0 ) {
        return false;
    } else {
        return true;
    }
}

/**
 * Programs buffer of 16b words to Flash address.
 */
void ext_flash_write_buf(uint32_t addr, uint16_t *buf, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        uint16_t data = *buf++;
        ext_flash_write_word(addr + i, data);
    }
}

/**
 * Initialize the parameters for the non ext_flash_non_blocking_w_buf function
 */
void ext_flash_init_non_blocking_write(uint32_t addr, uint16_t *bufferAddr, size_t len)
{
    non_blocking_write_count = 0;
    non_blocking_write_addr = addr;
    non_blocking_write_len = len;
    non_blocking_write_buff_ptr = bufferAddr;
    non_blocking_write_buff_start_flag = true;

    Serial_debug(DEBUG_INFO, &cli_serial, "non_blocking_write_count=[%d], non_blocking_write_addr=[0x%08p], non_blocking_write_len=[%d], "
            "non_blocking_write_buff_ptr=[0x%08p], non_blocking_write_buff_start_flag = [%d]",
    non_blocking_write_count, non_blocking_write_addr, non_blocking_write_len = len, non_blocking_write_buff_ptr , non_blocking_write_buff_start_flag);
}

ext_flash_buff_write_status_t ext_flash_non_blocking_write_buf()
{
    static ext_flash_buff_write_status_t retVal = EXT_FLASH_BUF_WAITING;
    static uint16_t timeout_count=0;
    enum EFWStates { EFWWaitStart = 0, EFWInit,
                     EFWWrite, EFWIncrement, EFWWaitExtFlashReady,
                     EFWEndOk, EFWEndError};


    static States_t EFWSM = { 0 };


    switch( EFWSM.State_Current ) {

        case EFWWaitStart:
            retVal = EXT_FLASH_BUF_WAITING;
            if( non_blocking_write_buff_start_flag == true ) {
                retVal = EXT_FLASH_BUF_WRITE_ONGOING;
                EFWSM.State_Next = EFWInit;
            }
            break;

        case EFWInit:
                if( non_blocking_write_len == 0 ||
                    non_blocking_write_buff_ptr == 0 ||
                    non_blocking_write_addr == 0 ) {

                    EFWSM.State_Next = EFWEndOk;
                } else {
                    EFWSM.State_Next = EFWWrite;
                }
            break;

        case EFWWrite:
            Serial_debug(DEBUG_INFO, &cli_serial,"ext_flash write addr[0x%08p] data[0x%04X]\r\n",
                         non_blocking_write_addr, *non_blocking_write_buff_ptr );
            ext_flash_non_blocking_write_word(non_blocking_write_addr, (*non_blocking_write_buff_ptr) );
            timeout_count=0;
            EFWSM.State_Next = EFWWaitExtFlashReady;
            break;

        case EFWWaitExtFlashReady:
            DEVICE_DELAY_US(EXT_FLASH_BUSY_DELAY); // Wait for Ready/Busy# signal to become valid

            if( ext_flash_ready() == true ) {
                EFWSM.State_Next = EFWIncrement;
            } else {
                timeout_count++;
                if( timeout_count >= EXT_FLASH_WRITE_MAX_COUNT_TIME_OUT ) {
                    EFWSM.State_Next = EFWEndError;
                } else {
                    EFWSM.State_Next = EFWWaitExtFlashReady;
                }
            }
            break;

        case EFWIncrement:
            non_blocking_write_count++;
            if( non_blocking_write_count < non_blocking_write_len ) {
                non_blocking_write_addr++;
                non_blocking_write_buff_ptr++;
                EFWSM.State_Next = EFWWrite;
            } else {
                EFWSM.State_Next = EFWEndOk;
            }
            break;

        case EFWEndOk:
            non_blocking_write_buff_start_flag = false;
            EFWSM.State_Next = EFWWaitStart;
            retVal = EXT_FLASH_BUF_WRITE_DONE;
        break;

        case EFWEndError:
            non_blocking_write_buff_start_flag = false;
            EFWSM.State_Next = EFWWaitStart;
            retVal = EXT_FLASH_BUF_WRITE_TIME_OUT;
        break;

    }
    if( EFWSM.State_Current != EFWSM.State_Next ) {
        Serial_debug(DEBUG_INFO, &cli_serial, "EFWSM.State_Current[%d] -> EFWSM.State_Next[%d]\r\n", EFWSM.State_Current, EFWSM.State_Next );
    }
    EFWSM.State_Current = EFWSM.State_Next;
    return retVal;


}

/**
 * ext_flash_chip_erase - Erase entire Flash memory space
 */
void ext_flash_chip_erase(void)
{
    ext_command_flash_chip_erase();
    while ( !ext_flash_ready() ) {
        // Wait for Flash to complete command
    }
}

/**
 * ext_flash_erase_sector - Erase addressed Flash sector
 */
void ext_flash_erase_sector_by_descriptor(ext_flash_desc_t *sector_desc)
{

    uint32_t sector_offset;

    sector_offset = sector_desc->addr - EXT_FLASH_START_ADDRESS_CS3;

    set_a19(0);

    FLASH_SEQ(0x555, 0xAA);
    FLASH_SEQ(0x2AA, 0x55);
    FLASH_SEQ(0x555, 0x80);
    FLASH_SEQ(0x555, 0xAA);
    FLASH_SEQ(0x2AA, 0x55);
    FLASH_SEQ(sector_offset, 0x30);

    DEVICE_DELAY_US(EXT_FLASH_BUSY_DELAY); // Wait for Ready/Busy# signal to become valid

    while (GPIO_readPin(EXT_FLASH_READY) == 0) {
        // Serial_printf( &cli_serial, "Waiting GPIO_readPin(EXT_FLASH_READY)=[%d]\r\n",GPIO_readPin(EXT_FLASH_READY) );
        // Wait for Flash to complete command
        DEVICE_DELAY_US( EXT_FLASH_BUSY_DELAY * 1000 );
    }
}


void ext_flash_erase_sector(uint32_t sa)
{
    uint32_t sector_offset = sa & 0xf8000;

    set_a19(0);

    FLASH_SEQ(0x555, 0xAA);
    FLASH_SEQ(0x2AA, 0x55);
    FLASH_SEQ(0x555, 0x80);
    FLASH_SEQ(0x555, 0xAA);
    FLASH_SEQ(0x2AA, 0x55);
    FLASH_SEQ(sector_offset, 0x30);

    DEVICE_DELAY_US(EXT_FLASH_BUSY_DELAY); // Wait for Ready/Busy# signal to become valid

    while (GPIO_readPin(EXT_FLASH_READY) == 0) {
        // Wait for Flash to complete command
    }
}

/**
 * ext_flash_reset - RESET the external flash
 */
void ext_flash_reset(void)
{
    set_a19(0);
    GPIO_writePin(EXT_FLASH_RESET, 0);              // Assert FLASH RESET/
    DEVICE_DELAY_US(EXT_FLASH_RESET_DELAY);         // keep RESET active for a while
    GPIO_writePin(EXT_FLASH_RESET, 1);              // Deassert FLASH RESET/
}

static void ext_flash_pinmux_setup(void)
{
    // Selecting control pins.
    GPIO_setPinConfig(GPIO_34_EMIF1_CS2N);
    GPIO_setPinConfig(GPIO_35_EMIF1_CS3N);
    GPIO_setPinConfig(GPIO_31_EMIF1_WEN);
    GPIO_setPinConfig(GPIO_37_EMIF1_OEN);

    // GPIO 36 is connected to external flash RDY/BSY output pin.
    GPIO_setPinConfig(GPIO_36_GPIO36);
    GPIO_setPadConfig(EXT_FLASH_READY, GPIO_PIN_TYPE_PULLUP);
    GPIO_setDirectionMode(EXT_FLASH_READY, GPIO_DIR_MODE_IN);

    // GPIO 33 is connected to external flash RESET input pin.
    GPIO_setPinConfig(GPIO_33_GPIO33);
    GPIO_writePin(EXT_FLASH_RESET, 0);              // Assert FLASH RESET/
    GPIO_setDirectionMode(EXT_FLASH_RESET, GPIO_DIR_MODE_OUT);

    // Selecting 20 address lines.
    GPIO_setPinConfig(GPIO_92_EMIF1_BA1);
    GPIO_setPinConfig(GPIO_38_EMIF1_A0);
    GPIO_setPinConfig(GPIO_39_EMIF1_A1);
    GPIO_setPinConfig(GPIO_40_EMIF1_A2);
    GPIO_setPinConfig(GPIO_41_EMIF1_A3);
    GPIO_setPinConfig(GPIO_44_EMIF1_A4);
    GPIO_setPinConfig(GPIO_45_EMIF1_A5);
    GPIO_setPinConfig(GPIO_46_EMIF1_A6);
    GPIO_setPinConfig(GPIO_47_EMIF1_A7);
    GPIO_setPinConfig(GPIO_48_EMIF1_A8);
    GPIO_setPinConfig(GPIO_49_EMIF1_A9);
    GPIO_setPinConfig(GPIO_50_EMIF1_A10);
    GPIO_setPinConfig(GPIO_51_EMIF1_A11);
    GPIO_setPinConfig(GPIO_52_EMIF1_A12);
    GPIO_setPinConfig(GPIO_86_EMIF1_A13);
    GPIO_setPinConfig(GPIO_87_EMIF1_A14);
    GPIO_setPinConfig(GPIO_88_EMIF1_A15);
    GPIO_setPinConfig(GPIO_89_EMIF1_A16);
    GPIO_setPinConfig(GPIO_90_EMIF1_A17);
//    GPIO_setPinConfig(GPIO_91_EMIF1_A18);

    // GPIO 91 is connected to external flash A19 input pin.
    GPIO_setPinConfig(GPIO_91_GPIO91);
    GPIO_writePin(EXT_FLASH_A19, 0);
    GPIO_setDirectionMode(EXT_FLASH_A19, GPIO_DIR_MODE_OUT);

    // Selecting 16 data lines.
    GPIO_setPinConfig(GPIO_69_EMIF1_D15);
    GPIO_setPinConfig(GPIO_70_EMIF1_D14);
    GPIO_setPinConfig(GPIO_71_EMIF1_D13);
    GPIO_setPinConfig(GPIO_72_EMIF1_D12);
    GPIO_setPinConfig(GPIO_73_EMIF1_D11);
    GPIO_setPinConfig(GPIO_74_EMIF1_D10);
    GPIO_setPinConfig(GPIO_75_EMIF1_D9);
    GPIO_setPinConfig(GPIO_76_EMIF1_D8);
    GPIO_setPinConfig(GPIO_77_EMIF1_D7);
    GPIO_setPinConfig(GPIO_78_EMIF1_D6);
    GPIO_setPinConfig(GPIO_79_EMIF1_D5);
    GPIO_setPinConfig(GPIO_80_EMIF1_D4);
    GPIO_setPinConfig(GPIO_81_EMIF1_D3);
    GPIO_setPinConfig(GPIO_82_EMIF1_D2);
    GPIO_setPinConfig(GPIO_83_EMIF1_D1);
    GPIO_setPinConfig(GPIO_85_EMIF1_D0);

    // Setup async mode and enable pull-ups for Data pins.
    for (int i = 69; i <= 85; i++) {
        if (i != 84) {
            GPIO_setPadConfig(i, GPIO_PIN_TYPE_PULLUP);
            GPIO_setQualificationMode(i, GPIO_QUAL_ASYNC);
        }
    }

    // Deassert external flash RESET.
    GPIO_writePin(EXT_FLASH_RESET, 1);
}

/**
 * ext_flash_config - configure EMIF1 registers for accessing external flash on CS3
 */
void ext_flash_config(void)
{
    EMIF_AsyncTimingParams tparam;

    //
    // Configure to run EMIF1 on full Rate. (EMIF1CLK = CPU1SYSCLK)
    //
    SysCtl_setEMIF1ClockDivider(SYSCTL_EMIF1CLK_DIV_1);

    //
    // Grab EMIF1 For CPU1.
    //
    while (HWREGH(EMIF1CONFIG_BASE + MEMCFG_O_EMIF1MSEL) != EMIF_MASTER_CPU1_G) {
        EMIF_selectMaster(EMIF1CONFIG_BASE, EMIF_MASTER_CPU1_G);
    }

    //
    // Disable Access Protection. (CPU_FETCH/CPU_WR/DMA_WR)
    //
    EMIF_setAccessProtection(EMIF1CONFIG_BASE, 0x0);

    //
    // Commit the configuration related to protection. Till this bit remains
    // set, contents of EMIF1ACCPROT0 register can't be changed.
    //
    EMIF_commitAccessConfig(EMIF1CONFIG_BASE);

    //
    // Lock the configuration so that EMIF1COMMIT register can't be changed
    // any more.
    //
    EMIF_lockAccessConfig(EMIF1CONFIG_BASE);

    //
    // Configure GPIO pins for EMIF1.
    //
    ext_flash_pinmux_setup();

    //
    // Configures Normal Asynchronous Mode of Operation.
    //
    EMIF_setAsyncMode(EMIF1_BASE, EMIF_ASYNC_CS3_OFFSET, EMIF_ASYNC_NORMAL_MODE);

    //
    // Disables Extended Wait Mode.
    //
    EMIF_disableAsyncExtendedWait(EMIF1_BASE, EMIF_ASYNC_CS3_OFFSET);

    //
    // Configure EMIF1 Data Bus Width.
    //
    EMIF_setAsyncDataBusWidth(EMIF1_BASE, EMIF_ASYNC_CS3_OFFSET,
                              EMIF_ASYNC_DATA_WIDTH_16);

    //
    // Configure the access timing for CS3 space.
    //
    tparam.rSetup = 3;
    tparam.rStrobe = 7;
    tparam.rHold = 0;
    tparam.turnArnd = 2;
    tparam.wSetup = 0;
    tparam.wStrobe = 7;
    tparam.wHold = 0;
    EMIF_setAsyncTimingParams(EMIF1_BASE, EMIF_ASYNC_CS3_OFFSET, &tparam);
}

/**
 * ext_flash_test - test external flash API.
 */
void ext_flash_test(void)
{
    //
    // Test interface with flash by reading CFI data
    //
    enter_CFI();

    if ((g_ext_flash_data[16] != 0x51) || (g_ext_flash_data[17] != 0x52) || (g_ext_flash_data[18] != 0x59)) {
        Serial_printf(&cli_serial, "Unknown flash device: [%04x] [%04x] [%04x]\r\n", g_ext_flash_data[16], g_ext_flash_data[17], g_ext_flash_data[18]);
        exit_CFI();
        return;
    }

    exit_CFI();

    Serial_printf(&cli_serial, "External flash identified\r\n");

    //
    // Erase flash and verify buffer contents
    //
    Serial_printf(&cli_serial, "Erasing flash ...\r\n");
    ext_flash_erase_sector(0x8000);

    //
    // Verify that flash was erased by checking that first number of words equals 0xFFFF.
    //
    Serial_printf(&cli_serial, "Verifying flash erased ...\r\n");
    for (uint16_t word = 0; word < BUFFER_WORDS; word++) {
        if (g_ext_flash_data[(uint32_t)word + 0x8000U] != 0xFFFF) {
            Serial_printf(&cli_serial, "Flash erase failed!\r\n");
            return;
        }
    }

    //
    // Program dataBuffer contents
    //
    Serial_printf(&cli_serial, "Writing test data to flash ...\r\n");
    for (uint16_t word = 0; word < BUFFER_WORDS; word++) {
        uint32_t wraddr = (uint32_t) &g_ext_flash_data[0x8000 + word];
        if (word == 0) {
            Serial_printf(&cli_serial, "Write starts at 0x%lx\r\n", wraddr);
        }
//        ext_flash_write_word((uint32_t) &g_ext_flash_data[0x8000 + word], word);
        ext_flash_write_word(wraddr, word);
    }

    ext_flash_write_word((uint32_t) &g_ext_flash_data[0x0007ff00], 0x1234);

    //
    // Verify dataBuffer contents
    //
    Serial_printf(&cli_serial, "Verifying test data in flash ...\r\n");
    for (uint16_t word = 0; word < BUFFER_WORDS; word++) {
        if (g_ext_flash_data[(uint32_t)word + 0x8000U] != word) {
            Serial_printf(&cli_serial, "Verification failed!\r\n");
            return;
        }
    }

    Serial_printf(&cli_serial, "TEST PASS\r\n");
}
