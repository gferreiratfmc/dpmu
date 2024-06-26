/*
 * Stack definitions for DPMU_001 - generated by CANopen DeviceDesigner 3.8.0
 * tis jan 30 13:14:11 2024
 */

/* protect against multiple inclusion of the file */
#ifndef GEN_DEFINE_H
#define GEN_DEFINE_H 1

/* some information about the tool that has generated this file */
#define GEN_TOOL_NAME	CANOPEN_DEVICEDESIGNER
#define GEN_TOOL_VERSION	0x030800ul

/* Line Settings */
#define CANOPEN_SUPPORTED	1u
/* Number of receive/transmit buffer entries per line */
#define CO_REC_BUFFER_COUNTS	10u
#define CO_TR_BUFFER_COUNTS	10u
/* Number of objects per line */
#define CO_OBJECTS_LINE_0_CNT	59u
#define CO_OBJECT_COUNTS	59u
#define CO_COB_COUNTS	12u
#define CO_TXPDO_COUNTS	4u
#define CO_RXPDO_COUNTS	2u
#define CO_SSDO_COUNTS	1u
#define CO_CSDO_COUNTS	0u
#define CO_ASSIGN_COUNTS	0u
#define CO_MAX_ASSIGN_COUNTS	0u
#define CO_GUARDING_COUNTS	CO_MAX_ASSIGN_COUNTS
#define CO_ERR_CTRL_COUNTS	1u
#define CO_ERR_HIST_COUNTS	10u
#define CO_ACT_ERR_HIST_COUNTS	0u
#define CO_EMCY_CONS_COUNTS	0u
#define CO_NODE_IDS	1
#define CO_NODE_ID_FUNCTIONS	node_set_nodeID


/* Definition of numbers of CANopen services */
#define CO_PROFILE_401	1u
#define CO_SDO_SERVER_CNT	1u
#define CO_PDO_TRANSMIT_CNT	4u
#define CO_PDO_RECEIVE_CNT	2u
#define CO_MAX_MAP_ENTRIES	5u
#define CO_TR_PDO_DYN_MAP_ENTRIES	4u
#define CO_REC_PDO_DYN_MAP_ENTRIES	1u
#define CO_PDO_BIT_MAPPING	1u
#define CO_HB_CONSUMER_CNT 1u
#define CO_EMCY_PRODUCER	1u
#define CO_STORE_SUPPORTED	1u
#define CO_STORE_NVS_CNT	0u
#define CO_EMCY_ERROR_HISTORY 10u
#define CO_SDO_BLOCK		1u
#define CO_SDO_BLOCK_SIZE	4u
#define CO_SDO_BLOCK_MIN_SIZE	4u
#define CO_SSDO_DOMAIN_CNT	2u
#define CO_INHIBIT_SUPPORTED	1u
/* number of used COB objects */
#define CO_COB_CNT	12u


/* Definition of number of call-back functions for each service*/
#define CO_EVENT_DYNAMIC_SDO_SERVER_READ	1u
#define CO_EVENT_DYNAMIC_SDO_SERVER_WRITE	1u
#define CO_EVENT_DYNAMIC_SDO_SERVER_CHECK_WRITE	1u
#define CO_EVENT_DYNAMIC_PDO	1u
#define CO_EVENT_DYNAMIC_NMT	1u
#define CO_EVENT_DYNAMIC_ERRCTRL	1u
#define CO_EVENT_DYNAMIC_LED	1u
#define CO_EVENT_DYNAMIC_CAN	1u
#define CO_EVENT_DYNAMIC_EMCY	1u
#define CO_EVENT_DYNAMIC_SSDO_DOMAIN_READ	1u

#define CO_ONE_HB_CONSUMER_COB	1u
/* Definition of CAN queue sizes */
#define CO_CONFIG_REC_BUFFER_CNT	10u
#define CO_CONFIG_TRANS_BUFFER_CNT	10u

/* Hardware settings */
/*  */

/* application-specific defines as defined in DeviceDesigner */
#define CO_TIMER_INTERVAL	10000u
#define CO_DRV_FILTER	1u
#define CO_DRV_GROUP_FILTER	1u

/* #define NO_PRINTF 1u */

#define  CO_CPU_DSP 1u
#define CO_CPU_DSP_BYTESIZE  2u
#define NO_PRINTF    1u

#ifdef NO_PRINTF
#define printf(...)

#else
#define printf debug_print
#endif


/* CAN_A*/
# define CAN_BASE_ADDRESS CAN_BASE_ADDRESS_CAN_A

#define CODRV_BIT_TABLE_EXTERN 1
#define CODRV_CANCLOCK_200MHZ 1
/* end of application-specific defines */

/* do not modify comments starting with 'user-specific section:' */
/* user-specific section: start */

#include "node_id.h"

/* user-specific section: end */

#endif /* GEN_DEFINE_H */
