/**
 * @file    lfs_api.h
 *
 *  Created on: 4 jan. 2023
 *      Author: vb
 */

#ifndef APP_LITTLEFS_INC_LFS_API_H_
#define APP_LITTLEFS_INC_LFS_API_H_

#include "lfs.h"

/**
 * The following two variables are defined in lfs_api.c and are used by
 * lfs_api_mount_filesystem, and lfs_unmount_filesystem.
 */
extern struct lfs_config ext_flash_lfs_config;
extern lfs_t ext_flash_lfs;

/**
 * Read a region in a block. Negative error codes are propagated to the user.
 */
int lfs_api_block_device_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);

/**
 * Program a region in a block. The block must have previously
 * been erased. Negative error codes are propagated to the user.
 * May return LFS_ERR_CORRUPT if the block should be considered bad.
 */
int lfs_api_block_device_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer,
        lfs_size_t size);

/**
 * Erase a block. A block must be erased before being programmed.
 * The state of an erased block is undefined. Negative error codes
 * are propagated to the user.
 * May return LFS_ERR_CORRUPT if the block should be considered bad.
 */
int lfs_api_block_device_erase(const struct lfs_config *c, lfs_block_t block);

/**
 * Sync the state of the underlying block device. Negative error codes
 * are propagated to the user. Does nothing in this project. Always returns LFS_ERR_OK.
 */
int lfs_api_block_device_sync(const struct lfs_config *c);

/* Test functions. */
void lfs_api_file_test_001(void);
void lfs_api_file_test_002(void);
void lfs_api_file_system_test(void);

/**
 * Just format the filesystem.
 */
int lfs_api_format_filesystem(void);

/**
 * Try to mount the filesystem. Format and retry on failure.
 */
int lfs_api_mount_filesystem(void);

/**
 * Just unmount the filesystem.
 */
int lfs_api_unmount_filesystem(void);

/**
 * List the directory contents. Like the "ls" command in Linux/Unix.
 */
int lfs_api_show_directory_contents(const char *dir_name);

#endif /* APP_LITTLEFS_INC_LFS_API_H_ */
