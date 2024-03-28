/**---------------------------------------------------------------------------------------------------
-- Copyright (c) 2020 by Enclustra GmbH, Switzerland.
--
-- Permission is hereby granted, free of charge, to any person obtaining a copy of
-- this hardware, software, firmware, and associated documentation files (the
-- "Product"), to deal in the Product without restriction, including without
-- limitation the rights to use, copy, modify, merge, publish, distribute,
-- sublicense, and/or sell copies of the Product, and to permit persons to whom the
-- Product is furnished to do so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in all
-- copies or substantial portions of the Product.
--
-- THE PRODUCT IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
-- INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
-- PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
-- HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
-- OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
-- PRODUCT OR THE USE OR OTHER DEALINGS IN THE PRODUCT.
---------------------------------------------------------------------------------------------------
*/

//-------------------------------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------------------------------

#include <cli_cpu1.h>
#include <stdlib.h>

#include "defines.h"
#include "i2c_com.h"
#include "main.h"
#include "serial.h"
#include "temperature_sensor.h"

//-------------------------------------------------------------------------------------------------
// File scope variables
//-------------------------------------------------------------------------------------------------

/// A little bit of left padding for when we're printing strings.
char LEFT_PADDING[] = "   ";


//-------------------------------------------------------------------------------------------------
// Function definitions
//-------------------------------------------------------------------------------------------------
	
//int ReadRealtimeClock(int debug_print)
//{
//	int status = XST_SUCCESS;
//
//	if(debug_print){
//		debug_print(VAB2_DEBUG, "\n\rReal Time Clock:\n\r");
//	}
//
//    // Set the time and date, then wait a bit.
//    if(RTC_SetTime(11, 22, 32) != XST_SUCCESS)
//		status = XST_FAILURE;
//    if(RTC_SetDate(22, 11, 10) != XST_SUCCESS)
//		status = XST_FAILURE;
//    sleep(1);
//
//    // Read back the time.
//    int hour, minute, seconds;
//    if(RTC_ReadTime((int*)&hour, (int*)&minute, (int*)&seconds) != XST_SUCCESS)
//		status = XST_FAILURE;
//
//    // Read back the date.
//    int year, month, day;
//    if(RTC_ReadDate((int*)&day, (int*)&month, (int*)&year) != XST_SUCCESS)
//		status = XST_FAILURE;
//
//    // Print time and date
//    debug_print(VAB2_DEBUG, "%sTime: %02d:%02d:%02d\n\r", LEFT_PADDING, hour, minute, seconds);
//    debug_print(VAB2_DEBUG, "%sDate: %02d.%02d.%02d\n\r", LEFT_PADDING, year, day, month);
//
//    /* chech update of time */
//    if(hour    != 11)
//    	status = XST_FAILURE;
//    if(minute  != 22)
//    	status = XST_FAILURE;
//    if(seconds < (33-1) || seconds > (33+1))
//    	status = XST_FAILURE;
//
//    /* chech update of date */
//    if(day     != 22)
//    	status = XST_FAILURE;
//    if(month   != 11)
//    	status = XST_FAILURE;
//    if(year    != 10)
//    	status = XST_FAILURE;
//
//    return status;
//
//}
//
//int ReadRtcReadTemperature() {
//	float TemperatureCelsius;
//	int status = XST_SUCCESS;
//
//	if(RTC_ReadTemperature(&TemperatureCelsius) != XST_SUCCESS) {
//		debug_print(VAB2_DEBUG, "Temperature %d\n\r", TemperatureCelsius);
//		status = XST_SUCCESS;
//	}
//
//	return status;
//}

