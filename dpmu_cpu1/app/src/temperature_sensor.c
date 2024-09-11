/*
 * temperature_sensor.c
 *
 *  Created on: 3 feb. 2023
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "board.h"
#include "canopen_emcy.h"
#include "cli_cpu1.h"
#include "co_common.h"
#include "common.h"
#include "device.h"
#include "error_handling.h"
#include "gen_indices.h"
#include "i2c_com.h"
#include "initialization_app.h"
#include "log.h"
#include "main.h"
#include "serial.h"
#include "temperature_sensor.h"
#include "timer.h"

#define TEMPERATURE_READ_INTERVAL_TIME_IN_MILISECS 2000


struct I2CHandle temperature_sensor[4];
uint16_t TX_MsgBuffer[MAX_BUFFER_SIZE];
uint16_t RX_MsgBuffer[MAX_BUFFER_SIZE];
uint32_t controlAddress;
uint16_t status;
int16_t temperatureSensorVector[4];
int16_t temperatureHotPoint;
int16_t temperature_absolute_max_limit;
int16_t temperature_high_limit;
/*
 * returns pointer do wanted sensor struct
 */
static struct I2CHandle* select_temperature_sensor(uint8_t temp_sensor_number)
{
    return &temperature_sensor[temp_sensor_number];
}

/*
 * run select_temperature_sensor() before to get the pointer
 */
static int16_t temperature_sensor_read_register(struct I2CHandle *temperatureSensor)
{
    int16_t status = STATUS_S_SUCCESS;

    /* receive data */
    status = I2C_MasterReceiver(temperatureSensor);

    return status;
}

/*
 * run select_temperature_sensor() before to get the pointer
 */
static int16_t temperature_sensor_write_register(struct I2CHandle *temperatureSensor)
{
    int16_t status = 0;

    /* send data */
    status = I2C_MasterTransmitter(temperatureSensor);

    return status;
}



/* reads the temperature from the sensor
 *
 * Attribute:
 *  temp_sensor_number  - number of sensor to read
 *  readValue           - pointer to where to store the measured value
 *
 * Return:
 *  returns the status of the measurement, 0 for success
 *
 * Caveats:
 *  if unsuccessful read of temperature sensor the measured value will be set to 0
 *
 * Presumptions: none
 */
int16_t temperature_sensor_read_temperature(uint8_t temp_sensor_number, int16_t *readValue)
{
    int16_t  status = 0;
    static uint16_t readBuffer[2] = {0, 0};
    struct I2CHandle *temperatureSensor;
    temperatureSensor = select_temperature_sensor(temp_sensor_number);

    /* set up struct */
    temperatureSensor->NumOfDataBytes = 2;
    temperatureSensor->NumOfAddrBytes = 1;
    temperatureSensor->pRX_MsgBuffer = readBuffer;
    controlAddress = TMP100_TEMP_REG;

    /* receive temperature */
    status = temperature_sensor_read_register(temperatureSensor);
    if(STATUS_S_SUCCESS == status)
    {
        int16_t  lowByte = readBuffer[1] & 0xff;
        int16_t highByte = readBuffer[0] & 0xff;

        *readValue  = highByte << 8;
        *readValue |= lowByte;
    } else
    {
        *readValue = 0;
    }

    /* convert to temperature
     *
     * the lower nibble of lsb is always zero
     * the upper nibble of lsb are the decimals
     * */
    *readValue >>= 8;    /* integer, no decimals */

    return status;
}

