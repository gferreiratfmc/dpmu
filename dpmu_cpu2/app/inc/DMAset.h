/*
  ******************************************************************************
  * @file    DMAset.h
  * @brief   This file provides code for the DMA congfig
  *

  ******************************************************************************
  *          Created on: 10 Feb 2023
  *          Author: Luyu Wang
  *
  ******************************************************* ***********************
 */

#ifndef APP_INC_DMASET_H_
#define APP_INC_DMASET_H_


#include "board.h"
#include "main.h"

#if 1
#define DMA1_BASE DMA_CH1_BASE
#define DMA2_BASE DMA_CH2_BASE
#define DMA3_BASE DMA_CH3_BASE
#define DMA4_BASE DMA_CH4_BASE
#define DMA1_BURSTSIZE 1U
#define DMA1_TRANSFERSIZE 1U
#define DMA2_BURSTSIZE 1U
#define DMA2_TRANSFERSIZE 1U
#define DMA3_BURSTSIZE 1U
#define DMA3_TRANSFERSIZE 1U
#define DMA4_BURSTSIZE 1U
#define DMA4_TRANSFERSIZE 1U
#endif

#if 1
//extern ADC_Raw ADCrawvalue;
////extern const void *SrcAddr;
//extern const void *DestAddr1;
//extern const void *DestAddr2;
//extern const void *DestAddr3;
//extern const void *DestAddr4;
//extern uint16_t Datasource[2];
#include "GlobalV.h"
#endif


void OWN_DMA_init();


#endif /* APP_INC_DMASET_H_ */
