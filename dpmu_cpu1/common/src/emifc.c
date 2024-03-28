/*
 * emif.c
 *
 *  Created on: 2 jan. 2023
 *      Author: vb
 */

#include "stdbool.h"
#include "stdio.h"

#include <emifc.h>
#include "board.h"
#include "driverlib.h"
#include "device.h"
#include "emif.h"
#include "emifc.h"
#include "serial.h"
#include "ext_flash.h"

static uint16_t configMaster = 0;

uint16_t errCountGlobalCPU1 = 0U;
uint32_t testStatusGlobalCPU1;
uint32_t i, iter;

#define MEM_RW_ITER    0x1U

volatile int dma_test_xfer_done;

void INT_CPU1_EXT_MEM_ISR(void)
{
    // Stop DMA channel.
    DMA_stopChannel(CPU1_EXT_MEM_BASE);

    // ACK to receive more interrupts from this PIE group.
    EALLOW;
    Interrupt_clearACKGroup(INT_CPU1_EXT_MEM_INTERRUPT_ACK_GROUP);
    EDIS;

    dma_test_xfer_done = 1; // Test done.

    return;
}

//bool emifc_execute(void)
//{
//    bool returnValue = false;
//    uint16_t errCountLocal;
//    testStatusGlobalCPU1 = 1;
//
//    //    emifc_pinmux_setup_memory(MEM_TYPE_RAM);
//
//    for(iter = 0; iter < MEM_RW_ITER; iter++){
//        // Grab EMIF1 for CPU1.
//        while(HWREGH(EMIF1CONFIG_BASE + MEMCFG_O_EMIF1MSEL) != EMIF_MASTER_CPU1_G){
//            EMIF_selectMaster(EMIF1CONFIG_BASE, EMIF_MASTER_CPU1_G);
//        }
//
//
//        // Check basic RD/WR access to CS2 space.
//        errCountLocal = emifc_write_flash_data(EXT_FLASH_START_ADDRESS_CS3, EXT_FLASH_SIZE_CS3);
//        emifc_read_flash_data(EXT_FLASH_START_ADDRESS_CS3, EXT_FLASH_SIZE_CS3);
//
//        errCountGlobalCPU1 = errCountGlobalCPU1 + errCountLocal;
//
//
//        // Release EMI1
//        EMIF_selectMaster(EMIF1CONFIG_BASE, EMIF_MASTER_CPU1_NG);
//    }
//
//
//    if(errCountGlobalCPU1 == 0x0U){
//        testStatusGlobalCPU1 = 0;
//        returnValue = true;
//        //        printf ("TEST PASS \r\n");
//    }
//    else{
//        testStatusGlobalCPU1 = 1;
//        returnValue = true;
//        //        printf ("TEST FAIL \r\n");
//    }
//
//    return returnValue;
//}

void emifc_set_cpun_as_master(EMIF1_Config* emif1)
{
    if (emif1->cpuType == CPU_TYPE_ONE && configMaster == 0){
        // Grab EMIF1 for CPU1.
        while(HWREGH(EMIF1CONFIG_BASE + MEMCFG_O_EMIF1MSEL) != EMIF_MASTER_CPU1_G){
            EMIF_selectMaster(EMIF1CONFIG_BASE, EMIF_MASTER_CPU1_G);
        }
    } else if (emif1->cpuType == CPU_TYPE_TWO && configMaster == 0){
        // Grab EMIF1 for CPU2.
        while(HWREGH(EMIF1CONFIG_BASE + MEMCFG_O_EMIF1MSEL) != EMIF_MASTER_CPU2_G){
            EMIF_selectMaster(EMIF1CONFIG_BASE, EMIF_MASTER_CPU2_G);
        }
    }
}

void emifc_realease_cpun_as_master(uint16_t cpuType)
{
    if (cpuType == CPU_TYPE_ONE){
        // Release EMIF1 for CPU1.
        EMIF_selectMaster(EMIF1CONFIG_BASE, EMIF_MASTER_CPU1_NG);
    }

    if (cpuType == CPU_TYPE_TWO){
        // Release EMIF1 for CPU2.
        EMIF_selectMaster(EMIF1CONFIG_BASE, EMIF_MASTER_CPU1_NG2);
    }
}

