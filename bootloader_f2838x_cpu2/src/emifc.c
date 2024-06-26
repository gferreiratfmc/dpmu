#include <emifc.h>
#include "driverlib.h"
#include "device.h"
#include "emif.h"
#include "cli_cpu2.h"
#include "stdio.h"

static void emifc_pinmux_setup_memory(uint16_t memType);

void emifc_cpu_write_memory(EMIF1_Config* emif1)
{
    uint32_t i;
    uint16_t* memPtr = (uint16_t *)emif1->address;

    for(i = 0U; i < emif1->size; i++) {
        *(memPtr+i) = emif1->data[i];
    }
}

void emifc_set_cpun_as_master(EMIF1_Config* emif1)
{
    if (emif1->cpuType == CPU_TYPE_ONE){
        // Grab EMIF1 for CPU1.
        while(HWREGH(EMIF1CONFIG_BASE + MEMCFG_O_EMIF1MSEL) != EMIF_MASTER_CPU1_G){
            EMIF_selectMaster(EMIF1CONFIG_BASE, EMIF_MASTER_CPU1_G);
        }
    }

    if (emif1->cpuType == CPU_TYPE_TWO){
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

void emifc_cpu_read_memory(EMIF1_Config* emif1)
{
    emifc_set_cpun_as_master(emif1);

    uint32_t i;
    uint16_t* memPtr = (uint16_t *)emif1->address;
    for(i = 0U; i < emif1->size; i++){
        emif1->data[i] = *(memPtr + i);
    }
}

void emifc_configuration()
{
    //    emifc_pinmux_setup_flash();
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
    }
    if (memType == MEM_TYPE_RAM){
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
//    IPC_waitForFlag(IPC_CPU2_L_CPU1_R, IPC_FLAG11);

    // Grab EMIF1 for CPU2.
    while(HWREGH(EMIF1CONFIG_BASE + MEMCFG_O_EMIF1MSEL) != EMIF_MASTER_CPU2_G){
        EMIF_selectMaster(EMIF1CONFIG_BASE, EMIF_MASTER_CPU2_G);
    }
}
