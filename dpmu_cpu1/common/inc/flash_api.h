#ifndef COAPPL_FLASH_API_H_
#define COAPPL_FLASH_API_H_

#include "F021_F2838x_C28x.h"
#include "flash_programming_f2838x_c28x.h"
typedef enum {
    FLASH_RET_OK=0,  /**< OK */
    FLASH_RET_CAN_BUSY, /**< CAN is busy, e.g. cannot transmit */

    FLASH_RET_BUSY, /**< functionality is busy/already running */
    FLASH_RET_NOTHING, /**< nothing is required to do */

    FLASH_RET_CRC_WRONG, /**< wrong CRC sum */

    FLASH_RET_FLASH_END, /**< action was stopped */
    FLASH_RET_FLASH_ERROR, /**< general error */

    FLASH_RET_CALLBACK_READY, /**< callback has answer sent */

    FLASH_RET_USB_WRONG_FORMAT, /**< format error */
    FLASH_RET_IMAGE_WRONG,/* flash image has incorrect parameter */
    FLASH_RET_ERROR /**< general error */
} FlashRet_t;


typedef enum {

    FLASH_USERSTATE_OK, /**< OK, nothing to do */
    FLASH_USERSTATE_RUNNING, /**< doing anything (erase, program) */
    FLASH_USERSTATE_ERROR /**< error during flash/erase */
} FlashUserState_t;


/** \brief Flash driver states */
typedef enum {

    FLASH_STATE_INIT, /**< ready for commands */
    FLASH_STATE_WAIT, /**< wait for new flash pages */
    FLASH_STATE_ERASE, /**< erase a page */
    FLASH_STATE_FLASH, /**< flash a page */
    FLASH_STATE_ERROR, /**< error during flash/erase */
    FLASH_STATE_END     /**< last page flashed */
} FlashState_t;


FlashRet_t flash_api_init();
FlashRet_t flash_api_erase();
FlashRet_t flash_api_write(uint32_t address,const uint16_t* pSrc);
FlashRet_t flash_api_read(uint32_t address, uint16_t* readBuffer, uint32_t len);

void flash_api_test();


#endif /* COAPPL_FLASH_API_H_ */

