/*
* codrv_error.h
*
* Copyright (c) 2012-2019 emotas embedded communication GmbH
*-------------------------------------------------------------------
* SVN  $Id: codrv_error.h 29515 2019-10-15 09:36:06Z hil $
*
*
*-------------------------------------------------------------------
*
*
*/

/********************************************************************/
/**
* \file
* \brief can error handling
*
*/

#ifndef CODRV_ERROR_H
#define CODRV_ERROR_H 1

/** CAN controller state */
typedef enum {
	Error_Offline,		/**< disabled */
	Error_Active,		/**< Error Active */
	Error_Passive,		/**< Error Passive */
	Error_Busoff		/**< Error bus off */
} CAN_ERROR_STATES_T;

/** CAN state event collection structure */
typedef struct {
	BOOL_T canErrorRxOverrun; 			/**< CAN controller overrun */

	BOOL_T canErrorPassive;				/**< CAN Passive event occured */
	BOOL_T canErrorActive;				/**< CAN active event occured */
	BOOL_T canErrorBusoff;				/**< CAN busoff event occured */

	BOOL_T drvErrorGeneric;				/**< CAN for generic errors */

	CAN_ERROR_STATES_T canOldState; 	/**< last signaled state */
	CAN_ERROR_STATES_T canNewState; 	/**< current state */

} CAN_ERROR_FLAGS_T;

/* externals
*--------------------------------------------------------------------------*/

/* function prototypes
*--------------------------------------------------------------------------*/
CAN_ERROR_FLAGS_T * codrvCanErrorGetFlags(void);
void codrvCanErrorInit(void);
RET_T codrvCanErrorInformStack(void);

#endif /* CODRV_ERROR_H */

