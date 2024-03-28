#include <assert.h>

#include "driverlib.h"
#include "device.h"
#include "lfs_api.h"
#include "ext_flash.h"
#include "serial.h"

// #pragma LOCATION( emif_flash_region, 0x00300000U)
//char emif_flash_region[4096];
//

/*Word Mode (x16)*/
#define SECTOR_OFFSET                      0x8000          // 64 kB (32 k words)
#define EMIF1_CS0N_START_ADD               0x80000000U
#define EMIF1_CS0N_END_ADD                 0x8FFFFFFFU
#define EMIF1_CS2N_START_ADD               0x00200000U
#define EMIF1_CS2N_END_ADD                 0x002FFFFFU
#define EMIF1_CS3N_START_ADD               0x00300000U
#define EMIF1_CS3N_END_ADD                 0x0037FFFFU
#define EMIF1_CS4N_START_ADD               0x00380000U
#define EMIF1_CS4N_END_ADD                 0x003DFFFFU

#define LFS_MIN_READ_SIZE   16                  // min read size
#define LFS_MIN_PROG_SIZE   16                  // min write size
#define LFS_BLOCK_SIZE      0x8000              // size of erasable block in bytes
#define LFS_BLOCK_COUNT     31                  // number of erasable blocks
#define LFS_CACHE_SIZE      16                  // size of block cache in bytes
#define LFS_LOOKAHEAD_SIZE  32                  // size of lookahead buffer in bytes
#define LFS_BLOCK_CYCLES    100

/**
 * Local data.
 */
static uint16_t l_read_buffer[LFS_CACHE_SIZE];
static uint16_t l_prog_buffer[LFS_CACHE_SIZE];

#pragma DATA_ALIGN(l_lookahead_buffer, 4)
static uint16_t l_lookahead_buffer[LFS_LOOKAHEAD_SIZE];

static lfs_file_t file;

/**
 * Global data.
 */
lfs_t ext_flash_lfs;

struct lfs_config ext_flash_lfs_config = {
        // block device operations
        //        .context = &str;
        .read = lfs_api_block_device_read,      // reads a region in a block
        .prog = lfs_api_block_device_prog,      // programs a region in a block
        .erase = lfs_api_block_device_erase,    // erases a block
        .sync = lfs_api_block_device_sync,      // syncs the state of a block device

        .read_size = LFS_MIN_READ_SIZE,         // min read size
        .prog_size = LFS_MIN_PROG_SIZE,         // min write size
        .block_size = LFS_BLOCK_SIZE,           // size of erasable block in bytes
        .block_count = LFS_BLOCK_COUNT,         // number of erasable blocks
        .block_cycles = LFS_BLOCK_CYCLES,
        .cache_size = LFS_CACHE_SIZE,           // size of block cache in bytes
        .lookahead_size = LFS_LOOKAHEAD_SIZE,   // size of lookahead buffer in bytes

        .read_buffer = l_read_buffer,           // statically allocated read buffer
        .prog_buffer = l_prog_buffer,           // statically allocated prog buffer
        .lookahead_buffer = l_lookahead_buffer, // statically allocated lookahead buffer

        .name_max = 100
};

extern struct Serial cli_serial;

int lfs_api_block_device_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    uint32_t address = EMIF1_CS3N_START_ADD + SECTOR_OFFSET + c->block_size * block + off;

    uint16_t *dst = buffer;

    for (uint32_t i = 0; i < size; ++i) {
        uint16_t data = ext_flash_read_word(address + i);
        *dst++ = data;
    }

    return LFS_ERR_OK;
}

int lfs_api_block_device_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    uint32_t address = EMIF1_CS3N_START_ADD + SECTOR_OFFSET + c->block_size * block + off;

    const uint16_t *src = buffer;

    for (uint32_t i = 0; i < size; ++i) {
        uint16_t data;
        data = *src++;
        ext_flash_write_word(address + i, data);
    }

    return LFS_ERR_OK;
}

int lfs_api_block_device_erase(const struct lfs_config *c, lfs_block_t block)
{
    uint32_t address = EMIF1_CS3N_START_ADD + SECTOR_OFFSET + c->block_size * block;

    ext_flash_erase_sector(address);

    return LFS_ERR_OK;
}

int lfs_api_block_device_sync(const struct lfs_config *c)
{
    return LFS_ERR_OK;
}

int lfs_api_format_filesystem(void)
{
    return lfs_format(&ext_flash_lfs, &ext_flash_lfs_config);
}

