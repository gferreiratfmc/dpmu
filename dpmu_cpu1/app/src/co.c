/*
* main.c - contains program main
*
* Copyright (c) 2012-2020 emotas embedded communication GmbH
*-------------------------------------------------------------------
* $Id: main.c 31316 2020-03-24 11:25:56Z hil $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \file
* \brief main routine
* This CANopen slave1 example sends an Emergency message after bootup.
* When started via NMT command,
* it sends and receives PDOs on the default COB-IDs of TPDO1 and RPDO1.
*
*/

/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include <app/device_profile/gen_define.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* header of project specific types
---------------------------------------------------------------------------*/
#include <co_canopen.h>

#include <canopen/codrv/tms320f2837xd/codrv_cpu_28379d.h>

#include <co_p401.h>
#include <error_handling.h>

#include "fwupdate.h"
#include "device.h"
//#include <test/test_flash.h>
//#include <test/test_log.h>
#include "co.h"
#include "canopen_sdo_download_indices.h"
#include "canopen_sdo_upload_indices.h"


/* constant definitions
---------------------------------------------------------------------------*/

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
--------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/
static void errorHandler(UNSIGNED8 error);
static RET_T nmtInd(BOOL_T  execute, CO_NMT_STATE_T newState);
static void hbState(UNSIGNED8   nodeId, CO_ERRCTRL_T state,
        CO_NMT_STATE_T  nmtState);
static RET_T sdoServerReadInd(BOOL_T execute, UNSIGNED8 sdoNr, UNSIGNED16 index,
        UNSIGNED8   subIndex);
static RET_T sdoServerCheckWriteInd(BOOL_T execute, UNSIGNED8 node,
        UNSIGNED16 index, UNSIGNED8 subIndex, const UNSIGNED8 *pData);
static RET_T sdoServerWriteInd(BOOL_T execute, UNSIGNED8 sdoNr,
        UNSIGNED16 index, UNSIGNED8 subIndex);;
//static RET_T sdoSwitchesWriteInd(BOOL_T execute, UNSIGNED8 sdoNr,
//        UNSIGNED16 index, UNSIGNED8 subIndex);
static void pdoInd(UNSIGNED16);
static void pdoRecEvent(UNSIGNED16);
static void canInd(CO_CAN_STATE_T);
static void commInd(CO_COMM_STATE_EVENT_T);
static void ledGreenInd(BOOL_T);
static void ledRedInd(BOOL_T);


/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/


/***************************************************************************/
/**
* \brief main entry
*
* \param
*   nothing
* \results
*   nothing
*/



