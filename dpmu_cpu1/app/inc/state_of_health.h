/*
 * state_of_health.h
 *
 *  Created on: 23 de mai de 2024
 *      Author: gferreira
 */

#ifndef APP_INC_STATE_OF_HEALTH_H_
#define APP_INC_STATE_OF_HEALTH_H_

void RequestSaveNewCapacitanceToExtFlash(float initialCapacitance, float currentCapacitance );
void RetrieveInitalCapacitanceFromFlash( float *initialCapacitance, float *currentCapacitance  );


#endif /* APP_INC_STATE_OF_HEALTH_H_ */
