/*
 * dpmu_type.h
 *
 *  Created on: 25 sep. 2023
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#ifndef COMMON_INC_DPMU_TYPE_H_
#define COMMON_INC_DPMU_TYPE_H_


#include <stdbool.h>

typedef enum dpmu_type{
    DPMU_DEFAULT_CAP = 0,               /* default DPMU with capacitors */
    DPMU_DEFAULT_CAP_W_REDUNTANT,       /* redundant DPMU with capacitors, with redundant DPMU */
    DPMU_DEFAULT_CAP_W_SUPPLEMENTARY,   /* redundant DPMU with capacitors, with supplementary DPMU */
    DPMU_DEFAULT_CAP_W_REDUNDANT_SUPPLEMENTARY,/* redundant DPMU with capacitors, with redundant DPMU and with supplementary DPMU */
    DPMU_DEFAULT_BAT,                   /* default DPMU with batteries  */
    DPMU_DEFAULT_BAT_W_REDUNTANT,       /* redundant DPMU with batteries , with supplementary DPMU */
    DPMU_DEFAULT_BAT_W_SUPPLEMENTARY,   /* redundant DPMU with batteries , with redundant DPMU */
    DPMU_DEFAULT_BAT_W_REDUNDANT_SUPPLEMENTARY,/* redundant DPMU with capacitors, with redundant DPMU and with supplementary DPMU */
    DPMU_REDUNDANT_CAP,                 /* redundant DPMU with capacitors */
    DPMU_REDUNDANT_CAP_W_SUPPLEMENTARY, /* redundant DPMU with capacitors, with supplementary DPMU */
    DPMU_REDUNDANT_BAT,                 /* redundant DPMU with batteries  */
    DPMU_REDUNDANT_BAT_W_SUPPLEMENTARY, /* redundant DPMU with batteries , with supplementary DPMU */
    DPMU_SUPPLEMENTARY_CAP,             /* supplementary DPMU with capacitors in parallel with default or redundant DPMU */
    DPMU_SUPPLEMENTARY_BAT,             /* supplementary DPMU with batteries  in parallel with default or redundant DPMU */
} dpmu_type_t;

bool dpmu_type_having_battery(void);
bool dpmu_type_having_caps(void);

bool dpmu_type_allowed_to_use_shared_bus(void);
bool dpmu_type_allowed_to_use_load_bus(void);

bool dpmu_type_default(void);

#endif /* COMMON_INC_DPMU_TYPE_H_ */