void temperature_sensor_read_all_temperatures(void)
{
//    static bool absolute_max_limit_reached_flag = false;
//    static bool high_limit_reached = false;
//    static bool normal_reached = true;
    int16_t status = SUCCESS;
    int16_t read_temperature;
    static int16_t read_temperature_max = 0;
    static uint16_t sensorCount = 0;

    switch( sensorCount )
    {
        /* DSP BASE */
        case 0:
            if( SUCCESS != temperature_sensor_read_temperature(TEMPERATURE_SENSOR_BASE, &read_temperature) )
            {
                status |= (1 << TEMPERATURE_SENSOR_BASE);
            }
            else
            {
                read_temperature_max = -128;
                //read_temperature >>= 8;   /* convert to full degrees */
                temperatureSensorVector[TEMPERATURE_SENSOR_BASE] = read_temperature;
                coOdPutObj_i8(I_TEMPERATURE, S_TEMPERATURE_BASE, read_temperature);
                read_temperature_max = MAX(read_temperature, read_temperature_max);
                sensorCount = 1;
            }
            break;

        /* MAIN */
        case 1:
            if(  SUCCESS != temperature_sensor_read_temperature(TEMPERATURE_SENSOR_MAIN, &read_temperature) )
            {
                status |= (1 << TEMPERATURE_SENSOR_MAIN);
            }
            else
            {
                //read_temperature >>= 8;   /* convert to full degrees */
                temperatureSensorVector[TEMPERATURE_SENSOR_MAIN] = read_temperature;
                coOdPutObj_i8(I_TEMPERATURE, S_TEMPERATURE_MAIN, read_temperature);
                read_temperature_max = MAX(read_temperature, read_temperature_max);
                sensorCount = 2;
            }
            break;

        /* MEZZANINE */
        case 2:
            if( SUCCESS != temperature_sensor_read_temperature(TEMPERATURE_SENSOR_MEZZANINE, &read_temperature) )
            {
                status |= (1 << TEMPERATURE_SENSOR_MEZZANINE);
            }
            else
            {
                //read_temperature >>= 8;   /* convert to full degrees */
                temperatureSensorVector[TEMPERATURE_SENSOR_MEZZANINE] = read_temperature;
                coOdPutObj_i8(I_TEMPERATURE, S_TEMPERATURE_MEZZANINE, read_temperature);
                read_temperature_max = MAX(read_temperature, read_temperature_max);
                sensorCount = 3;
            }
            break;

        /* connected/external energy backup */
        case 3:
            if( SUCCESS != temperature_sensor_read_temperature(TEMPERATURE_SENSOR_PWR_BANK, &read_temperature) )
            {
                status |= (1 << TEMPERATURE_SENSOR_PWR_BANK);
            }
            else
            {
                //read_temperature >>= 8;   /* convert to full degrees */
                temperatureSensorVector[TEMPERATURE_SENSOR_PWR_BANK] = read_temperature;
                coOdPutObj_i8(I_TEMPERATURE, S_TEMPERATURE_PWR_BANK, read_temperature);
                read_temperature_max = MAX(read_temperature, read_temperature_max);
                sensorCount = 0;
                temperatureHotPoint = read_temperature_max;
            }
            break;

        default:
            sensorCount = 0;
            break;

    }
    /* store maximum measured temperature */

    if ( read_temperature_max > temperature_absolute_max_limit ){
        Serial_debug(DEBUG_INFO, &cli_serial, "read_temperature_max:[%d] > temperature_absolute_max_limit:[%d]\r\n", read_temperature_max, temperature_absolute_max_limit );
        global_error_code |= (1 << ERROR_OVER_TEMPERATURE);
    } else {
        Serial_debug(DEBUG_INFO, &cli_serial, "read_temperature_max:[%d] <= temperature_absolute_max_limit:[%d]\r\n", read_temperature_max, temperature_absolute_max_limit );
        global_error_code &= ~(1 << ERROR_OVER_TEMPERATURE);
    }

//    if( read_temperature_max > temperature_high_limit ) {
//        global_error_code |= (1 << ERROR_HIGH_TEMPERATURE);
//    } else {
//        global_error_code &= ~(1 << ERROR_HIGH_TEMPERATURE);
//    }


    /* check if to high temperature */
//    if(read_temperature_max >= temperature_absolute_max_limit)
//    {   /* critical state, turn off everything and cool down */
//
//
//
//        if(!absolute_max_limit_reached_flag)
//        {
//            /* send error */
//            canopen_emcy_send_temperature_error(read_temperature_max);
//            Serial_debug(DEBUG_ERROR, &cli_serial, "DEVICE_TEMPERATURE_MAX\r\n");
//
//        }
//
//        /* mark event so we can handle it elsewhere */
//        global_error_code |= (1 << ERROR_OVER_TEMPERATURE);
//
//        /* mark it happened so we do not continuous send errors */
//        absolute_max_limit_reached_flag = true;
//
//        /* mark it false so we can send an warning if temperature reaches
//         * high_limit <= read_temperature_max < absolute_max_limit */
//        high_limit_reached = false;
//
//        /* mark it false so we can send a clear message if temperature reaches
//         * read_temperature_max < high_limit */
//        normal_reached = false;
//    } else if(read_temperature_max >= temperature_high_limit)
//    {   /* non critical state, send warning to IOP */
//        if(!high_limit_reached)
//        {
//            /* send warning */
//            canopen_emcy_send_temperature_warning(read_temperature_max);
//            Serial_debug(DEBUG_ERROR, &cli_serial, "DEVICE_TEMPERATURE_HIGH\r\n");
//        }
//
//        /* keep commented
//         * use this temperature range as hysteresis
//         * */
////        global_error_code &= ~(1 << ERROR_OVER_TEMPERATURE);
//
//        /* mark it happened so we do not continuous send warnings
//         *
//         * clear/set markings
//         * no high temperature warning
//         * max temperature error */
//        absolute_max_limit_reached_flag = false;
//        high_limit_reached = true;
//        normal_reached = false;
//    } else
//    {
//        if(!normal_reached)
//        {
//            /* send OK */
//            canopen_emcy_send_temperature_ok(read_temperature_max);
//            Serial_debug(DEBUG_ERROR, &cli_serial, "DEVICE_TEMPERATURE_OK\r\n");
//        }
//
//        /* clear markings
//         * no high temperature warning
//         * no max temperature error */
//        global_error_code &= ~(1 << ERROR_OVER_TEMPERATURE);
//        absolute_max_limit_reached_flag = false;
//        high_limit_reached = false;
//        normal_reached = true;
//    }

}