///*****************************************************************************
// *
//* @brief
//* This function test write and read to UseerEEPROM on the test card
//* It does it using a polled mode send in master mode.
//*
//* The function writes the ASCII table starting at 0x30 ('0').
//*
//* @param    offset offset to change the starting value in the WriteBuffer
//*
//* @return
//*       - XST_SUCCESS if everything went well.
//*       - XST_FAILURE if error.
//*
//* @note     The functions uses the offset to change starting value in the ASCII
//*           table. It does this so it is possible to change the values in the
//*           EEPROM to make sure you are reading the latest written value and not
//*           from an earlier attempt to write.
//*
//*****************************************************************************/
//
//#define MAX_SIZE        64
//#define PAGE_SIZE_16    16
//#define PAGE_SIZE_32    32
//#define PAGE_SIZE_64    64
//
////#define __I2C_TEST_WRITE_SRAM_LENGTH MAX_SIZE
//#define __I2C_TEST_WRITE_SRAM_LENGTH (8-1)
//
//int i2c_test_of_write_and_of_RTC_SRAM(uint8_t offset, int debug_print) {
//    u8 WriteBuffer[MAX_SIZE];   /* Write buffer for writing a page. */
//    u8 ReadBuffer[MAX_SIZE];    /* Read  buffer for reading a page. */
//
//    u32 Index;
//    u32 Status = XST_FAILURE;
//
//	if(debug_print) {
//		xil_printf( "\r\nTest of write to RTC SRAM\r\n" );
//	}
//
//    /* initialize buffers */
//    for (Index = 0; Index < __I2C_TEST_WRITE_SRAM_LENGTH; Index++) {
//        WriteBuffer[Index] = 0x30 + Index + offset;
//        ReadBuffer[Index] = 0;
//    }
//
//    Status = RTC_SRAM_i2c_write(0, WriteBuffer, __I2C_TEST_WRITE_SRAM_LENGTH);
//    if(Status != XST_SUCCESS) {
//        xil_printf("I2CWrite Failed!");
//    }
//    else {
//    	if(debug_print) {
//			xil_printf("Written: ");
//			for (int i = 0; i < __I2C_TEST_WRITE_SRAM_LENGTH; i++) {
//				xil_printf("%c ", WriteBuffer[i]);
//			}
//    	}
//    }
//
//    Status = RTC_SRAM_i2c_read(0, ReadBuffer, __I2C_TEST_WRITE_SRAM_LENGTH);
//    if(Status != XST_SUCCESS) {
//        xil_printf("I2CRead Failed!");
//    }
//    else {
//    	if(debug_print) {
//			xil_printf("\nRead    : ");
//			for (int i = 0; i < __I2C_TEST_WRITE_SRAM_LENGTH; i++) {
//				char c = ReadBuffer[i];
//				xil_printf("%c ", c);
//	//          xil_printf("%c ", ReadBuffer[i]);
//			}
//    	}
//    }
//
//    /*
//     * Verify the data read against the data written.
//     */
//    for (Index = 0; Index < __I2C_TEST_WRITE_SRAM_LENGTH; Index++) {
//        if (ReadBuffer[Index] != WriteBuffer[Index]) {
//            Status = XST_FAILURE;
//        	if(debug_print) {
//				xil_printf("\n Index %d  WriteBuffer[%d] = %d  ReadBuffer[%d] = %d", \
//						Index, Index, WriteBuffer[Index], Index, ReadBuffer[Index]);
//				break;
//        	}
//        }
//    }
//    if (Status == XST_SUCCESS) {
//    	if(debug_print) {
//    		xil_printf("\nI2C Write / Read SUCCESS");
//    	}
//    }
//
//    return Status;
//}

int compare_buffers(uint16_t *buffer1, uint16_t *buffer2, uint16_t nrOfBytes)
{
    for(int i = 0; i < nrOfBytes; i++)
    {
        if(buffer1[i] != buffer2[i])
           return 1;
    }

    return 0;
}

