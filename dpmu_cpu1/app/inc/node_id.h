/*
 * node_id.h
 *
 *  Created on: 25 jan. 2024
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#ifndef APP_INC_NODE_ID_H_
#define APP_INC_NODE_ID_H_


#include "co_canopen.h"

UNSIGNED8 node_set_nodeID(void);
void node_update_nodeID(uint8_t new_nodeID);


#endif /* APP_INC_NODE_ID_H_ */