int lfs_api_mount_filesystem(void)
{
    int err = lfs_mount(&ext_flash_lfs, &ext_flash_lfs_config);

    // Reformat if we can't mount the fs.
    // This should be only happen on the first boot.

    if (err) {
        lfs_format(&ext_flash_lfs, &ext_flash_lfs_config);
        err = lfs_mount(&ext_flash_lfs, &ext_flash_lfs_config);
    }

    return err;
}

int lfs_api_unmount_filesystem(void)
{
    int err;

    // Release any resources we were using.
    err = lfs_unmount(&ext_flash_lfs);

    return err;
}

void lfs_api_file_test_001(void)
{

    uint32_t test_count = 0;

    int err = lfs_api_mount_filesystem();

    if (err) {
        // Give up on error.
        Serial_printf(&cli_serial, "Failed to mount file system\r\n");
        return;
    }

    // Read current count.
    uint32_t boot_count = 0;
    lfs_file_open(&ext_flash_lfs, &file, "boot_count", LFS_O_CREAT | LFS_O_RDWR);
    lfs_file_read(&ext_flash_lfs, &file, &boot_count, sizeof(boot_count));

    // Update boot count.
    boot_count += 10;
    test_count = boot_count + 15;
    lfs_file_rewind(&ext_flash_lfs, &file);
    lfs_file_write(&ext_flash_lfs, &file, &test_count, sizeof(boot_count));

    // Remember the storage is not updated until the file is closed successfully.
    lfs_file_close(&ext_flash_lfs, &file);

    boot_count = 0;
    lfs_file_open(&ext_flash_lfs, &file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
    lfs_file_read(&ext_flash_lfs, &file, &boot_count, sizeof(boot_count));

    lfs_file_write(&ext_flash_lfs, &file, &boot_count, sizeof(boot_count));

    // Remember the storage is not updated until the file is closed successfully.
    lfs_file_close(&ext_flash_lfs, &file);

    // Release any resources we were using.
    lfs_api_unmount_filesystem();

    Serial_printf(&cli_serial, "Done.\r\n");
}

void lfs_api_file_test_002(void)
{
    unsigned char log_data_write_001[] = "This is test log data";
    unsigned char log_data_write_002[] = "which is being updated now";
    unsigned char log_data_read[50];
    int err;

    err = lfs_api_mount_filesystem();
    if (err < 0) {
        Serial_printf(&cli_serial, "Failed to mount file system\r\n");
        return;
    }

    do {
        err = lfs_file_open(&ext_flash_lfs, &file, "log_xxxx", LFS_O_RDWR | LFS_O_CREAT);
        if (err < 0) {
            Serial_printf(&cli_serial, "Failed to open file 'log_xxxx'\r\n");
            break;
        }

        err = lfs_file_write(&ext_flash_lfs, &file, log_data_write_001, sizeof(log_data_write_001));
        if (err < 0) {
            err = lfs_file_close(&ext_flash_lfs, &file);
            Serial_printf(&cli_serial, "Failed to write to file 'log_xxxx'\r\n");
            break;
        } else {
            Serial_printf(&cli_serial, "Bytes written into the file: %d\r\n", err);
        }

        // Sync file to update data.
        err = lfs_file_sync(&ext_flash_lfs, &file);

        err = lfs_file_write(&ext_flash_lfs, &file, log_data_write_002, sizeof(log_data_write_002));
        if (err < 0) {
            err = lfs_file_close(&ext_flash_lfs, &file);
            Serial_printf(&cli_serial, "Failed to write to file 'log_xxxx'\r\n");
            break;
        } else {
            Serial_printf(&cli_serial, "Bytes written into the file: %d\r\n", err);
        }

        // Close the file.
        err = lfs_file_close(&ext_flash_lfs, &file);
        if (err < 0) {
            Serial_printf(&cli_serial, "Failed to close the file\r\n");
            break;
        }

        // Open the file again.
        err = lfs_file_open(&ext_flash_lfs, &file, "log_xxxx", LFS_O_RDWR | LFS_O_CREAT);
        if (err < 0) {
            Serial_printf(&cli_serial, "Failed to open file 'log_xxxx' again\r\n");
            break;
        }

        /* Read the file data*/
        err = lfs_file_read(&ext_flash_lfs, &file, log_data_read, sizeof(log_data_read));
        if (err < 0) {
            err = lfs_file_close(&ext_flash_lfs, &file);
            Serial_printf(&cli_serial, "Failed to read from file 'log_xxxx'\r\n");
            break;
        } else {
            Serial_printf(&cli_serial, "Bytes read from file: %d\r\n", err);
        }

        /* Get the file size*/
        lfs_soff_t file_size = lfs_file_size(&ext_flash_lfs, &file);
        if (file_size < 0) {
            Serial_printf(&cli_serial, "Failed to get the file size\r\n");
            break;
        } else {
            Serial_printf(&cli_serial, "File size: %ld\r\n", file_size);
        }

        err = lfs_file_close(&ext_flash_lfs, &file);
    } while (0);

    // Release any resources we were using.
    lfs_api_unmount_filesystem();
}

void lfs_api_file_test_003(void)
{
    unsigned char log_data_write_001[] = "This is test log data";
    unsigned char log_data_write_002[] = "which is being updated now";
    unsigned char log_data_read[50];

    lfs_dir_t dir;

    int err = lfs_api_mount_filesystem();

    err = lfs_mkdir(&ext_flash_lfs, "\root\files");

    err = lfs_dir_open(&ext_flash_lfs, &dir, "\root\files");

    err = lfs_file_open(&ext_flash_lfs, &file, "log_xxxx", LFS_O_RDWR | LFS_O_CREAT);
    if (err < 0) {
        ; // error
    }

    err = lfs_file_write(&ext_flash_lfs, &file, log_data_write_001, sizeof(log_data_write_001));
    if (err < 0) {
        ; //error
    } else {
        ; //DBG_PRINT("Bytes written into the file: %d\r\n", err);
    }

    // Sync file to update data
    err = lfs_file_sync(&ext_flash_lfs, &file);

    err = lfs_file_write(&ext_flash_lfs, &file, log_data_write_002, sizeof(log_data_write_002));
    if (err < 0) {
        ; //error
    } else {
        ; // DBG_PRINT("Bytes written into the file: %d\r\n", err);
    }

    /* Close the file*/
    err = lfs_file_close(&ext_flash_lfs, &file);

    /* Open the file again*/
    err = lfs_file_open(&ext_flash_lfs, &file, "log_xxxx", LFS_O_RDWR | LFS_O_CREAT);
    if (err < 0) {
        ; //error
    }

    /* Read the file data*/
    err = lfs_file_read(&ext_flash_lfs, &file, log_data_read, sizeof(log_data_read));
    if (err < 0) {
        ; //error
    } else {
        ;
    }

    /* Get the file size*/
    lfs_soff_t file_size = lfs_file_size(&ext_flash_lfs, &file);
    if (file_size < 0) {
        ; //error
    } else {
        ; // DBG_PRINT("File size: %d\r\n", file_size);
    }

    err = lfs_file_close(&ext_flash_lfs, &file);

    err = lfs_dir_close(&ext_flash_lfs, &dir);

    // Release any resources we were using.
    lfs_api_unmount_filesystem();
}

int lfs_api_show_directory_contents(const char *dir_name)
{
    static lfs_dir_t dir;
    static struct lfs_info info;

    int err = lfs_dir_open(&ext_flash_lfs, &dir, dir_name);
    if (err) {
        return err;
    }

    while (true) {
        int res = lfs_dir_read(&ext_flash_lfs, &dir, &info);
        if (res < 0) {
            return res;
        }

        if (res == 0) {
            break;
        }

        if (info.type == LFS_TYPE_REG || info.type == LFS_TYPE_DIR) {
            Serial_printf(&cli_serial, "%c  %8ld  %s\r\n", info.type == LFS_TYPE_DIR ? 'D' : ' ', info.size, info.name);
        }
    }

    err = lfs_dir_close(&ext_flash_lfs, &dir);
    if (err) {
        return err;
    }

    return 0;
}

int lfs_api_create_file(const char *name)
{

    int err = lfs_file_open(&ext_flash_lfs, &file, name, LFS_O_RDWR | LFS_O_CREAT);
    if (err < 0) {
        ; // Error
    }
    return err;
}

void lfs_api_file_system_test(void)
{
    int status;

    status = lfs_api_mount_filesystem();

    if (status != 0) {
        Serial_printf(&cli_serial, "FAIL with error code %d\r\n", status);
    }

    Serial_printf(&cli_serial, "creating file ...\r\n");
    status = lfs_api_create_file("log_xxxx");

    if (status != 0) {
        Serial_printf(&cli_serial, "FAIL with error code %d\r\n", status);
    }

    Serial_printf(&cli_serial, "show dir ...\r\n");
    status = lfs_api_show_directory_contents("/");

    if (status != 0) {
        Serial_printf(&cli_serial, "FAIL with error code %d\r\n", status);
    }

    lfs_api_unmount_filesystem();
}
