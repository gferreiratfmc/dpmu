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
#include "GlobalV.h"
#include "main.h"

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

void OWN_DMA_init();


#endif /* APP_INC_DMASET_H_ */