void emifc_cpu_write_memory(EMIF1_Config* emif1)
{
    uint16_t max_dma_transfer_size = 32;
    uint16_t number_of_tranfers = 1;
    uint16_t remaining_bytes_to_transfer, current_transfer_size;
    const void *src_address, *dest_address;
    /* set CPUn as master for memory */
    emifc_set_cpun_as_master(emif1);

    configMaster = 1;

//    /*** backup solution ***/
//    uint16_t* memPtr = (uint16_t *)emif1->address;
//
//    for(uint32_t i = 0U; i < emif1->size; i++) {
//        *(memPtr+i) = emif1->data[i];
//    }

//    /*** secondary solution - memcpy() ***/
//    memcpy((volatile uint32_t*)emif1->address, (volatile uint16_t*)emif1->data, emif1->size);


    number_of_tranfers = emif1->size / max_dma_transfer_size;

    remaining_bytes_to_transfer = emif1->size;
    src_address = (const void*)emif1->data;
    dest_address = (const void*)emif1->address;
    for( i=0; i <= number_of_tranfers; i++){
        if( remaining_bytes_to_transfer >= max_dma_transfer_size) {
            current_transfer_size = max_dma_transfer_size;
            remaining_bytes_to_transfer = remaining_bytes_to_transfer - max_dma_transfer_size;
        } else {
            current_transfer_size = remaining_bytes_to_transfer;
        }
        /*** preferred solution - DMA ***/
        dma_test_xfer_done = 0;

        /* configure the burst
         * base address for the DMA channel
         * size of the data to be transmitted
         * step size of address in the source address
         * step size of address in the destination address
         * */
        //DMA_configBurst(CPU1_EXT_MEM_BASE, emif1->size, 1, 1);
        DMA_configBurst(CPU1_EXT_MEM_BASE, current_transfer_size, 1, 1);
        DMA_configAddresses(CPU1_EXT_MEM_BASE, dest_address, src_address);
        DMA_startChannel(CPU1_EXT_MEM_BASE);
        DMA_forceTrigger(CPU1_EXT_MEM_BASE);

        /* Wait for DMA do finish */
        /* CONTROL register, bit TRANSFERSTS, '1' = transfer active */
        //  while (HWREGB(CPU1_EXT_MEM_BASE) & (1<<11)) //DmaRegs.CH2.CONTROL.bit.TRANSFERSTS
        while (!dma_test_xfer_done)
        {
            asm(" RPT #255 || NOP");
        }
        src_address = src_address + current_transfer_size;
        dest_address = dest_address + current_transfer_size;
    }
    // Release EMIF1
    EMIF_selectMaster(EMIF1CONFIG_BASE, EMIF_MASTER_CPU1_NG);
}

void emifc_cpu_read_memory(EMIF1_Config* emif1)
{
    uint16_t max_dma_transfer_size = 32;
    uint16_t number_of_tranfers = 1;
    uint16_t remaining_words_to_transfer, current_transfer_size;
    const void *src_address, *dest_address;
    emifc_set_cpun_as_master(emif1);

    configMaster = 1;

//    /*** backup solution ***/
//    uint16_t* memPtr = (uint16_t *)emif1->address;
//    for(i = 0U; i < emif1->size; i++){
//        emif1->data[i] = *(memPtr + i);
//    }

    /*** secondary solution - memcpy() ***/
    //memcpy((volatile uint16_t*)emif1->data, (volatile uint32_t*)emif1->address, emif1->size);


    number_of_tranfers = emif1->size / max_dma_transfer_size;
    remaining_words_to_transfer = emif1->size;
    dest_address = (const void*)emif1->data;
    src_address = (const void*)emif1->address;
    for( i=0; i <= number_of_tranfers; i++){
        if( remaining_words_to_transfer >= max_dma_transfer_size) {
            current_transfer_size = max_dma_transfer_size;
            remaining_words_to_transfer = remaining_words_to_transfer - max_dma_transfer_size;
        } else {
            current_transfer_size = remaining_words_to_transfer;
        }

        /*** preferred solution - DMA ***/
        dma_test_xfer_done = 0;

        /* configure the burst
         * base address for the DMA channel
         * size of the data to be transmitted
         * step size of address in the source address
         * step size of address in the destination address
         * */
        //DMA_configBurst(CPU1_EXT_MEM_BASE, emif1->size, 1, 1);
        DMA_configBurst(CPU1_EXT_MEM_BASE, current_transfer_size, 1, 1);
        DMA_configAddresses(CPU1_EXT_MEM_BASE, dest_address, src_address);
        DMA_startChannel(CPU1_EXT_MEM_BASE);
        DMA_forceTrigger(CPU1_EXT_MEM_BASE);

        /* Wait for DMA do finish */
        /* CONTROL register, bit TRANSFERSTS, '1' = transfer active */
        //    while (HWREGB(CPU1_EXT_MEM_BASE) & (1<<11)) //DmaRegs.CH2.CONTROL.bit.TRANSFERSTS
        while (!dma_test_xfer_done)
        {
            asm(" RPT #255 || NOP");
        }
        src_address = src_address + current_transfer_size;
        dest_address = dest_address + current_transfer_size;
    }
    // Release EMIF1
    EMIF_selectMaster(EMIF1CONFIG_BASE, EMIF_MASTER_CPU1_NG);
}