int16_t temperature_sensor_read_config_reg(uint8_t temp_sensor_number, uint16_t *readBuffer)
{
    int16_t status;
    struct I2CHandle *temperatureSensor;
    temperatureSensor = select_temperature_sensor(temp_sensor_number);

    /* set up struct */
    temperatureSensor->NumOfDataBytes = 1;
    temperatureSensor->NumOfAddrBytes = 1;
    temperatureSensor->pRX_MsgBuffer = readBuffer;
    controlAddress = TMP100_CONFIG_REG;

    status = temperature_sensor_read_register(temperatureSensor);

    return status;
}

int16_t temperature_sensor_write_config_reg(uint8_t temp_sensor_number, uint16_t *writeBuffer)
{
    int16_t status;
    struct I2CHandle *temperatureSensor;
    temperatureSensor = select_temperature_sensor(temp_sensor_number);

    /* set up struct */
    temperatureSensor->NumOfDataBytes = 1;
    temperatureSensor->NumOfAddrBytes = 1;
    temperatureSensor->pTX_MsgBuffer = writeBuffer;
    controlAddress = TMP100_CONFIG_REG;

    /* send data */
    status = temperature_sensor_write_register(temperatureSensor);

    return status;
}

int16_t temperature_sensor_read_tLow_reg(uint8_t temp_sensor_number, uint16_t *readBuffer)
{
    int16_t status;
    struct I2CHandle *temperatureSensor;
    temperatureSensor = select_temperature_sensor(temp_sensor_number);

    /* set up struct */
    temperatureSensor->NumOfDataBytes = 2;
    temperatureSensor->NumOfAddrBytes = 1;
    temperatureSensor->pRX_MsgBuffer = readBuffer;
    controlAddress = TMP100_TEMP_LOW_REG;

    status = temperature_sensor_read_register(temperatureSensor);

    return status;
}

int16_t temperature_sensor_write_tLow_reg(uint8_t temp_sensor_number, uint16_t *writeBuffer)
{
    int16_t status;
    struct I2CHandle *temperatureSensor;
    temperatureSensor = select_temperature_sensor(temp_sensor_number);

    /* set up struct */
    temperatureSensor->NumOfDataBytes = 2;
    temperatureSensor->NumOfAddrBytes = 1;
    temperatureSensor->pTX_MsgBuffer = writeBuffer;
    controlAddress = TMP100_TEMP_LOW_REG;

    /* send data */
    status = temperature_sensor_write_register(temperatureSensor);

    return status;
}

int16_t temperature_sensor_read_tHigh_reg(uint8_t temp_sensor_number, uint16_t *readBuffer)
{
    int16_t status;
    struct I2CHandle *temperatureSensor;
    temperatureSensor = select_temperature_sensor(temp_sensor_number);

    /* set up struct */
    temperatureSensor->NumOfDataBytes = 2;
    temperatureSensor->NumOfAddrBytes = 1;
    temperatureSensor->pRX_MsgBuffer = readBuffer;
    controlAddress = TMP100_TEMP_HIGH_REG;

    status = temperature_sensor_read_register(temperatureSensor);

    return status;
}

int16_t temperature_sensor_write_tHigh_reg(uint8_t temp_sensor_number, uint16_t *writeBuffer)
{
    uint16_t status;
    struct I2CHandle *temperatureSensor;
    temperatureSensor = select_temperature_sensor(temp_sensor_number);

    /* set up struct */
    temperatureSensor->NumOfDataBytes = 2;
    temperatureSensor->NumOfAddrBytes = 1;
    temperatureSensor->pTX_MsgBuffer = writeBuffer;
    controlAddress = TMP100_TEMP_HIGH_REG;

    /* send data */
    status = temperature_sensor_write_register(temperatureSensor);

    return status;
}