static int i2c_test_of_temperature_sensor_config_reg(uint8_t temp_sensor_number)
{
    int16_t status = SUCCESS;
    uint16_t readBuffer[1]  = {0};
    uint16_t writeBuffer[1] = {0};

    /* write one */
    if((status = temperature_sensor_write_config_reg(temp_sensor_number, writeBuffer)))
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - Could not write\r\n");
        return status;
    }
    Serial_debug(DEBUG_INFO, &cli_serial, "Wrote Config reg with: 0x%02x\r\n", writeBuffer[0]);

    /* read one */
    if((status = temperature_sensor_read_config_reg(temp_sensor_number, readBuffer)))
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - Could not read\r\n");
        return status;
    }
    Serial_debug(DEBUG_INFO, &cli_serial, "Read  Config reg with: 0x%02x\r\n", readBuffer[0]);

    /* mask out bit that is always one for first read after power cycle */
    readBuffer[0] &= 0x7f;

    if(compare_buffers(readBuffer, writeBuffer, 1))
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - Buffers not equal (readBuffer != readBuffer)\r\n");
        return status;
    }

    /* write two */
    writeBuffer[0] = 0b00111000; /* 0x38, 0d56 */
    if((status = temperature_sensor_write_config_reg(temp_sensor_number, writeBuffer)))
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - Could not write\r\n");
        return status;
    }
    Serial_debug(DEBUG_INFO, &cli_serial, "Wrote Config reg with: 0x%02x\r\n", writeBuffer[0]);


    /* read two */
    if((status = temperature_sensor_read_config_reg(temp_sensor_number, readBuffer)))
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - Could not read\r\n");
        return status;
    }
    Serial_debug(DEBUG_INFO, &cli_serial, "Read  Config reg with: 0x%02x\r\n", readBuffer[0]);

    /* mask out bit that is always one for first read after power cycle */
    readBuffer[0] &= 0x7f;

    if(compare_buffers(readBuffer, writeBuffer, 1))
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - Buffers not equal (readBuffer != readBuffer)\r\n");
        return status;
    }

    return status;
}

int i2c_test_of_temperature_sensor_tLow_reg(uint8_t temp_sensor_number)
{
    int16_t status = SUCCESS;
    uint16_t readBuffer[2]  = {0, 0};
    uint16_t writeBuffer[2] = {0, 0};

    /* write one */
    if((status = temperature_sensor_write_tLow_reg(temp_sensor_number, writeBuffer)))
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - Could not write\r\n");
        return status;
    }
    Serial_debug(DEBUG_INFO, &cli_serial, "Wrote tLow reg with: 0x%02x 0x%02x\r\n", writeBuffer[0], writeBuffer[1]);

    /* read one */
    if((status = temperature_sensor_read_tLow_reg(temp_sensor_number, readBuffer)))
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - Could not read\r\n");
        return status;
    }
    Serial_debug(DEBUG_INFO, &cli_serial, "Read  tLow reg with: 0x%02x 0x%02x\r\n", readBuffer[0], readBuffer[1]);

    if(compare_buffers(readBuffer, writeBuffer, 2))
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - Buffers not equal (readBuffer != readBuffer)\r\n");
        return status;
    }

    /* write two */
    writeBuffer[0] = 0x04; /* 75 degree Celsius */
    writeBuffer[1] = 0xB0;
    if((status = temperature_sensor_write_tLow_reg(temp_sensor_number, writeBuffer)))
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - Could not write\r\n");
        return status;
    }
    Serial_debug(DEBUG_INFO, &cli_serial, "Wrote tLow reg with: 0x%02x 0x%02x\r\n", writeBuffer[0], writeBuffer[1]);


    /* read two */
    if((status = temperature_sensor_read_tLow_reg(temp_sensor_number, readBuffer)))
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - Could not read\r\n");
        return status;
    }
    Serial_debug(DEBUG_INFO, &cli_serial, "Read  tLow reg with: 0x%02x 0x%02x\r\n", readBuffer[0], readBuffer[1]);

    if(compare_buffers(readBuffer, writeBuffer, 2))
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - Buffers not equal (readBuffer != readBuffer)\r\n");
        return status;
    }

    return status;
}

