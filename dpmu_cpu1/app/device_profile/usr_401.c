/*
* usr_401.c -  process io functions needed for co_p401.c
*
* Copyright (c) 2013 emotas embedded communication GmbH
*-------------------------------------------------------------------
* $Id: usr_401.c 29114 2019-08-30 15:38:11Z phi $
*
*
*-------------------------------------------------------------------
*
*
*/


/********************************************************************/
/**
* \brief Implements hardware access to fulfill co_p401.c requirements
*
* These functions are called by the CANopen library module co_p401
* which implements the CiA 401 profile behaviour.
* The user has to register the functions to the CANopen stack
* before the stack is activated.
*
* \file usr_401.c
*
*/


/* header of standard C - libraries
---------------------------------------------------------------------------*/
#include "cli_cpu1.h"
#include "common.h"
#include "driverlib.h"
#include "switches_pins.h"

#include <gen_define.h>
#include <stddef.h>
#include <stdio.h>      /* debug using printf() */
#include <stdlib.h>     /* system() */

/* header of project specific types
---------------------------------------------------------------------------*/


#ifdef CO_PROFILE_401
#include <co_datatype.h>
#include <co_timer.h>

#include <usr_401.h>

/* constant definitions
---------------------------------------------------------------------------*/

/* local defined data types
---------------------------------------------------------------------------*/

/* list of external used functions, if not in headers
---------------------------------------------------------------------------*/

/* list of global defined functions
---------------------------------------------------------------------------*/

/* list of local defined functions
---------------------------------------------------------------------------*/

/* external variables
---------------------------------------------------------------------------*/

/* global variables
---------------------------------------------------------------------------*/

/* local defined variables
---------------------------------------------------------------------------*/




/***************************************************************************/
/**
* \brief - set an 8 bit digital output - printf()
*
* This simple version uses only printf() to print the information
* to the stdout of the process.
*
* This function has to be registered using coEventRegister_401()
* \code
*   coEventRegister_401(NULL, byte_out_printfInd, NULL, NULL)
* \endcode
*
* \return void
*
*/
void byte_out_printfInd(
        UNSIGNED8 port,         /**< selected 8bit port */
        UNSIGNED16 outVal       /**< output value */
    )
{
    printf("digout port:%2d, value:0x%02x, %3d\n",
            port, outVal, outVal);
}


/***************************************************************************/
/**
* \brief - set an 8 bit digital output - process image
*
* This version uses  a virtual file system
* where digout ports are files named like the port number
* under the directory digout.
* The port value is written there as decimal value.
*
* This function has to be registered using coEventRegister_401()
* \code
*   coEventRegister_401(NULL, byte_out_piInd, NULL, NULL)
* \endcode
*
* \return void
*
*/

void switches_set_state(UNSIGNED8 outVal)
{
//    cli_switches(IPC_SWITCHES_QIRS, (outVal & (1 << SWITCHES_QIRS) ? SW_ON : SW_OFF));
//    cli_switches(IPC_SWITCHES_QINB, (outVal & (1 << SWITCHES_QINB) ? SW_ON : SW_OFF));
//    cli_switches(IPC_SWITCHES_QLB,  (outVal & (1 << SWITCHES_QLB)  ? SW_ON : SW_OFF));
//    cli_switches(IPC_SWITCHES_QSB,  (outVal & (1 << SWITCHES_QSB)  ? SW_ON : SW_OFF));
}

void byte_out_piInd(
        UNSIGNED8 port,         /**< selected 8bit port */
        UNSIGNED8 outVal        /**< output value */
    )
{
//char command[100];

//  snprintf(command, 100, "/bin/echo %d > digout/%d", outVal, port);
//  system(command);

    printf("outVal: %d, port: %d\r\n",outVal, port);
//    Serial_debug(DEBUG_INFO, &cli_serial, "> Port %x  Pins %x\r\n", port, outVal);
    //TODO this continuously turns off all switches wile CAN bus error
    switches_set_state(outVal);

}

/* check state of single switch */
uint8_t switches_read_switch_state(switches_t switchNr)
{
    uint8_t pinValues = 0;

    switch (switchNr)
    {
    case SWITCHES_QIRS:
        if(GPIO_readPin(Qinrush))
            pinValues |= 1 << SWITCHES_QIRS;
        break;
    case SWITCHES_QINB:
        if(GPIO_readPin(Qinb))
            pinValues |= 1 << SWITCHES_QINB;
        break;
    case SWITCHES_QLB:
        if(GPIO_readPin(Qlb))
            pinValues |= 1 << SWITCHES_QLB;
        break;
    case SWITCHES_QSB:
        if(GPIO_readPin(Qsb))
            pinValues |= 1 << SWITCHES_QSB;
        break;
    }

    return pinValues;
}