void emifc_pinConfiguration(){
    emifc_pinmux_setup_memory(MEM_TYPE_RAM);

}

void emifc_pinmux_setup_memory(uint16_t memType)
{
    uint16_t i;
    uint32_t chipSelect;

    //
    // Selecting control pins.
    //
    if (memType == MEM_TYPE_FLASH){
        chipSelect = GPIO_35_EMIF1_CS3N;
    } else if (memType == MEM_TYPE_RAM){
        chipSelect = GPIO_34_EMIF1_CS2N;
    }

    GPIO_setPinConfig(chipSelect);
    GPIO_setPinConfig(GPIO_31_EMIF1_WEN);
    GPIO_setPinConfig(GPIO_37_EMIF1_OEN);
    GPIO_setPinConfig(GPIO_36_EMIF1_WAIT);
    GPIO_setPinConfig(GPIO_33_EMIF1_RNW);

    //
    // Selecting 18 address lines.
    //
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

    //
    // Selecting 16 data lines.
    //
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

    //
    // Setup async mode and enable pull-ups for Data pins.
    //
    for(i=69; i<=85;i++)
    {
        if(i != 84)
        {
            GPIO_setPadConfig(i, GPIO_PIN_TYPE_PULLUP);
            GPIO_setQualificationMode(i, GPIO_QUAL_ASYNC);
        }
    }
}



uint16_t emifc_read_flash_data(uint32_t startAddr, uint32_t memSize){
    static uint16_t iterCnt = 0;
    uint16_t memReadData;
    uint16_t memWriteData;
    uint16_t *memPtr;
    uint32_t i;

    iterCnt++;
    memPtr = (uint16_t *)startAddr;


    //
    // Verify data written to memory.
    //
    memWriteData = 0x0301U;
    memPtr = (uint16_t *)startAddr;
    for(i = 0U; i < memSize; i++)
    {
        memReadData = *memPtr;
        if(memReadData != memWriteData)
        {
            return(1U);
        }
        memPtr++;
        memWriteData += (0x0001U + iterCnt);
    }
    return 0;
}


uint16_t emifc_write_flash_data(uint32_t startAddr, uint32_t memSize){
    static uint16_t iterCnt = 0;
    uint16_t memWriteData = 0x0301U;
    uint16_t *memPtr;

    iterCnt++;
    memPtr = (uint16_t *)startAddr;

    for(uint32_t i = 0U; i < memSize; i++) {
        *memPtr++ = memWriteData;
        memWriteData += (0x0001U + iterCnt);
    }
    return 0;
}


void emifc_configure_cpu2(){

    EMIF_AsyncTimingParams tparam;
    // Grab EMIF1 For CPU2.
    //
    EMIF_selectMaster(EMIF1CONFIG_BASE, EMIF_MASTER_CPU2_G);

    //
    // Configure to run EMIF1 on full Rate. (EMIF1CLK = CPU1SYSCLK)
    //
    SysCtl_setEMIF1ClockDivider(SYSCTL_EMIF1CLK_DIV_1);

    //
    // Configures Normal Asynchronous Mode of Operation.
    //
    EMIF_setAsyncMode(EMIF1_BASE, EMIF_ASYNC_CS2_OFFSET,
                      EMIF_ASYNC_NORMAL_MODE);

    //
    // Disables Extended Wait Mode.
    //
    EMIF_disableAsyncExtendedWait(EMIF1_BASE, EMIF_ASYNC_CS2_OFFSET);

    //
    // Configure EMIF1 Data Bus Width.
    //
    EMIF_setAsyncDataBusWidth(EMIF1_BASE, EMIF_ASYNC_CS2_OFFSET,
                              EMIF_ASYNC_DATA_WIDTH_16);

    //
    // Configure the access timing for CS2 space.
    //
    tparam.rSetup = 0;
    tparam.rStrobe = 3;
    tparam.rHold = 0;
    tparam.turnArnd = 0;
    tparam.wSetup = 0;
    tparam.wStrobe = 1;
    tparam.wHold = 0;
    EMIF_setAsyncTimingParams(EMIF1_BASE, EMIF_ASYNC_CS2_OFFSET, &tparam);
    EMIF_selectMaster(EMIF1CONFIG_BASE, EMIF_MASTER_CPU1_NG);

    //
    // Sync CPU1 and CPU2.
    //
//    IPC_sync(IPC_CPU2_L_CPU1_R, IPC_FLAG11);

}

void emif_cpu1_test(EMIF1_Config* emif)
{
    emifc_pinmux_setup_memory(MEM_TYPE_RAM);
    emifc_cpu_write_memory(emif);
}
void emif_cpu2_test(EMIF1_Config* emif)
{
    emifc_cpu_read_memory(emif);
}


