/*
 * canopen_emcy.c
 *
 *  Created on: 11 aug. 2023
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#include <string.h>

#include "co_canopen.h"
#include "canopen_emcy.h"
#include "error_handling.h"
#include "log.h"

#pragma DATA_ALIGN(error_message, 4)
unsigned char error_message[9];   /* First Byte ('E') signals it is an EMCY log */

//The error register (index 0x1001:0) has to be updated by the application.
static void canopen_emcy_send(void)
{
    /* marks 'EMCY' */
    error_message[0] = 'E';

    /* store the error in CAN log  because it is sent over CANopen */
    log_store_can_log(8, error_message);    //TODO update code to read Bytes, not words

    /* set CANopen error register bits */
    coOdPutObj_u8(CO_INDEX_ERROR_REGISTER, 0, error_message[3]);

    /* send the error over CANopen */
    coEmcyWriteReq(error_message[1] | (error_message[2] << 8), (const uint8_t*)&(error_message[4]));
}

//The error register (index 0x1001:0) has to be updated by the application.
void canopen_emcy_send_no_errors(void)
{
    /* clear error message */
    memset(error_message, 0, sizeof(error_message));

    error_message[3] = EMCY_ERROR_CODE_GENERIC_ERROR;

    /* set CANopen error register bits */
    coOdPutObj_u8(CO_INDEX_ERROR_REGISTER, 0, EMCY_ERROR_CODE_GENERIC_ERROR);

    canopen_emcy_send();
}

 static void canopen_emcy_send_temperature(void)
{
    /* set common Bytes for error codes */
    error_message[1] = EMCY_ERROR_TEMPERATURE & 0xff;
    error_message[2] = (EMCY_ERROR_TEMPERATURE >> 8) & 0xff;
    error_message[3] = EMCY_ERROR_CODE_TEMPERATURE;

    /* send the error over CANopen */
//    coEmcyWriteReq(EMCY_ERROR_CODE_DEVICE_TEMPERATURE, &(error_message[4]));
    canopen_emcy_send();
}

 void canopen_emcy_send_temperature_ok(uint8_t temperature)
{
    /* clear error message */
    memset(error_message, 0, sizeof(error_message));

    /* set common Bytes for error codes */
    error_message[4] = DEVICE_TEMPERATURE_OK;
    error_message[5] = temperature;

    canopen_emcy_send_temperature();
}

 void canopen_emcy_send_temperature_warning(uint8_t temperature)
{
    /* clear error message */
    memset(error_message, 0, sizeof(error_message));

    /* set common Bytes for error codes */
    error_message[4] = DEVICE_TEMPERATURE_HIGH;
    error_message[5] = temperature;

    canopen_emcy_send_temperature();
}

void canopen_emcy_send_temperature_error(uint8_t temperature)
{
    /* clear error message */
    memset(error_message, 0, sizeof(error_message));

    /* set common Bytes for error codes */
    error_message[4] = DEVICE_TEMPERATURE_MAX;
    error_message[5] = temperature;

    canopen_emcy_send_temperature();
}

void canopen_emcy_send_dcbus_over_voltage(uint8_t status)
{
    /* clear error message */
    memset(error_message, 0, sizeof(error_message));

    /* set common Bytes for error codes */
    error_message[1] = EMCY_ERROR_BUS_OVER_VOLTAGE & 0xff;
    error_message[2] = (EMCY_ERROR_BUS_OVER_VOLTAGE >> 8) & 0xff;
    error_message[3] = EMCY_ERROR_CODE_VOLTAGE;
    error_message[4] = status;

    /* send the error over CANopen */
    canopen_emcy_send();
}

void canopen_emcy_send_dcbus_under_voltage(uint8_t status, uint8_t payload[4])
{
    /* clear error message */
    memset(error_message, 0, sizeof(error_message));

    /* set common Bytes for error codes */
    error_message[1] = EMCY_ERROR_BUS_UNDER_VOLTAGE & 0xff;
    error_message[2] = (EMCY_ERROR_BUS_UNDER_VOLTAGE >> 8) & 0xff;
    error_message[3] = EMCY_ERROR_CODE_VOLTAGE;
    error_message[4] = status;
    error_message[5] = payload[0];
    error_message[6] = payload[1];
    error_message[7] = payload[2];
    error_message[8] = payload[3];

    /* send the error over CANopen */
    canopen_emcy_send();
}

void canopen_emcy_send_dcbus_short_curcuit(uint8_t status, uint8_t payload[4])
{
    /* clear error message */
    memset(error_message, 0, sizeof(error_message));

    /* set common Bytes for error codes */
    error_message[1] = EMCY_ERROR_BUS_SHORT_CIRCUIT & 0xff;
    error_message[2] = (EMCY_ERROR_BUS_SHORT_CIRCUIT >> 8) & 0xff;
    error_message[3] = EMCY_ERROR_CODE_VOLTAGE;
    error_message[4] = status;
    error_message[5] = payload[0];
    error_message[6] = payload[1];
    error_message[7] = payload[2];
    error_message[8] = payload[3];

    /* send the error over CANopen */
    canopen_emcy_send();
}

