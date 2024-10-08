/*
 *  CO index/J1939 PGN/raw CAN-ID  defines for DPMU_001 - generated by CANopen DeviceDesigner 3.8.0
 * tis jan 30 13:14:10 2024
 */

/* protect against multiple inclusion of the file */
#ifndef GEN_INDICES_H
#define GEN_INDICES_H 1

#define I_DEVICE_TYPE            	0x1000u
#define I_ERROR_REGISTER         	0x1001u
#define I_PREDEFINED_ERROR_FIELD 	0x1003u
#define  S_ERROR_CODE             	0x1u
#define I_MANUFACTURER_DEVICE_NAME	0x1008u
#define I_STORE_PARAMETERS       	0x1010u
#define  S_SAVE_ALL_PARAMETERS    	0x1u
#define  S_SAVE_COMMUNICATION_PARAMETERS	0x2u
#define  S_SAVE_APPLICATION_PARAMETERS	0x3u
#define  S_SAVE_MANUFACTURER_DEFINED_PARAMETERS	0x4u
#define  S_SAVE_SERIAL_NUMBER   0x5u
#define I_RESTORE_DEFAULT_PARAMETERS	0x1011u
#define  S_RESTORE_ALL_DEFAULT_PARAMETERS	0x1u
#define  S_RESTORE_COMMUNICATION_DEFAULT_PARAMETERS	0x2u
#define  S_RESTORE_APPLICATION_DEFAULT_PARAMETERS	0x3u
#define  S_RESTORE_MANUFACTURER_DEFINED_DEFAULT_PARAMETERS	0x4u
#define  S_RESTORE_SERIAL_NUMBER   0x5u
#define I_COB_ID_EMCY            	0x1014u
#define I_INHIBIT_TIME_EMERGENCY 	0x1015u
#define I_CONSUMER_HEARTBEAT_TIME	0x1016u
#define  S_CONSUMER_HEARTBEAT_TIME	0x1u
#define I_PRODUCER_HEARTBEAT_TIME	0x1017u
#define I_IDENTITY_OBJECT        	0x1018u
#define  S_VENDOR_ID              	0x1u
#define  S_PRODUCT_CODE           	0x2u
#define  S_REVISION_NUMBER        	0x3u
#define  S_SERIAL_NUMBER          	0x4u
#define I_ERROR_BEHAVIOUR        	0x1029u
#define  S_COMMUNICATION_ERROR    	0x1u
#define  S_SPECIFIC_ERROR_CLASS_  	0x2u
#define I_SERVER_SDO_PARAMETER   	0x1200u
#define  S_COB_ID_CLIENT_TO_SERVER	0x1u
#define  S_COB_ID_SERVER_TO_CLIENT	0x2u
#define I_RECEIVE_PDO_COMMUNICATION_PARAMETER	0x1400u
#define  S_COB_ID                 	0x1u
#define  S_TRANSMISSION_TYPE      	0x2u
#define  S_INHIBIT_TIME           	0x3u
#define  S_COMPATIBILITY_ENTRY    	0x4u
#define  S_EVENT_TIMER            	0x5u
#define I_RECEIVE_PDO_COMMUNICATION_PARAMETER1	0x1401u
#define I_RECEIVE_PDO_MAPPING_PARAMETER	0x1600u
#define  S_PDO_MAPPING_ENTRY      	0x1u
#define I_RECEIVE_PDO_MAPPING_PARAMETER1	0x1601u
#define  S_MAPPING_ENTRY_1        	0x1u
#define  S_MAPPING_ENTRY_2        	0x2u
#define  S_MAPPING_ENTRY_3        	0x3u
#define  S_MAPPING_ENTRY_4        	0x4u
#define I_TRANSMIT_PDO_IO        	0x1800u
#define I_TRANSMIT_PDO_TEMPERATURES	0x1801u
#define I_TRANSMIT_PDO_DC_BUS    	0x1802u
#define I_TRANSMIT_PDO_ENERGY_BANK	0x1803u
#define I_TPDO_MAPPING_IO        	0x1a00u
#define  S_STATE_OF_SWITCHES_PDO  	0x1u
#define I_TPDO_MAPPING_TEMPERATURES	0x1a01u
#define  S_TEMPERATURE_MEASURED_AT_DPMU_HOTTEST_POINT_PDO	0x1u
#define  S_TEMPERATURE_BASE_PDO   	0x2u
#define  S_TEMPERATURE_MAIN_PDO   	0x3u
#define  S_TEMPERATURE_MEZZANINE_PDO	0x4u
#define  S_TEMPERATURE_PWR_BANK_PDO	0x5u
#define I_TPDO_MAPPING_DC_BUS    	0x1a02u
#define  S_READ_VOLTAGE_AT_DC_BUS_PDO	0x1u
#define  S_POWER_FROM_DC_INPUT_PDO	0x2u
#define  S_READ_LOAD_CURRENT_PDO  	0x3u
#define  S_POWER_CONSUMED_BY_LOAD_PDO	0x4u
#define I_TPDO_MAPPING_ENERGY_BANK	0x1a03u
#define  S_STATE_OF_CHARGE_OF_ENERGY_BANK_PDO	0x1u
#define  S_STATE_OF_HEALTH_OF_ENERGY_BANK_PDO	0x2u
#define  S_REMAINING_ENERGY_TO_MIN_SOC_AT_ENERGY_BANK_PDO	0x3u
#define  S_STACK_TEMPERATURE_PDO  	0x4u
#define I_PROGRAM_CONTROL        	0x1f51u
#define  S_PROGRAM_CONTROL_1      	0x1u
#define I_DATE_AND_TIME          	0x2000u
#define I_MANUFACTURER_OBJECT    	0x2001u
#define I_SET_NODEID             	0x2002u
#define I_DC_BUS_VOLTAGE         	0x3000u
#define  S_MIN_ALLOWED_DC_BUS_VOLTAGE	0x1u
#define  S_MAX_ALLOWED_DC_BUS_VOLTAGE	0x2u
#define  S_TARGET_VOLTAGE_AT_DC_BUS	0x3u
#define  S_VDC_BUS_SHORT_CIRCUIT_LIMIT	0x4u
#define  S_VDROOP                 	0x5u
#define I_ESS_CURRENT            	0x3001u
#define I_ENERGY_CELL_SUMMARY    	0x3100u
#define  S_MIN_VOLTAGE_ENERGY_CELL	0x1u
#define  S_MAX_VOLTAGE_ENERGY_CELL	0x2u
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_01	0x3u
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_02	0x4u
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_03	0x5u
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_04	0x6u
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_05	0x7u
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_06	0x8u
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_07	0x9u
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_08	0xau
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_09	0xbu
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_10	0xcu
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_11	0xdu
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_12	0xeu
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_13	0xfu
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_14	0x10u
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_15	0x11u
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_16	0x12u
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_17	0x13u
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_18	0x14u
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_19	0x15u
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_20	0x16u
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_21	0x17u
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_22	0x18u
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_23	0x19u
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_24	0x1au
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_25	0x1bu
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_26	0x1cu
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_27	0x1du
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_28	0x1eu
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_29	0x1fu
#define  S_STATE_OF_CHARGE_OF_ENERGY_CELL_30	0x20u
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_01	0x21u
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_02	0x22u
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_03	0x23u
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_04	0x24u
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_05	0x25u
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_06	0x26u
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_07	0x27u
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_08	0x28u
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_09	0x29u
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_10	0x2au
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_11	0x2bu
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_12	0x2cu
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_13	0x2du
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_14	0x2eu
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_15	0x2fu
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_16	0x30u
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_17	0x31u
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_18	0x32u
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_19	0x33u
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_20	0x34u
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_21	0x35u
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_22	0x36u
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_23	0x37u
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_24	0x38u
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_25	0x39u
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_26	0x3au
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_27	0x3bu
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_28	0x3cu
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_29	0x3du
#define  S_STATE_OF_HEALTH_OF_ENERGY_CELL_30	0x3eu
#define I_TEMPERATURE            	0x3200u
#define  S_DPMU_TEMPERATURE_MAX_LIMIT	0x1u
#define  S_TEMPERATURE_MEASURED_AT_DPMU_HOTTEST_POINT	0x2u
#define  S_DPMU_TEMPERATURE_HIGH_LIMIT	0x3u
#define  S_TEMPERATURE_BASE       	0x4u
#define  S_TEMPERATURE_MAIN       	0x5u
#define  S_TEMPERATURE_MEZZANINE  	0x6u
#define  S_TEMPERATURE_PWR_BANK   	0x7u
#define I_MAXIMUM_ALLOWED_LOAD_POWER	0x3301u
#define I_POWER_BUDGET_DC_INPUT  	0x3302u
#define  S_AVAILABLE_POWER_BUDGET_DC_INPUT	0x1u
#define I_CHARGE_FACTOR          	0x3303u
#define I_READ_POWER             	0x3304u
#define  S_READ_VOLTAGE_AT_DC_BUS 	0x1u
#define  S_POWER_FROM_DC_INPUT    	0x2u
#define  S_READ_LOAD_CURRENT      	0x3u
#define  S_POWER_CONSUMED_BY_LOAD 	0x4u
#define I_DPMU_STATE             	0x3305u
#define  S_DPMU_OPERATION_REQUEST_STATE	0x1u
#define  S_DPMU_OPERATION_CURRENT_STATE	0x2u
#define I_ENERGY_BANK_SUMMARY    	0x3400u
#define  S_MAX_VOLTAGE_APPLIED_TO_ENERGY_BANK	0x1u
#define  S_MIN_VOLTAGE_APPLIED_TO_ENERGY_BANK	0x2u
#define  S_SAFETY_THRESHOLD_STATE_OF_CHARGE	0x4u
#define  S_STATE_OF_CHARGE_OF_ENERGY_BANK	0x5u
#define  S_STATE_OF_HEALTH_OF_ENERGY_BANK	0x6u
#define  S_REMAINING_ENERGY_TO_MIN_SOC_AT_ENERGY_BANK	0x7u
#define  S_STACK_TEMPERATURE      	0x9u
#define  S_CONSTANT_VOLTAGE_THRESHOLD	0xau
#define  S_PRECONDITIONAL_THRESHOLD	0xbu
#define I_SWITCH_STATE           	0x4000u
#define  S_SW_QINRUSH_STATE       	0x1u
#define  S_SW_QLB_STATE           	0x2u
#define  S_SW_QSB_STATE           	0x3u
#define  S_SW_QINB_STATE          	0x4u
#define I_DPMU_POWER_SOURCE_TYPE 	0x4001u
#define I_SEND_REBOOT_REQUEST    	0x4002u
#define I_OPERATIONAL_ERROR      	0x4003u
#define I_DEBUG_LOG              	0x4010u
#define  S_DEBUG_LOG_STATE        	0x1u
#define  S_DEBUG_LOG_READ         	0x2u
#define  S_DEBUG_LOG_RESET        	0x3u
#define I_CAN_LOG                	0x4011u
#define  S_CAN_LOG_READ           	0x1u
#define  S_CAN_LOG_RESET          	0x2u
#define I_IO                     	0x6000u
#define  S_STATE_OF_SWITCHES      	0x1u
#define I_POLARITY_INPUT_8_BIT   	0x6002u
#define  S_POLARITY8              	0x1u
#define  S_POLARITY81             	0x2u
#define  S_POLARITY82             	0x3u
#define I_GLOBAL_INTERRUPT_ENABLE_DIGITAL_8BIT	0x6005u
#define I_INTERRUPT_MASK_ANY_CHANGE_8_BIT	0x6006u
#define  S_INTERRUPTANYCHANGE8    	0x1u
#define  S_INTERRUPTANYCHANGE81   	0x2u
#define  S_INTERRUPTANYCHANGE82   	0x3u
#define I_INTERRUPT_MASK_LOW_TO_HIGH_8_BIT	0x6007u
#define  S_INTERRUPTLOWTOHIGH8    	0x1u
#define  S_INTERRUPTLOWTOHIGH81   	0x2u
#define  S_INTERRUPTLOWTOHIGH82   	0x3u
#define I_INTERRUPT_MASK_HIGH_TO_LOW_8_BIT	0x6008u
#define  S_INTERRUPTHIGHTOLOW8    	0x1u
#define  S_INTERRUPTHIGHTOLOW81   	0x2u
#define  S_INTERRUPTHIGHTOLOW82   	0x3u
#define I_WRITE_OUTPUT_8_BIT     	0x6200u
#define  S_DIGOUTPUT8             	0x1u
#define  S_DIGOUTPUT81            	0x2u
#define  S_DIGOUTPUT82            	0x3u
#define  S_DIGOUTPUT83            	0x4u
#define  S_DIGOUTPUT84            	0x5u
#define  S_DIGOUTPUT85            	0x6u
#define I_POLARITY_OUTPUT_8_BIT  	0x6202u
#define  S_POLARITY83             	0x1u
#define I_ADC_PORT_VALUE         	0x6401u
#define  S_READ_LOAD_CURRENT1     	0x1u
#define  S_READ_VOLTAGE_AT_DC_BUS1	0x3u
#define  S_DPMU_TEMPERATURE       	0x4u
#define I_WRITE_ANALOGUE_OUTPUT_16_BIT	0x6411u
#define  S_ANALOGUEOUTPUT16       	0x1u
#define  S_ANALOGUEOUTPUT161      	0x2u
#define  S_ANALOGUEOUTPUT162      	0x3u
#define  S_ANALOGUEOUTPUT163      	0x4u
#define  S_ANALOGUEOUTPUT164      	0x5u
#define  S_ANALOGUEOUTPUT165      	0x6u
#define  S_ANALOGUEOUTPUT166      	0x7u
#define  S_ANALOGUEOUTPUT167      	0x8u
#define I_INTERRUPT_TRIGGER_SELECTION	0x6421u
#define  S_INTERRUPTTRIGGERSELECTION	0x1u
#define I_ANALOG_INPUT_GLOBAL_INTERRUPT_ENABLE	0x6423u
#define I_ANALOGUE_INPUT_INTERRUPT_UPPER_LIMIT_INTEGER	0x6424u
#define  S_LOAD_OVER_CURRENT_MAX_LIMIT	0x1u
#define  S_DC_BUS_VOLTAGE_MAX_LIMIT	0x3u
#define  S_DPMU_TEMPERATURE_MAX_LIMIT1	0x4u
#define I_ANALOGUE_INPUT_INTERRUPT_LOWER_LIMIT_INTEGER	0x6425u
#define  S_LOAD_OVER_CURRENT_MIN_LIMIT	0x1u
#define  S_DC_BUS_VOLTAGE_MIN_LIMIT	0x3u
#define  S_DPMU_TEMPERATURE_MIN_LIMIT	0x4u
#define  S_NUMBER_OF_ENTRIES      	0x0u

#endif /* GEN_INDICES_H */
