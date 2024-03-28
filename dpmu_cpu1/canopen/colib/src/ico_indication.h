/*
* ico_indication.h - contains internal defines for indications
*
* Copyright (c) 2014-2022 emotas embedded communication GmbH
*
*-------------------------------------------------------------------
* $Id: ico_indication.h 40516 2022-04-12 11:32:11Z boe $

*-------------------------------------------------------------------
*
*
*/

/**
* \file
* \brief indication header
*/

#ifndef ICO_INDICATION_H
#define ICO_INDICATION_H 1



/* -----------------------------------------------------------------*/
/* Profile 401 */
/* -----------------------------------------------------------------*/
#ifdef CO_PROFILE_401
# define CO_EVENT_PROFILE_SDO_SERVER_WRITE	1u
# define CO_EVENT_PROFILE_NMT				1u
# define CO_EVENT_PROFILE_CAN				1u
# define CO_EVENT_PROFILE_PDO       		1u
#endif /* CO_PROFILE_401 */


/* -----------------------------------------------------------------*/
/* Profile 402 */
/* -----------------------------------------------------------------*/
#ifdef CO_PROFILE_402
# define CO_EVENT_PROFILE_SDO_SERVER_WRITE	1u
# define CO_EVENT_PROFILE_PDO       		1u
#endif /* CO_PROFILE_402 */

/* -----------------------------------------------------------------*/
/* Profile 404 */
/* -----------------------------------------------------------------*/
#ifdef CO_PROFILE_404
# define CO_EVENT_PROFILE_SDO_SERVER_WRITE	1u
# define CO_EVENT_PROFILE_NMT				1u
# define CO_EVENT_PROFILE_CAN				1u
#endif /* CO_PROFILE_404 */

/* -----------------------------------------------------------------*/
/* Profile 406 */
/* -----------------------------------------------------------------*/
#ifdef CO_PROFILE_406
# define CO_EVENT_PROFILE_SDO_SERVER_WRITE	1u
# define CO_EVENT_PROFILE_SDO_SERVER_READ	1u
#endif /* CO_PROFILE_406 */


/* -----------------------------------------------------------------*/
/* Profile 410 */
/* -----------------------------------------------------------------*/
#ifdef CO_PROFILE_410
# define CO_EVENT_PROFILE_SDO_SERVER_WRITE	1u
#endif /* CO_PROFILE_410 */


/* -----------------------------------------------------------------*/
/* Profile 418 */
/* -----------------------------------------------------------------*/
#ifdef CO_PROFILE_418
# define CO_EVENT_PROFILE_SDO_SERVER_WRITE	1u
# define CO_EVENT_PROFILE_PDO       		1u
#endif /* CO_PROFILE_418 */


/* -----------------------------------------------------------------*/
/* Profile 419 */
/* -----------------------------------------------------------------*/
#ifdef CO_PROFILE_419
# define CO_EVENT_PROFILE_SDO_SERVER_WRITE	1u
# define CO_EVENT_PROFILE_PDO       		1u
# define CO_EVENT_PROFILE_SRD       		1u
# define CO_EVENT_PROFILE_SDO_CLIENT_READ	1u
#endif /* CO_PROFILE_419 */


/* -----------------------------------------------------------------*/
/* Profile 447 */
/* -----------------------------------------------------------------*/
#ifdef CO_PROFILE_447

# ifdef CO_NMT_MASTER
#  define CO_EVENT_PROFILE_ERRCTRL	1u
#  define CO_EVENT_PROFILE_SDO_CLIENT_READ	1u
# endif /* CO_NMT_MASTER */

# define CO_EVENT_PROFILE_LSS	1u
# define CO_EVENT_PROFILE_STORE	1u
#endif /* CO_PROFILE_447 */


