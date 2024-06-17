/*
 * blancing.h
 *
 *  Created on: 20 de fev de 2024
 *      Author: gferreira
 */

#ifndef APP_INC_BALANCING_H_
#define APP_INC_BALANCING_H_

#include <stdint.h>
#define CELL_VOLTAGE_RATIO_LOW_THRESHOLD (2.7 / 3.0)  //0.90
#define CELL_VOLTAGE_RATIO_HIGH_THRESHOLD (2.90 / 3.0) //0.967




void EnableContinuousReadCellVoltages();
void DisableContinuousReadCellVoltages();
bool StatusContinousReadCellVoltages();
bool ReadCellVoltagesStateMachine(float *cellVoltages, float *energyBankVoltage, bool *cellVoltageOverThreshold );
bool BalancingAllCells();
bool ReadCellVoltagesDone();


static bool is_energy_bank_discharged_enough_for_recalculating_SoH(bool *first_CC_charge_stage_done);
static bool save_voltages_before_first_cc_charge(bool *first_CC_charge_stage_done,
                                                 float *cellvoltages,
                                                 float *energyBankVoltage);
static bool save_voltages_after_first_cc_charge(bool *first_CC_charge_stage_done,
                                                float *CapsCellvoltages,
                                                float *energyBankVoltage);
static bool mark_cells_for_discharge(float cellvoltages[30],
                                     uint16_t LowSideCurrentIndex,
                                     uint16_t HighSideCurrentIndex);
static bool unmark_discharged_cells(void);

static void SettingSwitchMatrix_Discharge(uint16_t LowSideCurrentIndex, uint16_t HighSideCurrentIndex);


#endif /* APP_INC_BALANCING_H_ */