void canopen_emcy_send_power_sharing_error(uint8_t status)
{
    /* clear error message */
    memset(error_message, 0, sizeof(error_message));

    /* set common Bytes for error codes */
    error_message[1] = EMCY_ERROR_POWER_SHARING & 0xff;
    error_message[2] = (EMCY_ERROR_POWER_SHARING >> 8) & 0xff;
    error_message[3] = EMCY_ERROR_CODE_VOLTAGE;
    error_message[4] = status;

    /* send the error over CANopen */
    canopen_emcy_send();
}

void canopen_emcy_send_load_overcurrent(uint8_t status)
{
    /* clear error message */
    memset(error_message, 0, sizeof(error_message));

    /* set common Bytes for error codes */
    error_message[1] = EMCY_ERROR_OVER_CURRENT & 0xff;
    error_message[2] = (EMCY_ERROR_OVER_CURRENT >> 8) & 0xff;
    error_message[3] = EMCY_ERROR_CODE_CURRENT;
    error_message[4] = status;

    /* send the error over CANopen */
    canopen_emcy_send();
}

void canopen_emcy_send_operational_error(uint8_t status)
{
    /* clear error message */
    memset(error_message, 0, sizeof(error_message));

    /* set common Bytes for error codes */
    error_message[1] = EMCY_ERROR_OPERATIONAL & 0xff;
    error_message[2] = (EMCY_ERROR_OPERATIONAL >> 8) & 0xff;
    error_message[3] = EMCY_ERROR_CODE_MF_SPECIFIC;
    error_message[4] = status;

    /* send the error over CANopen */
    canopen_emcy_send();
}

void canopen_emcy_send_power_line_failure(uint8_t status)
{
    /* clear error message */
    memset(error_message, 0, sizeof(error_message));

    /* set common Bytes for error codes */
    error_message[1] = EMCY_ERROR_EXT_PWR_LOSS & 0xff;
    error_message[2] = (EMCY_ERROR_EXT_PWR_LOSS >> 8) & 0xff;
    error_message[3] = EMCY_ERROR_CODE_MF_SPECIFIC;
    error_message[4] = status;

    /* send the error over CANopen */
    canopen_emcy_send();
}

void canopen_emcy_send_power_line_failure_both(uint8_t status)
{
    /* clear error message */
    memset(error_message, 0, sizeof(error_message));

    /* set common Bytes for error codes */
    error_message[1] = EMCY_ERROR_EXT_PWR_LOSS_BOTH & 0xff;
    error_message[2] = (EMCY_ERROR_EXT_PWR_LOSS_BOTH >> 8) & 0xff;
    error_message[3] = EMCY_ERROR_CODE_MF_SPECIFIC;
    error_message[4] = status;

    /* send the error over CANopen */
    canopen_emcy_send();
}

void canopen_emcy_send_state_of_charge_safety_error(uint8_t status, uint8_t soc)
{

    /* clear error message */
    memset(error_message, 0, sizeof(error_message));

    /* set common Bytes for error codes */
    error_message[1] = EMCY_ERROR_SOC_BELOW_SAFETY_THRESHOLD & 0xff;
    error_message[2] = (EMCY_ERROR_SOC_BELOW_SAFETY_THRESHOLD >> 8) & 0xff;
    error_message[3] = EMCY_ERROR_CODE_MF_SPECIFIC;
    error_message[4] = status;
    error_message[5] = soc;

    /* send the error over CANopen */
    canopen_emcy_send();
}

void canopen_emcy_send_state_of_charge_min_error(uint8_t status, uint8_t soc)
{

    /* clear error message */
    memset(error_message, 0, sizeof(error_message));

    /* set common Bytes for error codes */
    error_message[1] = EMCY_ERROR_SOC_BELOW_LIMIT & 0xff;
    error_message[2] = (EMCY_ERROR_SOC_BELOW_LIMIT >> 8) & 0xff;
    error_message[3] = EMCY_ERROR_CODE_MF_SPECIFIC;
    error_message[4] = status;
    error_message[5] = soc;

    /* send the error over CANopen */
    canopen_emcy_send();
}

void canopen_emcy_send_generic(uint16_t emcy_error_code,
                               uint8_t emcy_error_byte,
                               uint8_t status,
                               uint8_t payload[4])
{

    /* clear error message */
    memset(error_message, 0, sizeof(error_message));

    /* set common Bytes for error codes */
    error_message[1] = (emcy_error_code     ) & 0xff;
    error_message[2] = (emcy_error_code >> 8) & 0xff;
    error_message[3] = emcy_error_byte;
    error_message[4] = status;
    error_message[5] = payload[0];
    error_message[6] = payload[1];
    error_message[7] = payload[2];
    error_message[8] = payload[3];

    /* send the error over CANopen */
    canopen_emcy_send();
}

/* brief: Check if there are any errors. If not, send EMCY OK
 *
 * details:
 *
 * requirements:
 *
 * argument: none
 *
 * return: none
 *
 * note:
 *
 */
void canopen_emcy_send_reboot_warning(void)
{
    /* clear error message */
    memset(error_message, 0, sizeof(error_message));

    /* set common Bytes for error codes */
    error_message[1] = EMCY_ERROR_REBOOT_WARNING & 0xff;
    error_message[2] = (EMCY_ERROR_REBOOT_WARNING >> 8) & 0xff;
    error_message[3] = EMCY_ERROR_CODE_MF_SPECIFIC;

    /* send EMCY OK */
    canopen_emcy_send();

}