/* -----------------------------------------------------------------*/
/* Profile 454 */
/* -----------------------------------------------------------------*/
#ifdef CO_PROFILE_454
# if defined(EB_NETWORK_VIRTUAL_DEV_CNT)
#  define CO_EVENT_PROFILE_OBJECT_CHANGED	EB_NETWORK_VIRTUAL_DEV_CNT
# endif
# define CO_EVENT_PROFILE_SDO_SERVER_READ		1u
# define CO_EVENT_PROFILE_SDO_SERVER_WRITE		1u
# define CO_EVENT_PROFILE_SDO_SERVER_CHECK_WRITE	1u
# define CO_EVENT_PROFILE_PDO			1u
# define CO_EVENT_PROFILE_NMT			1u
# define CO_EVENT_PROFILE_ERRCTRL		1u
# define CO_EVENT_PROFILE_SYNC			1u
# define CO_EVENT_PROFILE_SYNC_FINISHED	1u
# define CO_EVENT_PROFILE_CAN			1u
# define CO_EVENT_PROFILE_SDO_CLIENT_READ		1u
# define CO_EVENT_PROFILE_SDO_CLIENT_WRITE		1u
# define CO_EVENT_PROFILE_EMCY_CONSUMER	1u
# define CO_EVENT_PROFILE_LSS			1u

# ifdef EB_EBC_CNT
#  ifndef CO_SDO_QUEUE
#   define CO_SDO_QUEUE	1
#   define CO_SDO_QUEUE_LEN	(EB_NETWORK_VIRTUAL_DEV_CNT * 4)
#  endif /* CO_SDO_QUEUE */
#  ifndef CO_SDOQUEUE_INTERNAL_ACCESS
#   define CO_SDOQUEUE_INTERNAL_ACCESS	1
#  endif /* CO_SDOQUEUE_INTERNAL_ACCESS */
# endif /* EB_EBC_CNT */
#endif /* CO_PROFILE_454 */


/* -----------------------------------------------------------------*/
/* Configuration manager */
/* -----------------------------------------------------------------*/
#ifdef CO_CFG_MANAGER
# define CO_EVENT_PROFILE_SDO_CLIENT_WRITE       1u
#endif /* CO_CFG_MANAGER */


/* -----------------------------------------------------------------*/
/* CANopen startup manager */
/* -----------------------------------------------------------------*/
#ifdef CO_BOOTUP_MANAGER
# define CO_EVENT_PROFILE_SDO_CLIENT_READ		1u
# define CO_EVENT_PROFILE_USDO_CLIENT_READ		1u
# define CO_EVENT_PROFILE_ERRCTRL				1u
#endif /* CO_BOOTUP_MANAGER */


/* -----------------------------------------------------------------*/
/* -----------------------------------------------------------------*/
#if defined(CO_EVENT_STATIC_STORE) || defined(CO_EVENT_DYNAMIC_STORE) || defined(CO_EVENT_PROFILE_STORE)
# define CO_EVENT_STORE   1u
#endif /* defined(CO_EVENT_STATIC_STORE) || defined(CO_EVENT_STORE_CNT) || defined(CO_EVENT_PROFILE_STORE) */


#if defined(CO_EVENT_STATIC_SLEEP) || defined(CO_EVENT_DYNAMIC_SLEEP) || defined(CO_EVENT_PROFILE_SLEEP)
# define CO_EVENT_SLEEP   1u
#endif /* defined(CO_EVENT_STATIC_SLEEP) || defined(CO_EVENT_DYNAMIC_SLEEP) || defined(CO_EVENT_PROFILE_SLEEP) */


#if defined(CO_EVENT_DYNAMIC_OBJECT_CHANGED) || defined(CO_EVENT_PROFILE_OBJECT_CHANGED)
# define CO_EVENT_OBJECT_CHANGED   1u
#endif /* defined(CO_EVENT_DYNAMIC_OBJECT_CHANGED) || defined(CO_EVENT_PROFILE_OBJECT_CHANGED) */


#if defined(CO_EVENT_STATIC_LED) || defined(CO_EVENT_DYNAMIC_LED) || defined(CO_EVENT_PROFILE_LED)
# define CO_EVENT_LED   1u
#endif /* defined(CO_EVENT_STATIC_LED) || defined(CO_EVENT_DYNAMIC_LED) || defined(CO_EVENT_PROFILE_LED) */


#if defined(CO_EVENT_STATIC_SSDO_DOMAIN_WRITE) || defined(CO_EVENT_DYNAMIC_SSDO_DOMAIN_WRITE) || defined(CO_EVENT_PROFILE_SSDO_DOMAIN_WRITE)
# define CO_EVENT_SSDO_DOMAIN_WRITE   1u
#endif /* defined(CO_EVENT_STATIC_SSDO_DOMAIN_WRITE) || defined(CO_EVENT_SSDO_DOMAIN_WRITE) */