int i2c_test_of_temperature_sensor_tHigh_reg(uint8_t temp_sensor_number)
{
    int16_t status = SUCCESS;
    uint16_t readBuffer[2]  = {0, 0};
    uint16_t writeBuffer[2] = {0, 0};

    /* write one */
    if((status = temperature_sensor_write_tHigh_reg(temp_sensor_number, writeBuffer)))
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - Could not write\r\n");
        return status;
    }
    Serial_debug(DEBUG_INFO, &cli_serial, "Wrote tHigh reg with: 0x%02x 0x%02x\r\n", writeBuffer[0], writeBuffer[1]);

    /* read one */
    if((status = temperature_sensor_read_tHigh_reg(temp_sensor_number, readBuffer)))
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - Could not read\r\n");
        return status;
    }
    Serial_debug(DEBUG_INFO, &cli_serial, "Read  tHigh reg with: 0x%02x 0x%02x\r\n", readBuffer[0], readBuffer[1]);

    if(compare_buffers(readBuffer, writeBuffer, 2))
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - Buffers not equal (readBuffer != readBuffer)\r\n");
        return status;
    }

    /* write two */
    writeBuffer[0] = 0x05; /* 80 degree Celsius */
    writeBuffer[1] = 0x00;
    if((status = temperature_sensor_write_tHigh_reg(temp_sensor_number, writeBuffer)))
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - Could not write\r\n");
        return status;
    }
    Serial_debug(DEBUG_INFO, &cli_serial, "Wrote tHigh reg with: 0x%02x 0x%02x\r\n", writeBuffer[0], writeBuffer[1]);


    /* read two */
    if((status = temperature_sensor_read_tHigh_reg(temp_sensor_number, readBuffer)))
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - Could not read\r\n");
        return status;
    }
    Serial_debug(DEBUG_INFO, &cli_serial, "Read  tHigh reg with: 0x%02x 0x%02x\r\n", readBuffer[0], readBuffer[1]);

    if(compare_buffers(readBuffer, writeBuffer, 2))
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - Buffers not equal (readBuffer != readBuffer)\r\n");
        return status;
    }

    return status;
}

int i2c_test_of_temperature_sensor(uint8_t temp_sensor_number)
{
    int16_t status = SUCCESS;
    int16_t read_temperature_value = 0;
//	float temperature;

	/* test write to temperature sensor configuration registers */
    status |= i2c_test_of_temperature_sensor_config_reg(temp_sensor_number);
    status |= i2c_test_of_temperature_sensor_tLow_reg(temp_sensor_number);
    status |= i2c_test_of_temperature_sensor_tHigh_reg(temp_sensor_number);

    /* read temperature sensor measured temperature */
	if((status = temperature_sensor_read_temperature(temp_sensor_number, &read_temperature_value)))
	{
	    Serial_debug(DEBUG_ERROR, &cli_serial, "ERROR - Could not test temperature sensor %x\r\n", temp_sensor_number);
	} else
	{
//	    temperature = read_temperature_value / 4;
	    Serial_debug(DEBUG_INFO, &cli_serial, "OK - Could test temperature sensor %x: \r\n", temp_sensor_number, read_temperature_value);
	}

    return status;
}

