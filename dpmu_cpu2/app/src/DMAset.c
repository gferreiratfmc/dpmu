/*
  ******************************************************************************
  * @file    DMAset.c
  * @brief   This file provides code for the DMA congfig
  *

  ******************************************************************************
  *          Created on: 10 Feb 2023
  *          Author: Luyu Wang
  *
  ******************************************************* ***********************
 */

#include "board.h"
#include "DMAset.h"


void OWN_DMA1_init();
//void OWN_DMA2_init();
void OWN_DMA3_init();
void OWN_DMA4_init();

void OWN_DMA_init()
{
//    DMA_initController();
//    OWN_DMA1_init();
////    OWN_DMA2_init();  /* DMA2_BASE is used by CPU1 only */
//    OWN_DMA3_init();
//    OWN_DMA4_init();
}

void OWN_DMA1_init()
{
    DMA_setEmulationMode(DMA_EMULATION_FREE_RUN);
//    DMA_configAddresses(DMA1_BASE, (uint16_t *)&AdcRawValue.V24f, (uint16_t *)(ADCARESULT_BASE+1));
    DMA_configBurst(DMA1_BASE, 1U, 1, 1);
    DMA_configTransfer(DMA1_BASE, 1U, 1, 1);
    DMA_configWrap(DMA1_BASE, 65535U, 0, 65535U, 0);
    DMA_configMode(DMA1_BASE, DMA_TRIGGER_EPWM12SOCB, DMA_CFG_ONESHOT_DISABLE | DMA_CFG_CONTINUOUS_ENABLE | DMA_CFG_SIZE_16BIT);
    DMA_setInterruptMode(DMA1_BASE, DMA_INT_AT_END);
    DMA_enableInterrupt(DMA1_BASE);
    DMA_disableOverrunInterrupt(DMA1_BASE);
    DMA_enableTrigger(DMA1_BASE);
    DMA_startChannel(DMA1_BASE);
}

/* DMA2_BASE is used by CPU1 only */
//void OWN_DMA2_init()
//{
//    DMA_setEmulationMode(DMA_EMULATION_FREE_RUN);
//    DMA_configAddresses(DMA2_BASE, (uint16_t *)&AdcRawValue.IF_1f, (uint16_t *)(ADCBRESULT_BASE+2U));
//    DMA_configBurst(DMA2_BASE, 3U, 1, 1);
//    DMA_configTransfer(DMA2_BASE, 1U, 1, 1);
//    DMA_configWrap(DMA2_BASE, 65535U, 0, 65535U, 0);
//    DMA_configMode(DMA2_BASE, DMA_TRIGGER_EPWM12SOCB, DMA_CFG_ONESHOT_DISABLE | DMA_CFG_CONTINUOUS_ENABLE | DMA_CFG_SIZE_16BIT);
//    DMA_setInterruptMode(DMA2_BASE, DMA_INT_AT_END);
//    DMA_enableInterrupt(DMA2_BASE);
//    DMA_disableOverrunInterrupt(DMA2_BASE);
//    DMA_enableTrigger(DMA2_BASE);
//    DMA_startChannel(DMA2_BASE);
//}

void OWN_DMA3_init()
{
    DMA_setEmulationMode(DMA_EMULATION_FREE_RUN);
//    DMA_configAddresses(DMA3_BASE, (uint16_t *)&AdcRawValue.ISen1f, (uint16_t *)(ADCCRESULT_BASE+0U));
    DMA_configBurst(DMA3_BASE, 2U, 1, 1);
    DMA_configTransfer(DMA3_BASE, 1U, 1, 1);
    DMA_configWrap(DMA3_BASE, 65535U, 0, 65535U, 0);
    DMA_configMode(DMA3_BASE, DMA_TRIGGER_EPWM12SOCB, DMA_CFG_ONESHOT_DISABLE | DMA_CFG_CONTINUOUS_ENABLE | DMA_CFG_SIZE_16BIT);
    DMA_setInterruptMode(DMA3_BASE, DMA_INT_AT_END);
    DMA_enableInterrupt(DMA3_BASE);
    DMA_disableOverrunInterrupt(DMA3_BASE);
    DMA_enableTrigger(DMA3_BASE);
    DMA_startChannel(DMA3_BASE);
}

void OWN_DMA4_init()
{
    DMA_setEmulationMode(DMA_EMULATION_FREE_RUN);
//    DMA_configAddresses(DMA4_BASE, (uint16_t *)&AdcRawValue.V_Upf, (uint16_t *)(ADCDRESULT_BASE+2U));
    DMA_configBurst(DMA4_BASE, 4U, 1, 1);
    DMA_configTransfer(DMA4_BASE, 1U, 1, 1);
    DMA_configWrap(DMA4_BASE, 65535U, 0, 65535U, 0);
    DMA_configMode(DMA4_BASE, DMA_TRIGGER_EPWM12SOCB, DMA_CFG_ONESHOT_DISABLE | DMA_CFG_CONTINUOUS_ENABLE | DMA_CFG_SIZE_16BIT);
    DMA_setInterruptMode(DMA4_BASE, DMA_INT_AT_END);
    DMA_enableInterrupt(DMA4_BASE);
    DMA_disableOverrunInterrupt(DMA4_BASE);
    DMA_enableTrigger(DMA4_BASE);
    DMA_startChannel(DMA4_BASE);
}
