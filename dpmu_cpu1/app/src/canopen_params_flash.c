/*
 * canopen_params_flash.c
 *
 *  Created on: 10 nov. 2023
 *      Author: us
 */

#include <stdint.h>
#include <stdbool.h>

#include "canopen_params_flash.h"
#include "ext_flash.h"
#include "main.h"
#include "serial.h"

/**
 * Write parameters to external flash.
 */
bool write_params_to_ext_flash(const save_od_t *params, uint16_t count)
{
    const ext_flash_desc_t *sector_info = &ex_flash_info[EXT_FLASH_SA2];

    save_od_info_t od_info;

    od_info.magic = OD_MAGIC_NUM;
    od_info.count = count;
    od_info.reserved = 0xFFFF;

    Serial_debug(DEBUG_ERROR, &cli_serial, "%s:\r\n", __FUNCTION__);

    // Q&D implementation here! Erasing Sector SA2 of external flash.
    Serial_debug(DEBUG_ERROR, &cli_serial, "  Erasing sector EXT_FLASH_SA2\r\n");
    ext_flash_erase_sector(sector_info->addr);

    uint16_t *next_addr = (uint16_t *)(EXT_FLASH_START_ADDRESS_CS3 + sector_info->addr);

    // Start by writing the magic number and object count.
    Serial_debug(DEBUG_ERROR, &cli_serial, "  Writing magic number: 0x%lX and object count: %u\r\n", od_info.magic, od_info.count);

    ext_flash_write_buf((uint32_t)next_addr, (uint16_t *)&od_info, sizeof(save_od_info_t));
    next_addr += sizeof(save_od_info_t);

    // Write parameters to external flash sector SA2.
    Serial_debug(DEBUG_ERROR, &cli_serial, "  Writing the objects\r\n");
    ext_flash_write_buf((uint32_t)next_addr, (uint16_t *)params, count * sizeof(save_od_t));

    return true;
}

/**
 * Reads parameters from external flash.
 *
 * Checks for a valid magic number and returns 0 if not found.
 *
 * @param   params  pointer do destination buffer
 *
 * @retval  number of parameters read, or 0 if none
 */
int read_params_from_ext_flash(save_od_t *params, const od_range_t *excluded)
{
    const ext_flash_desc_t *sector_info = &ex_flash_info[EXT_FLASH_SA2];

    save_od_info_t od_info;
    uint16_t count = 0;

    Serial_debug(DEBUG_ERROR, &cli_serial, "%s:\r\n", __FUNCTION__);

    uint16_t *next_addr = (uint16_t *)(EXT_FLASH_START_ADDRESS_CS3 + sector_info->addr);

    // Start by reading the magic number and object count.
    Serial_debug(DEBUG_ERROR, &cli_serial, "  Reading magic number & object count\r\n");

    ext_flash_read_buf((uint32_t)next_addr, (uint16_t *)&od_info, sizeof(save_od_info_t));
    next_addr += sizeof(save_od_info_t);

    do {
        // Check for valid magic number.
        if (od_info.magic != OD_MAGIC_NUM) {
            Serial_debug(DEBUG_ERROR, &cli_serial, "    Unexpected magic number!\r\n");
            count = 0;
            break;
        }

        // Check that count > 0.
        if (od_info.count == 0) {
            Serial_debug(DEBUG_ERROR, &cli_serial, "    Count == 0!\r\n");
            count = 0;
            break;
        }

        // If no exclusion range passed then read all objects.
        if (excluded == NULL) {
            Serial_debug(DEBUG_ERROR, &cli_serial, "    Reading all %u objects\r\n", od_info.count);
            ext_flash_read_buf((uint32_t)next_addr, (uint16_t *)params, od_info.count * sizeof(save_od_t));
            count = od_info.count;
            break;
        }

        // Read all objects but skip those within exclusion range.
        Serial_debug(DEBUG_ERROR, &cli_serial, "    Reading all %u objects, except those in range 0x%04x to 0x%04x\r\n", od_info.count, excluded->begin, excluded->end);
        count = 0;
        for (uint16_t i = 0; i < od_info.count; ++i) {
            save_od_t od;
            ext_flash_read_buf((uint32_t)next_addr, (uint16_t *)&od, sizeof(save_od_t));
            if (od.index < excluded->begin || od.index > excluded->end) {
                *params++ = od;
                count++;
            }
            next_addr += sizeof(save_od_t);
        }
    } while (0);

    Serial_debug(DEBUG_ERROR, &cli_serial, "    Read %u objects\r\n", count);

    return count;
}