int16_t temperature_sensor_configure(uint8_t temp_sensor_number)
{
    int16_t return_value = SUCCESS;
    struct I2CHandle *temperatureSensor;
    temperatureSensor = select_temperature_sensor(temp_sensor_number);

    temperatureSensor->NumOfDataBytes = 1;
    temperatureSensor->NumOfAddrBytes = 1;
    temperatureSensor->pTX_MsgBuffer = TX_MsgBuffer;

    /* configure device for continuous and comparator mode, 1/4 degree C resolution */
    TX_MsgBuffer[0] = TEMPERATURE_SENSOR_CONFIG_WORD;
    controlAddress = TMP100_CONFIG_REG;
    if(I2C_MasterTransmitter(temperatureSensor) != SUCCESS)
        return_value = (uint16_t)(temp_sensor_number + 1); /* do NOT return '0' for error/failure */

    return return_value;
}


int16_t temperature_sensor_reset(uint8_t temp_sensor_number)
{
    int16_t return_value = SUCCESS;
    struct I2CHandle *temperatureSensor;
    temperatureSensor = select_temperature_sensor(temp_sensor_number);

    temperatureSensor->NumOfDataBytes = 1;
    temperatureSensor->NumOfAddrBytes = 1;
    temperatureSensor->pTX_MsgBuffer = TX_MsgBuffer;

    /* configure device for continuous and comparator mode, 1/4 degree C resolution */
    TX_MsgBuffer[0] = 0x0006;
    controlAddress = 0x00000000;
    if(I2C_MasterTransmitter(temperatureSensor) != SUCCESS)
        return_value = (uint16_t)(temp_sensor_number + 1); /* do NOT return '0' for error/failure */

    return return_value;
}

void temperature_sensor_init_individual_sensor(
        struct I2CHandle *temperatureSensor,
        uint16_t slave_address)
{
    temperatureSensor->base                 = I2CA_BASE;
    temperatureSensor->SlaveAddr            = slave_address;    // Slave address tied to the message.
    temperatureSensor->pControlAddr         = &controlAddress;  // Slave sub address (slave internal register address)
    temperatureSensor->NumOfAddrBytes       = 1;
    temperatureSensor->pTX_MsgBuffer        = TX_MsgBuffer;     // Pointer to TX message buffer
    temperatureSensor->pRX_MsgBuffer        = RX_MsgBuffer;     // Pointer to RX message buffer
    temperatureSensor->numofSixteenByte     = 0;
    temperatureSensor->WriteCycleTime_in_us = 0;                //  Slave write cycle time. Depends on slave.
                                                                //  Please check slave device datasheet

    temperatureSensor->NumOfAttempts        = 1;                //  Number of attempts to make before reporting
                                                                //  slave not ready (NACK condition)
    temperatureSensor->Delay_us             = 10;               //  Delay time in microsecs (us)
}

