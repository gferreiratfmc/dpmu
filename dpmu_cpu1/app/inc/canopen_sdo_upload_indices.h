/*
 * ccanopen_sdo_upload_indices.h
 *
 *  Created on: 15 apr. 2023
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#ifndef APP_INC_CANOPEN_SDO_UPLOAD_INDICES_H_
#define APP_INC_CANOPEN_SDO_UPLOAD_INDICES_H_

#include "co_datatype.h"

uint8_t co_usr_sdo_ul_indices(
        BOOL_T      execute,
        UNSIGNED8   sdoNr,
        UNSIGNED16  index,
        UNSIGNED8   subIndex
    );


#endif /* APP_INC_CANOPEN_SDO_UPLOAD_INDICES_H_ */