int i2c_test_of_temperature_sensors(void)
{
    int16_t status = SUCCESS;

    Serial_debug(DEBUG_INFO, &cli_serial, "Test of temperature sensor TEMPERATURE_SENSOR_BASE\r\n");
    if(SUCCESS != i2c_test_of_temperature_sensor(TEMPERATURE_SENSOR_BASE)) {
        status |= (1 << TEMPERATURE_SENSOR_BASE);
        Serial_debug(DEBUG_ERROR, &cli_serial, "Could not test temperature sensor TEMPERATURE_SENSOR_BASE\r\n");
    } else {
        Serial_debug(DEBUG_INFO, &cli_serial, "OK - Temperature sensor TEMPERATURE_SENSOR_BASE\r\n");
    }

    Serial_debug(DEBUG_INFO, &cli_serial, "\r\nTest of temperature sensor TEMPERATURE_SENSOR_MAIN\r\n");
    if(SUCCESS != i2c_test_of_temperature_sensor(TEMPERATURE_SENSOR_MAIN)) {
        status |= (1 << TEMPERATURE_SENSOR_MAIN);
        Serial_debug(DEBUG_ERROR, &cli_serial, "Could not test temperature sensor TEMPERATURE_SENSOR_MAIN\r\n");
    } else {
        Serial_debug(DEBUG_INFO, &cli_serial, "OK - Temperature sensor TEMPERATURE_SENSOR_MAIN\r\n");
    }

    Serial_debug(DEBUG_INFO, &cli_serial, "\r\nTest of temperature sensor TEMPERATURE_SENSOR_MEZZANINE\r\n");
    if(SUCCESS != i2c_test_of_temperature_sensor(TEMPERATURE_SENSOR_MEZZANINE)) {
        status |= (1 << TEMPERATURE_SENSOR_MEZZANINE);
        Serial_debug(DEBUG_INFO, &cli_serial, "Could not test temperature sensor TEMPERATURE_SENSOR_MEZZANINE\r\n");
    } else {
        Serial_debug(DEBUG_INFO, &cli_serial, "OK - Temperature sensor TEMPERATURE_SENSOR_MEZZANINE\r\n");
    }

    Serial_debug(DEBUG_INFO, &cli_serial, "\r\nTest of temperature sensor TEMPERATURE_SENSOR_PWR_BANK\r\n");
    if(SUCCESS != i2c_test_of_temperature_sensor(TEMPERATURE_SENSOR_PWR_BANK)) {
        status |= (1 << TEMPERATURE_SENSOR_PWR_BANK);
        Serial_debug(DEBUG_ERROR, &cli_serial, "Could not test temperature sensor TEMPERATURE_SENSOR_PWR_BANK\r\n");
    } else {
        Serial_debug(DEBUG_INFO, &cli_serial, "OK - Temperature sensor TEMPERATURE_SENSOR_PWR_BANK\r\n");
    }

    if(status == SUCCESS)
    {
        Serial_debug(DEBUG_INFO, &cli_serial, "\r\nOK - Could test all temperature sensor\r\n");
    } else
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "\r\nERROR - Could not test all temperature sensor\r\n");
    }

    /* for machine reading */
    Serial_printf(&cli_serial, "%02X\n\r", status);

    return status;
}

int i2c_test_main(void)
{
    int16_t status = SUCCESS;
    static uint16_t count = 0;

    if(count > 0) status = SUCCESS;

    Serial_debug(DEBUG_INFO, &cli_serial, "\n\r == DPMU I2C test ==  %d\n\r", ++count);
//    Serial_printf(&cli_serial, "\n\r == DPMU I2C test ==  %d\n\r", ++count);
//
//    if (ReadRealtimeClock(0) != STATUS_S_SUCCESS)
//    {
//        debug_print(VAB2_DEBUG, "Error: Real-time clock read failed\n\r");
//        Status |= STATUS_E_FAILURE;
//    }
//    else {
//        debug_print(VAB2_DEBUG, "OK - Real-time clock\n\r");
//    }
//
//    if (ReadRtcReadTemperature() != STATUS_S_SUCCESS)
//    {
//        debug_print(VAB2_DEBUG, "Error: Real-time clock Temperature read failed\n\r");
//        Status |= STATUS_E_FAILURE;
//    }
//    else {
//        debug_print(VAB2_DEBUG, "OK - Real-time clock temperature reading\n\r");
//    }

    status = i2c_test_of_temperature_sensors();
    if(status != SUCCESS)
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "Error: Temperature sensors test failed\n\r");
    } else
    {
        Serial_debug(DEBUG_INFO, &cli_serial, "OK - Temperature sensors test\n\r");
    }

    if(status != SUCCESS)
    {
        Serial_debug(DEBUG_ERROR, &cli_serial, "\n\rERROR == End of test ==  %d\n\r", count);
    } else
    {
        Serial_debug(DEBUG_INFO, &cli_serial, "\n\rOK    == End of test ==  %d\n\r", count);
    }

    /* for machine reading */
    Serial_printf(&cli_serial, "%02X\n\r", status);

    return status;
}