void co_init(void){
UNSIGNED8   emcyData[5] = { 6, 7, 8, 9, 15 };

    /* hardware initialization */
    codrvHardwareInit();

//  debug_serial_setup();
//  debug_print_enabled(true);
//
//  printf("Application has started\r\n");

    if (codrvCanInit(125) != RET_OK)  {
        errorHandler(1);
    }

    if (codrvTimerSetup(CO_TIMER_INTERVAL) != RET_OK)  {
        errorHandler(2);
    }

    /* CANopen Initialization */

    if (coCanOpenStackInit(NULL) != RET_OK)  {
        printf("error init library\r\n");
        errorHandler(1);
    }

    /* Profile 401 Initialization */

    if (co401Init()!=RET_OK){
       printf("Error Init 401 Profile \r\n");
        errorHandler(1);
    }

    /* register indication functions */
    if (coEventRegister_NMT(nmtInd) != RET_OK)  {
        errorHandler(3);
    }
    if (coEventRegister_ERRCTRL(hbState) != RET_OK)  {
        errorHandler(4);
    }
    if (coEventRegister_SDO_SERVER_READ(sdoServerReadInd) != RET_OK)  {
        errorHandler(5);
    }
    if (coEventRegister_SDO_SERVER_CHECK_WRITE(sdoServerCheckWriteInd) != RET_OK)  {
        errorHandler(6);
    }
    if (coEventRegister_SDO_SERVER_WRITE(sdoServerWriteInd) != RET_OK)  {
        errorHandler(7);
    }
    if (coEventRegister_PDO(pdoInd) != RET_OK)  {
        errorHandler(8);
    }
    if (coEventRegister_PDO_REC_EVENT(pdoRecEvent) != RET_OK)  {
        errorHandler(9);
    }
    if (coEventRegister_LED_GREEN(ledGreenInd) != RET_OK)  {
        errorHandler(10);
    }
    if (coEventRegister_LED_RED(ledRedInd) != RET_OK)  {
        errorHandler(11);
    }
    if (coEventRegister_CAN_STATE(canInd) != RET_OK)  {
        errorHandler(12);
    }
    if (coEventRegister_COMM_EVENT(commInd) != RET_OK)  {
        errorHandler(13);
    }

    /* Enable Global Interrupt (INTM) and realtime interrupt (DBGM) */
    EINT;
    ERTM;

#include "usr_401.h"
//    coEventRegister_401 (digitalPortInputSimulate, NULL, analogPortInputSimulate, NULL);
    coEventRegister_401 (byte_in_piInd, byte_out_piInd, NULL, NULL);    /* pDI, pDO, pAI, pAO */

    /* enable CAN communication */
    if (codrvCanEnable() != RET_OK)  {
        errorHandler(14);
    }

//  setErrorRegister();
    /* send emcy message */
    if (coEmcyWriteReq(ERROR_OPERATIONAL, &emcyData[0]) != RET_OK)  {
        errorHandler(15);
    }

    if (coEmcyWriteReq(EMCY_ERROR_LOGGING, NULL) != RET_OK)  {
            errorHandler(15);
    }
//#include "switches.h"
//    if (coEventRegister_SDO_SERVER_WRITE(sdoSwitchesWriteInd) != RET_OK)  {
//        errorHandler(16);
//    }


//     test_log_execute();


#if 0
    /* CANopen main loop */
    while (1)  {
        while (coCommTask() == CO_TRUE)  {
            /* more CAN messages to process
             * coCommTask() is called again */
        }

        co401Task();
        update_count();
//      DEVICE_DELAY_US(2000000);


    }
#endif
}


/*********************************************************************/
static RET_T nmtInd(
        BOOL_T  execute,
        CO_NMT_STATE_T  newState
    )
{
    printf("nmtInd: New Nmt state %d - execute %d\r\n", newState, execute);

    return(RET_OK);
}


/*********************************************************************/
static void pdoInd(
        UNSIGNED16  pdoNr
    )
{
    printf("pdoInd: pdo %d received\r\n", pdoNr);
//    readFromOD();
}


/*********************************************************************/
static void pdoRecEvent(
        UNSIGNED16  pdoNr
    )
{
    printf("pdoRecEvent: pdo %d time out\r\n", pdoNr);
}


/*********************************************************************/
static void hbState(
        UNSIGNED8   nodeId,
        CO_ERRCTRL_T state,
        CO_NMT_STATE_T  nmtState
    )
{
    printf("hbInd: HB Event %d node %d nmtState: %d\r\n", state, nodeId, nmtState);

    return;
}


/*********************************************************************/
static RET_T sdoServerReadInd(
        BOOL_T      execute,
        UNSIGNED8   sdoNr,
        UNSIGNED16  index,
        UNSIGNED8   subIndex
    )
{
    printf("sdo server read ind: exec: %d, sdoNr %d, index %x:%d\r\n",
        execute, sdoNr, index, subIndex);

   // return(RET_INVALID_PARAMETER);
    RET_T retVal;
    retVal = co_usr_sdo_ul_indices(execute, sdoNr, index, subIndex);

    return(retVal);
}


/*********************************************************************/
static RET_T sdoServerCheckWriteInd(
        BOOL_T      execute,
        UNSIGNED8   sdoNr,
        UNSIGNED16  index,
        UNSIGNED8   subIndex,
        const UNSIGNED8 *pData
    )
{
    printf("sdo server check write ind: exec: %d, sdoNr %d, index %x:%d\r\n",
        execute, sdoNr, index, subIndex);

   // return(RET_INVALID_PARAMETER);
    return(RET_OK);
}