#if defined(CO_EVENT_STATIC_SSDO_DOMAIN_READ) || defined(CO_EVENT_DYNAMIC_SSDO_DOMAIN_READ) || defined(CO_EVENT_PROFILE_SSDO_DOMAIN_READ)
# define CO_EVENT_SSDO_DOMAIN_READ   1u
#endif /* defined(CO_EVENT_STATIC_SSDO_DOMAIN_READ) || defined(CO_EVENT_DYNAMIC_SSDO_DOMAIN_READ) || defined(CO_EVENT_PROFILE_SSDO_DOMAIN_READ) */

#if defined(CO_EVENT_STATIC_SUSDO_DOMAIN_WRITE) || defined(CO_EVENT_DYNAMIC_SUSDO_DOMAIN_WRITE) || defined(CO_EVENT_PROFILE_SUSDO_DOMAIN_WRITE)
# define CO_EVENT_SUSDO_DOMAIN_WRITE   1u
#endif /* defined(CO_EVENT_STATIC_SUSDO_DOMAIN_WRITE) || defined(CO_EVENT_DYNAMIC_SUSDO_DOMAIN_WRITE) || defined(CO_EVENT_PROFILE_SUSDO_DOMAIN_WRITE) */

#if defined(CO_EVENT_STATIC_SUSDO_DOMAIN_READ) || defined(CO_EVENT_DYNAMIC_SUSDO_DOMAIN_READ) || defined(CO_EVENT_PROFILE_SUSDO_DOMAIN_READ)
# define CO_EVENT_SUSDO_DOMAIN_READ   1u
#endif /* defined(CO_EVENT_STATIC_SUSDO_DOMAIN_READ) || defined(CO_EVENT_DYNAMIC_SUSDO_DOMAIN_READ) || defined(CO_EVENT_PROFILE_SUSDO_DOMAIN_READ) */

#if defined(CO_EVENT_STATIC_CUSDO_DOMAIN_WRITE) || defined(CO_EVENT_DYNAMIC_CUSDO_DOMAIN_WRITE) || defined(CO_EVENT_PROFILE_CUSDO_DOMAIN_WRITE)
# define CO_EVENT_CUSDO_DOMAIN_WRITE   1u
#endif /* defined(CO_EVENT_STATIC_CUSDO_DOMAIN_WRITE) || defined(CO_EVENT_DYNAMIC_CUSDO_DOMAIN_WRITE) || defined(CO_EVENT_PROFILE_CUSDO_DOMAIN_WRITE) */

#if defined(CO_EVENT_STATIC_CUSDO_DOMAIN_READ) || defined(CO_EVENT_DYNAMIC_CUSDO_DOMAIN_READ) || defined(CO_EVENT_PROFILE_CUSDO_DOMAIN_READ)
# define CO_EVENT_CUSDO_DOMAIN_READ   1u
#endif /* defined(CO_EVENT_STATIC_CUSDO_DOMAIN_READ || defined(CO_EVENT_DYNAMIC_CUSDO_DOMAIN_READ) || defined(CO_EVENT_PROFILE_CUSDO_DOMAIN_READ) */

#if defined(CO_EVENT_STATIC_SUSDO_READ_FINISHED) || defined(CO_EVENT_DYNAMIC_SUSDO_READ_FINISHED) || defined(CO_EVENT_PROFILE_SUSDO_READ_FINISHED)
# define CO_EVENT_SUSDO_READ_FINISHED   1u
#endif /* defined(CO_EVENT_STATIC_SUSDO_READ_FINISHED || defined(CO_EVENT_DYNAMIC_SUSDO_READ_FINISHED) || defined(CO_EVENT_PROFILE_SUSDO_READ_FINISHED) */

#if defined(CO_EVENT_STATIC_USDO_SERVER_CHECK_WRITE) || defined(CO_EVENT_DYNAMIC_USDO_SERVER_CHECK_WRITE) || defined(CO_EVENT_PROFILE_USDO_SERVER_CHECK_WRITE)
# define CO_EVENT_USDO_SERVER_CHECK_WRITE   1u
#endif /* defined(CO_EVENT_STATIC_SUSDO_READ_FINISHED || defined(CO_EVENT_DYNAMIC_SUSDO_READ_FINISHED) || defined(CO_EVENT_PROFILE_SUSDO_READ_FINISHED) */

#endif /* ICO_INDICATION_H */