void readAlltemperatures(){
    int16_t status;
    int16_t readValue;
    static uint32_t lastTimeTick = 0;
    static uint16_t sensorNumber = 0;
    static bool errorI2CFlag = false;
    static int16_t maxTemperature = -10000;
    uint16_t busStatus;

    if( (timer_get_ticks() - lastTimeTick) >= TEMPERATURE_READ_INTERVAL_TIME_IN_MILISECS ) {
        lastTimeTick = timer_get_ticks();


        switch( sensorNumber ) {

            case 0:
                status = temperature_sensor_read_temperature( TEMPERATURE_SENSOR_BASE, &readValue );
                if(status == 0) {
                    temperatureSensorVector[TEMPERATURE_SENSOR_BASE] = readValue;
                    if( readValue > maxTemperature ) {
                        maxTemperature = readValue;
                    }
                } else {
                    temperature_sensor_reset(0);
                    Serial_debug(DEBUG_ERROR, &cli_serial,"Error reading TEMPERATURE_SENSOR_BASE status:=[0x%04X]\r\n", status);
                    errorI2CFlag = true;
                }
                sensorNumber = 1;
                break;

            case 1:
                status = temperature_sensor_read_temperature( TEMPERATURE_SENSOR_MAIN, &readValue );
                if(status == 0) {
                    temperatureSensorVector[TEMPERATURE_SENSOR_MAIN] = readValue;
                    if( readValue > maxTemperature ) {
                        maxTemperature = readValue;
                    }
                } else {
                    temperature_sensor_reset(0);
                    Serial_debug(DEBUG_ERROR, &cli_serial,"Error reading TEMPERATURE_SENSOR_MAIN status:=[0x%04X]\r\n", status);
                    errorI2CFlag = true;
                }
                sensorNumber = 2;
                break;

            case 2:
                status = temperature_sensor_read_temperature( TEMPERATURE_SENSOR_MEZZANINE, &readValue );
                if(status == 0) {
                    temperatureSensorVector[TEMPERATURE_SENSOR_MEZZANINE] = readValue;
                    if( readValue > maxTemperature ) {
                        maxTemperature = readValue;
                    }
                } else {
                    temperature_sensor_reset(0);
                    Serial_debug(DEBUG_ERROR, &cli_serial,"Error reading TEMPERATURE_SENSOR_MEZZANINE status:=[0x%04X]\r\n", status);
                    errorI2CFlag = true;
                }
                sensorNumber = 3;
                break;

            case 3:
                status = temperature_sensor_read_temperature( TEMPERATURE_SENSOR_PWR_BANK, &readValue );
                if(status == 0) {
                    temperatureSensorVector[TEMPERATURE_SENSOR_PWR_BANK] = readValue;
                    if( readValue > maxTemperature ) {
                        maxTemperature = readValue;
                    }
                    temperatureHotPoint = maxTemperature;
                    maxTemperature = -10000;
                    sensorNumber = 0;
                } else {
                    temperature_sensor_reset(0);
                    Serial_debug(DEBUG_ERROR, &cli_serial,"Error reading TEMPERATURE_SENSOR_PWR_BANK status:=[0x%04X]\r\n", status);
                    errorI2CFlag = true;
                }
                if( errorI2CFlag == true) {
                    sensorNumber = 4;
                }
                break;

            case 4:
                busStatus = checkBusStatus(I2C_BUS_BASE);
                if( busStatus ==  ERROR_BUS_BUSY) {
                    I2C_clearStatus(I2C_BUS_BASE, I2C_STS_STOP_CONDITION);
                    sensorNumber = 0;
                } else if( busStatus ==  ERROR_STOP_NOT_READY) {
                    I2C_clearStatus(I2C_BUS_BASE, 0xFFFF);
                    sensorNumber = 0;
                } else {
                    errorI2CFlag = false;
                    sensorNumber = 0;
                }
                break;

            default:
                sensorNumber = 0;
                break;
        }


        if( VerifyAppInfoVarInitialized( IDX_DPMU_VAR_MAX_ALLOWED_DPMU_TEMPERATURE ) == true ) {
            if ( temperatureHotPoint > temperature_absolute_max_limit ){
                error_code_CPU1 = error_code_CPU1 | (1 << ERROR_OVER_TEMPERATURE);
            } else {
                error_code_CPU1 = error_code_CPU1 & ~(1 << ERROR_OVER_TEMPERATURE);

            }
        }
    }
}
int16_t temperature_sensors_init(void)
{
    int16_t return_value = SUCCESS;

    temperature_sensor_init_individual_sensor(&temperature_sensor[TEMPERATURE_SENSOR_BASE]     , TEMPERATURE_SENSOR_BASE_ADDRESS);
    temperature_sensor_init_individual_sensor(&temperature_sensor[TEMPERATURE_SENSOR_MAIN]     , TEMPERATURE_SENSOR_MAIN_ADDRESS);
    temperature_sensor_init_individual_sensor(&temperature_sensor[TEMPERATURE_SENSOR_MEZZANINE], TEMPERATURE_SENSOR_MEZZANINE_ADDRESS);
    temperature_sensor_init_individual_sensor(&temperature_sensor[TEMPERATURE_SENSOR_PWR_BANK] , TEMPERATURE_SENSOR_PWR_BANK_ADDRESS);

    if(SUCCESS != temperature_sensor_configure(TEMPERATURE_SENSOR_BASE))
        return_value |= (1 << TEMPERATURE_SENSOR_BASE);
    if(SUCCESS != temperature_sensor_configure(TEMPERATURE_SENSOR_MAIN))
        return_value |= (1 << TEMPERATURE_SENSOR_MAIN);
    if(SUCCESS != temperature_sensor_configure(TEMPERATURE_SENSOR_MEZZANINE))
        return_value |= (1 << TEMPERATURE_SENSOR_MEZZANINE);
    if(SUCCESS != temperature_sensor_configure(TEMPERATURE_SENSOR_PWR_BANK))
        return_value |= (1 << TEMPERATURE_SENSOR_PWR_BANK);

    return return_value;
}
