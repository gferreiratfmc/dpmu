/*
 * node_id.c
 *
 *  Created on: 25 jan. 2024
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#include "co_canopen.h"

uint8_t nodeID = 125;

UNSIGNED8 node_set_nodeID(void)
{
    return nodeID;
}

void node_update_nodeID(uint8_t new_nodeID)
{
    nodeID = new_nodeID;
}