/* check state of all switches switch */
uint8_t switches_read_states(void)
{
    uint8_t pinValues = 0;

    if(GPIO_readPin(Qinrush))
        pinValues |= 1 << SWITCHES_QIRS;
    if(GPIO_readPin(Qinb))
        pinValues |= 1 << SWITCHES_QINB;
    if(GPIO_readPin(Qlb))
        pinValues |= 1 << SWITCHES_QLB;
    if(GPIO_readPin(Qsb))
        pinValues |= 1 << SWITCHES_QSB;

    return pinValues;
}

/***************************************************************************/
/**
* \brief - read an 8 bit digital input - process image
*
* This version uses  a virtual file system
* where digin ports are files named like the port number
* under the directory digin.
* The port value is read there as decimal value.
*
* This function has to be registered using coEventRegister_401()
* \code
*   coEventRegister_401(byte_in_piInd, NULL, NULL, NULL)
* \endcode
*
* \return void
*
*/

UNSIGNED8 byte_in_piInd(
        UNSIGNED8 port,         /**< selected 8bit port */
        UNSIGNED8 filter        /**< port filter value */
    )
{
//FILE *fp;
//char file[100];
UNSIGNED8 value = 0;


//    (void)filter;           /* not used in this version */
//    snprintf(file, 100, "digin/%d", port);
//    fp = fopen(file, "r");
//    if (fp != NULL) {
//        fgets(file, 100, fp);
//        value = atoi(file);
//        fclose(fp);
//    } else {
////      debug_print("process image at \"%s\" not found \r\n", file);
//        exit(14);
//    }

//    Serial_debug(DEBUG_INFO, &cli_serial, "< Port %x  Pins %x\r\n", port, filter);
    value = switches_read_states();

    return value;
}



/***************************************************************************/
/**
* \brief - set an 16bit analog output - printf()
*
* This simple version uses only printf() to print the information
* to the stdout of the process.
*
* This function has to be registered using coEventRegister_401()
* \code
* coEventRegister_401(NULL, NULL, NULL, analog_out_printfInd)
* \endcode
*
* \return void
*
*/
void analog_out_printfInd(
        UNSIGNED8 port,         /**< selected 16bit analog channel */
        INTEGER16 outVal        /**< output value */
    )
{
    printf("analog out port:%2d, value:%5d\n",
            port, outVal);
}

/***************************************************************************/
/**
* \brief - set an 16bit analog output - process image
*
* This version uses a virtual file system
* where anout ports are files named like the port number
* under the directory anout.
* The port value is written there as decimal value.
*
* \note
* All analog channels are 16 bit integer.
* If hardware has a lower resolution, use the most left aligned bits.
*
* This function has to be registered using coEventRegister_401()
* \code
*   coEventRegister_401(NULL, NULL, NULL, analog_out_piInd)
* \endcode
*
*  \return void
*
*/
void analog_out_piInd(
        UNSIGNED8 port,         /**< selected 16bit analog channel */
        INTEGER16 outVal        /**< output value */
    )
{
char command[100];

    printf("analog out port:%2d, value:%5d\n",
            port, outVal);
    snprintf(command, 100, "/bin/echo %d > anout/%d", outVal, port);
//  system(command);
}

/***************************************************************************/
/**
* \brief - read an 16 bit analog input - process image
*
* This version uses  a virtual file system
* where anin ports are files named like the port number
* under the directory anin.
* The port value is read there as decimal value.
*
* \note
* All analog channels are 16 bit integer.
* If hardware has a lower resolution, use the most left aligned bits.
*
* This function has to be registered using coEventRegister_401()
* \code
*   coEventRegister_401(NULL, NULL, analog_in_piInd, NULL)
* \endcode
*
* \return void
*
*/

INTEGER16 analog_in_piInd(
        UNSIGNED8 port          /**< selected 8bit port */
    )
{
FILE *fp;
char file[100];
INTEGER16 value;

    snprintf(file, 100, "anin/%d", port);
    fp = fopen(file, "r");
    if (fp != NULL) {
        fgets(file, 100, fp);
        value = atoi(file);
        fclose(fp);
    } else {
        printf("process image at \"%s\" not found\n", file);
        exit(14);
    }
    // printf("Input from port %2d, val=%7d\n", port, value);
    return value;
}




#endif /* CO_PROFILE_401 */
/* vim: set ts=4 sw=4 spelllang=en : */