/*********************************************************************/
static RET_T sdoServerWriteInd(
        BOOL_T      execute,
        UNSIGNED8   sdoNr,
        UNSIGNED16  index,
        UNSIGNED8   subIndex
    )
{
    printf("sdo server write ind: exec: %d, sdoNr %d, index %x:%d\n",
        execute, sdoNr, index, subIndex);

#ifdef BOOT
    if ((execute == CO_TRUE) && (index == 0x1f51)) {
        UNSIGNED8 u8tmp;
        RET_T retVal;

        retVal = coOdGetObj_u8(index, 1, &u8tmp);
        if (retVal == RET_OK) {
            if (u8tmp == 0) {
                /* disable CAN */
                DINT;
                codrvCanDisable();

                /* disable other peripherie components */
                Device_disableAllPeripherals();

                /* jump to bootloader */
                jump2Bootloader();
                return RET_OK;
            }
        }
    }
#endif /* BOOT */

    RET_T retVal;
    retVal = co_usr_sdo_dl_indices(execute, sdoNr, index, subIndex);

    return(retVal);
}

//#include "gen_indices.h"
////#include "co_odaccess.h"
//#include "common/inc/switches.h"
//
//static RET_T sdoSwitchesWriteInd(
//        BOOL_T      execute,
//        UNSIGNED8   sdoNr,
//        UNSIGNED16  index,
//        UNSIGNED8   subIndex
//    )
//{
//    printf("sdo server write switches ind: exec: %d, sdoNr %d, index %x:%d\n",
//        execute, sdoNr, index, subIndex);
//
//    if ((execute == CO_TRUE) && (index == 0x4000)) {
//        uint8_t state;
//        RET_T retVal;
//
//        retVal = coOdGetObj_u8(I_SWITCH_STATE, S_SW_QINRUSH_STATE1, &state);  /*get unsigned integer 8 bits*/
//        if (retVal == RET_OK)
//            switches_Qinrush_digital(state);
//
//        retVal = coOdGetObj_u8(I_SWITCH_STATE, S_SW_QLB_STATE1, &state);
//        if (retVal == RET_OK)
//            switches_Qlb(state);
//
//        retVal = coOdGetObj_u8(I_SWITCH_STATE, S_SW_QSB_STATE1, &state);
//        if (retVal == RET_OK)
//            switches_Qsb(state);
//
//        retVal = coOdGetObj_u8(I_SWITCH_STATE, S_SW_QINB_STATE, &state);
//        if (retVal == RET_OK)
//            switches_Qinb(state);
//    }
//
//    return(RET_OK);
//}

/*********************************************************************/
static void canInd(
    CO_CAN_STATE_T  canState
    )
{
    switch (canState)  {
    case CO_CAN_STATE_BUS_OFF:
        printf("CAN: Bus Off\r\n");
        break;
    case CO_CAN_STATE_BUS_ON:
        printf("CAN: Bus On\r\n");
        break;
    case CO_CAN_STATE_PASSIVE:
        printf("CAN: Passive\r\n");
        break;
    default:
        break;
    }
}


/*********************************************************************/
static void commInd(
        CO_COMM_STATE_EVENT_T   commEvent
    )
{
    switch (commEvent)  {
    case CO_COMM_STATE_EVENT_CAN_OVERRUN:
        printf("COMM-Event: CAN Overrun\r\n");
        break;
    case CO_COMM_STATE_EVENT_REC_QUEUE_FULL:
        printf("COMM-Event: Rec Queue Full\r\n");
        break;
    case CO_COMM_STATE_EVENT_REC_QUEUE_OVERFLOW:
        printf("COMM-Event: Rec Queue Overflow\r\n");
        break;
    case CO_COMM_STATE_EVENT_REC_QUEUE_EMPTY:
        printf("COMM-Event: Rec Queue Empty\r\n");
        break;
    case CO_COMM_STATE_EVENT_TR_QUEUE_FULL:
        printf("COMM-Event: Tr Queue Full\r\n");
        break;
    case CO_COMM_STATE_EVENT_TR_QUEUE_OVERFLOW:
        printf("COMM-Event: Tr Queue Overflow\r\n");
        break;
    case CO_COMM_STATE_EVENT_TR_QUEUE_EMPTY:
        printf("COMM-Event: Tr Queue Empty\r\n");
        break;
    default:
        printf("COMM-Event: %d\r\n", commEvent);
        break;
    }

}


/*********************************************************************/
static void ledGreenInd(
        BOOL_T  on
    )
{
    printf("GREEN: %d\r\n", on);
}


/*********************************************************************/
static void ledRedInd(
        BOOL_T  on
    )
{
    printf("RED: %d\r\n", on);
}


/*********************************************************************/
static void errorHandler(
        UNSIGNED8 error
    )
{
    printf("exit with error: %u\r\n", error);

    while (1)  {
        /* do nothing */
    }
}
