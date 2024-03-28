#include <device.h>
#include <flash_api.h>

#include "debug.h"
#include "string.h"

#ifdef __TI_COMPILER_VERSION__
    #if __TI_COMPILER_VERSION__ >= 15009000
        #define ramFuncSection ".TI.ramfunc"
    #else
        #define ramFuncSection "ramfuncs"
    #endif
#endif


#define FLASH_APPL_START(x)     (0x084000)
#define FLASH_APPL_END(x)       ((FLASH_APPL_START((x))) + ((CFG_MAX_DOMAINSIZE((x)) / 2) - 1))

#define CFG_MAX_DOMAINSIZE(x) (2 * ((256 * 1024ul)))

/* number of Byte that are flashed at the same time (16 bytes -> 8 words) */
#define FLASH_ONE_CALL_SIZE 16u

#define DECRYPT_MEMORY(addr, pdest, psrc, size) inline_decrypt_memory(addr, pdest, psrc, size)
static inline void inline_decrypt_memory(
        uint32_t addr,
        uint8_t * pdest,
        const uint8_t * psrc,
        size_t size
    )
{
    (void)addr;
    memcpy(pdest, psrc, size);
}


#pragma CODE_SECTION(flash_api_init,ramFuncSection);
FlashRet_t flash_api_init(){
    Fapi_StatusType fret;
    FlashRet_t ret = FLASH_RET_OK;

//    InitFlash();
    Flash_initModule(FLASH0CTRL_BASE, FLASH0ECC_BASE, 3);
#ifdef CPU1
    Flash_claimPumpSemaphore(FLASH_CPU1_WRAPPER);
#elif defined(CPU2)
    Flash_claimPumpSemaphore(FLASH_CPU2_WRAPPER);
#endif



    /* initialize flash API */
    EALLOW;
    fret = Fapi_initializeAPI((Fapi_FmcRegistersType *)F021_CPU0_REGISTER_ADDRESS, 200);
    if (fret != Fapi_Status_Success) {
        ret = FLASH_RET_ERROR;
    }

    fret = Fapi_setActiveFlashBank(Fapi_FlashBank0);
    if (fret != Fapi_Status_Success) {
        ret = FLASH_RET_ERROR;
    }

//    SeizeFlashPump();
    EDIS;

    return ret;
}


static FlashRet_t flash_api_checkAddress(uint32_t address){

    if (address > FLASH_APPL_END(address)){
        return(FLASH_RET_ERROR);
    }

    if (address < FLASH_APPL_START(address))  {
        return(FLASH_RET_ERROR);
    }

    return(FLASH_RET_OK);

}

#pragma CODE_SECTION(flash_api_write,ramFuncSection);
FlashRet_t flash_api_write(uint32_t address, const uint16_t* pSrc){

    FlashRet_t ret;
    Fapi_StatusType fret;
    uint8_t dest[FLASH_ONE_CALL_SIZE];

    ret = flash_api_checkAddress(address);
    if (ret != FLASH_RET_OK)  {
        return ret;
    }

    DECRYPT_MEMORY(address,  &dest[0], (const uint8_t*) pSrc, FLASH_ONE_CALL_SIZE);

    EALLOW;
    fret = Fapi_issueProgrammingCommand( (uint32 *) address, (uint16 *) dest, FLASH_ONE_CALL_SIZE / 2,
                                         NULL, 0,  Fapi_AutoEccGeneration);

    // Wait until the Flash program operation is over
    while (Fapi_checkFsmForReady() != Fapi_Status_FsmReady){}

    if (fret != Fapi_Status_Success) {
        return FLASH_RET_FLASH_ERROR;
    }
    EDIS;
    return FLASH_RET_OK;

}

/* Erase One Page */
#pragma CODE_SECTION(flash_api_erase,ramFuncSection);
FlashRet_t flash_api_erase(uint32_t address){
    FlashRet_t status;
    Fapi_StatusType fret;

    flash_api_init();

    status = flash_api_checkAddress(address);

    if (status!=FLASH_RET_OK){
        return status;
    }

    EALLOW;
    fret = Fapi_issueAsyncCommandWithAddress(Fapi_EraseSector,
                   (uint32 *)address);

    // Wait until FSM is done with erase sector operation.
    while (Fapi_checkFsmForReady() != Fapi_Status_FsmReady){}
    EDIS;


    if(fret != Fapi_Status_Success){
        return FLASH_RET_FLASH_ERROR;
    }

    return FLASH_RET_OK;
}

#if 1
#pragma CODE_SECTION(flash_api_read,ramFuncSection);
FlashRet_t flash_api_read(uint32_t address, uint16_t* readBuffer, uint32_t len){
//    FlashRet_t status;
//    Fapi_StatusType fret;
//
//    status = flash_api_checkAddress(address);
//
//    if (status!=FLASH_RET_OK){
//        return status;
//    }
//
    EALLOW;
    memcpy (readBuffer, (uint32_t*)address, len);

//    while (Fapi_checkFsmForReady() != Fapi_Status_FsmReady){
//
//    }
    EDIS;

//    if(fret != Fapi_Status_Success){
//        return FLASH_RET_FLASH_ERROR;
//    }

    return FLASH_RET_OK;
}
#endif

void flash_api_test(){
    uint16_t dataBufferWrite[8] = {0x99, 0x98, 0x97, 0x96, 0x95, 0x94, 0x93, 0x92};

    flash_api_init();

    flash_api_write (0x0A8000, dataBufferWrite);


    memset(dataBufferWrite, 0, sizeof (dataBufferWrite));
    flash_api_read(0x80000, dataBufferWrite, 8);


}

