/*
 * balancing.c
 *
 *  Created on: 20 de fev de 2024
 *      Author: gferreira
 */

#include <error_handling.h>
#include <stdbool.h>

#include "common.h"
#include "balancing.h"
#include "cli_cpu2.h"
#include "energy_storage.h"
#include "sensors.h"
#include "shared_variables.h"
#include "state_machine.h"
#include "switch_matrix.h"
#include "timer.h"

uint16_t NUMBER_OF_READ_ITERATIONS = 5;
uint16_t NCOUNTS_TO_STABLE_VOLTAGE = ( 3.5e-3 / 17.5e-6 ); //Each count takes ~17.5us.
uint16_t NCOUNTS_TO_STABLE_VOLTAGE_VUP_VDOWN = 5 * ( 3.5e-3 / 17.5e-6 );



uint16_t cellNrReadOrder[] = {
     BAT_15, BAT_1, BAT_8, BAT_2, BAT_9, BAT_3, BAT_10, BAT_4, BAT_11, BAT_5, BAT_12, BAT_6, BAT_13, BAT_7, BAT_14,
     BAT_30, BAT_16, BAT_23, BAT_17, BAT_24, BAT_18, BAT_25, BAT_19, BAT_26, BAT_20, BAT_27, BAT_21, BAT_28, BAT_22, BAT_29
    };

bool enableReadCellVoltages = false;
bool readCellVoltagesDone = false;

uint16_t ConvCellNrToIdx(uint16_t cellNr);

void EnableContinuousReadCellVoltages( ) {
    enableReadCellVoltages = true;
}
void DisableContinuousReadCellVoltages( ) {
    enableReadCellVoltages = false;
}

bool StatusContinousReadCellVoltages() {
    return enableReadCellVoltages;
}

bool ReadCellVoltagesDone() {
    return readCellVoltagesDone;
}

bool ReadCellVoltagesStateMachine(float *cellVoltages, float *energyBankVoltage, bool *cellVoltageOverThreshold )
{

    static States_t read_cell_state = {READ_CELL_INIT};
    static int cellNr = 15;
    static int cellVoltagesIdx = 0;
    static int cellReadCounter = 0;
    static int iteration = 0;
    static uint16_t readCellDelayCount = 0;
    static uint16_t voltageOverThresholdCount = 0;
    bool done = false;

    volatile float cellVoltageValue;
    static float cellVoltagesTemp[30];



    if( enableReadCellVoltages == false ) {
        return false;
    }


    switch(read_cell_state.State_Current)
    {
    case READ_CELL_INIT:
        if( cellVoltagesIdx < 15) {
            cellVoltagesTemp[cellVoltagesIdx]      = 0;
            cellVoltagesTemp[cellVoltagesIdx + 15] = 0;
            cellVoltagesIdx++;
        } else {
            *cellVoltageOverThreshold = false;
            *energyBankVoltage = 0;
            cellReadCounter = 0;
            iteration = 0;
            readCellVoltagesDone = false;
            read_cell_state.State_Next = READ_CELL_CONNECT;
            //total_time_start = timer_get_ticks();
            voltageOverThresholdCount = 0;
        }
        break;

    case READ_CELL_CONNECT:
        /* break before make */
        switch_matrix_reset();

        cellNr = cellNrReadOrder[cellReadCounter];
        switch_matrix_connect_cell( cellNr );/* CELL[ 1..30] */
        readCellDelayCount = 0;
        read_cell_state.State_Next = READ_CELL_CONNECT_DELAY;
        break;

    case READ_CELL_CONNECT_DELAY:
        if( readCellDelayCount == NCOUNTS_TO_STABLE_VOLTAGE ){
            read_cell_state.State_Next = READ_CELL_VALUE;
        } else {
            readCellDelayCount++;
        }
        break;

    case READ_CELL_VALUE:

        if( (cellNr >= BAT_1) && ( cellNr <= BAT_15 ) ) {
            /* CELL[ 1..15] */
            if( sensorVector[V_DwnfIdx].convertedReady ) {
                cellVoltageValue = sensorVector[V_DwnfIdx].realValue;
                cellVoltagesIdx = ConvCellNrToIdx( cellNr );
                if( (cellNr & 1) == 1) {
                    cellVoltagesTemp[cellVoltagesIdx] = cellVoltagesTemp[cellVoltagesIdx] - cellVoltageValue;
                } else {
                    cellVoltagesTemp[cellVoltagesIdx] = cellVoltagesTemp[cellVoltagesIdx] + cellVoltageValue;
                }
                read_cell_state.State_Next = READ_NEXT_CELL;
            }
        } else if ( (cellNr >= BAT_16) &&  (cellNr <= BAT_30) ){
            /* CELL[16..30] */
            if( sensorVector[V_UpfIdx].convertedReady ) {
                cellVoltageValue = sensorVector[V_UpfIdx].realValue;
                cellVoltagesIdx = ConvCellNrToIdx( cellNr );
                if( (cellNr & 1) == 1) {
                    cellVoltagesTemp[cellVoltagesIdx] =  cellVoltagesTemp[cellVoltagesIdx]  - cellVoltageValue;
                } else {
                    cellVoltagesTemp[cellVoltagesIdx] =  cellVoltagesTemp[cellVoltagesIdx]  + cellVoltageValue;
                }
                read_cell_state.State_Next = READ_NEXT_CELL;
            }
        } else {
            //Invalid cell number
            read_cell_state.State_Next = READ_NEXT_CELL;
        }
        break;

    case READ_NEXT_CELL:
        cellReadCounter++;
        if( cellReadCounter < NUMBER_OF_CELLS ){
            read_cell_state.State_Next = READ_CELL_CONNECT;
        } else {
            read_cell_state.State_Next = READ_NEXT_ITERATION;
        }
        break;

    case READ_NEXT_ITERATION:
        iteration++;
        cellReadCounter = 0;
        if( iteration == NUMBER_OF_READ_ITERATIONS ) {
            read_cell_state.State_Next = READ_CELL_DONE;
        } else {
            /* measure Voltage over energy bank */
            *energyBankVoltage += sensorVector[VStoreIdx].realValue;
            read_cell_state.State_Next = READ_CELL_CONNECT;
        }

        break;

    case READ_CELL_DONE:
        /* calculate mean value for each cell Voltage */
        if( cellReadCounter < 15 ) {
            cellVoltages[cellReadCounter] = cellVoltagesTemp[cellReadCounter] / (float)iteration;
            cellVoltages[cellReadCounter + 15] = cellVoltagesTemp[cellReadCounter + 15] / (float)iteration;

            if( ( cellVoltages[cellReadCounter] > sharedVars_cpu1toCpu2.max_allowed_voltage_energy_cell * CELL_VOLTAGE_RATIO_HIGH_THRESHOLD ) ||
                    ( cellVoltages[cellReadCounter+15] > sharedVars_cpu1toCpu2.max_allowed_voltage_energy_cell * CELL_VOLTAGE_RATIO_HIGH_THRESHOLD ) ) {
                voltageOverThresholdCount++;
            }

            cellReadCounter++;

        } else {
            *energyBankVoltage = *energyBankVoltage / (float)iteration;
            cellReadCounter = 0; /* prepare for next readings of each cell voltage */
            iteration = 0;  /* prepare for next N readings of all cell voltages */
            read_cell_state.State_Next = READ_CELL_INIT;
            if( voltageOverThresholdCount > 0 ) {
                *cellVoltageOverThreshold = true;
            }
            cellVoltagesIdx=0;
            readCellVoltagesDone = true;
            done = true;    /* we are done with these readings */
            //total_time_stop = timer_get_ticks();
        }
        break;
    }

    read_cell_state.State_Before = read_cell_state.State_Current;
    read_cell_state.State_Current = read_cell_state.State_Next;

    return done;
}

bool BalancingAllCells( float *cellVoltageVector ) {
    static uint16_t cellIndex, cellNr = BAT_1;
    static States_t balancing_state = { BALANCE_INIT };
    static uint16_t avgCellReadCount = 0;
    static float avgDischargingCellVoltage = 0.0;
    static bool done = false;
    static uint16_t totalCellsUnderThreshold = 0;
    //static float energyBankVoltage;
    //static bool cellVoltageOverThreshold;
    volatile float dischargingCellVoltage;
    static uint32_t discharging_initial_time, discharging_elapsed_time;

    switch (balancing_state.State_Current)
    {
        case BALANCE_INIT:
            cellNr = BAT_1;
            cellIndex = ConvCellNrToIdx( cellNr );
            done = false;
            EnableContinuousReadCellVoltages();
            balancing_state.State_Next = BALANCE_READ_CELL_VOLTAGES;
            break;

        case BALANCE_READ_CELL_VOLTAGES:
            if( ReadCellVoltagesDone() ) {
                DisableContinuousReadCellVoltages();
                balancing_state.State_Next = BALANCE_VERIFY_THRESHOLD;
            }
            break;

        case BALANCE_VERIFY_THRESHOLD:
            cellIndex = ConvCellNrToIdx( cellNr );
            if( cellVoltageVector[cellIndex] >= (energy_bank_settings.max_allowed_voltage_energy_cell * CELL_VOLTAGE_RATIO_HIGH_THRESHOLD) ) {
                balancing_state.State_Next = BALANCE_CONNECT;
                PRINT("Start discharge cellNr:[%d] cellVoltageVector[cellIndex]:[%8.2f]\r\n", cellNr, cellVoltageVector[cellIndex]);
            } else {
                totalCellsUnderThreshold++;
                balancing_state.State_Next = BALANCE_NEXT_ITERATION;
            }
            break;

        case BALANCE_CONNECT:
            switch_matrix_reset();
            switch_matrix_connect_cell( cellNr );
            switch_matrix_set_cell_polarity( cellNr );
            avgCellReadCount = 0;
            avgDischargingCellVoltage = 0.0;
            StartCllcControlLoop(cellNr);
            discharging_initial_time = timer_get_ticks();
            balancing_state.State_Next = BALANCE_DISCHARGE;
            break;

        case BALANCE_DISCHARGE:
            CllcControlLoop( cellNr );
            discharging_elapsed_time = timer_get_ticks() - discharging_initial_time;
            if( discharging_elapsed_time > 5000 ) {
                discharging_initial_time = timer_get_ticks();
                StopCllcControlLoop();
                balancing_state.State_Next = BALANCE_VERIFY_DISCHARGING_CELL;
            }
            break;

        case BALANCE_VERIFY_DISCHARGING_CELL:
            if( cellNr <= BAT_15 ) {
                if( sensorVector[V_DwnfIdx].convertedReady ) {
                    dischargingCellVoltage = sensorVector[V_DwnfIdx].realValue;
                    avgCellReadCount++;
                    if( ( cellNr & 0x01 ) == 0x01 ) {
                        avgDischargingCellVoltage = avgDischargingCellVoltage - dischargingCellVoltage;
                    } else {
                        avgDischargingCellVoltage = avgDischargingCellVoltage + dischargingCellVoltage;
                    }
                }
            } else {
                if( sensorVector[V_UpfIdx].convertedReady ) {
                    dischargingCellVoltage = sensorVector[V_UpfIdx].realValue;
                    avgCellReadCount++;
                    if( ( cellNr & 0x01 ) == 0x01 ) {
                        avgDischargingCellVoltage = avgDischargingCellVoltage - dischargingCellVoltage;
                    } else {
                        avgDischargingCellVoltage = avgDischargingCellVoltage + dischargingCellVoltage;
                    }
                }
            }
            if( avgCellReadCount == NCOUNTS_TO_STABLE_VOLTAGE ) {
                avgDischargingCellVoltage = avgDischargingCellVoltage / avgCellReadCount;
                if(  avgDischargingCellVoltage  < (energy_bank_settings.max_allowed_voltage_energy_cell * CELL_VOLTAGE_RATIO_LOW_THRESHOLD ) ) {
                    PRINT("==>Stop discharge cellNr:[%d] avgDischargingCellVoltage:[%8.2f]\r\n", cellNr, avgDischargingCellVoltage);
                    StopCllcControlLoop();
                    balancing_state.State_Next = BALANCE_NEXT_ITERATION;
                } else {
                    PRINT("==>Verify discharge cellNr:[%d] avgDischargingCellVoltage:[%8.2f]\r\n", cellNr, avgDischargingCellVoltage);
                    balancing_state.State_Next = BALANCE_CONNECT;
                }
                avgCellReadCount = 0;
                avgDischargingCellVoltage = 0.0;
            }
            break;

        case BALANCE_NEXT_ITERATION:
            cellNr = cellNr + 1;
            if( cellNr == BAT_15_N) {
                cellNr = BAT_16;
            }
            if( cellNr > BAT_30) {
                cellNr = BAT_1;
                if( totalCellsUnderThreshold == 30 ) {
                    balancing_state.State_Next = BALANCE_DONE;
                } else {
                    totalCellsUnderThreshold = 0;
                    balancing_state.State_Next = BALANCE_INIT;
                }

            } else {
//                balancing_state.State_Next = BALANCE_VERIFY_THRESHOLD;
                balancing_state.State_Next = BALANCE_READ_CELL_VOLTAGES;

            }
            PRINT("NEXT ITERATION cellNr[%d]\r\n", cellNr);
            break;
        case BALANCE_DONE:
            StopCllcControlLoop();
            PRINT("==> Balancing done\r\n");
            done = true;
            balancing_state.State_Next = BALANCE_INIT;
            break;

    }

    balancing_state.State_Before = balancing_state.State_Current;
    balancing_state.State_Current = balancing_state.State_Next;

    return done;

}

uint16_t ConvCellNrToIdx(uint16_t cellNr) {
    uint16_t idx=0;
    if( cellNr <= BAT_15 ) {
        idx = (cellNr - 1);
    } else if( cellNr >= BAT_16 ){
        idx = (cellNr - 2);
    }
    return idx;
}




// Henrik functions for reference only...

//static void DischargeCells(void)
//{
//    if (CLLC_VI.DischargeFlag1 == 1)
//    {
//        CLLC_cell_discharge_1();
//        HAL_StartPwmCllcCellDischarge1();
//    } else
//    {
//        HAL_StopPwmCllcCellDischarge1();
//    }
//
//    if (CLLC_VI.DischargeFlag2 == 1)
//    {
//        CLLC_cell_discharge_2();
//        HAL_StartPwmCllcCellDischarge2();
//    } else
//    {
//        HAL_StopPwmCllcCellDischarge2();
//    }
//}
//
///* possibly mark cells for discharge */
//static bool mark_cells_for_discharge(float cellvoltages[30],
//                              uint16_t LowSideCurrentIndex,
//                              uint16_t HighSideCurrentIndex)
//{
//    bool any_cell_marked;
//
//    /* mark cell for discharge - CELL[ 1..15] */
//    if (cellvoltages[CellInformations.Index_Lowside[LowSideCurrentIndex]] >
//                        energy_bank_settings.max_allowed_voltage_energy_cell)
//        CLLC_VI.DischargeFlag1 = 1;
//    else
//        CLLC_VI.DischargeFlag1 = 0;
//
//    /* mark cell for discharge - CELL[16..30] */
//    if (cellvoltages[CellInformations.Index_Highside[HighSideCurrentIndex] + 15] >
//                        energy_bank_settings.max_allowed_voltage_energy_cell)
//        CLLC_VI.DischargeFlag2 = 1;
//    else
//        CLLC_VI.DischargeFlag2 = 0;
//
//    any_cell_marked = (CLLC_VI.DischargeFlag1 || CLLC_VI.DischargeFlag2 ? true : false);
//
//    return any_cell_marked;
//}
//
///* possibly mark cells for discharge */
//static bool unmark_discharged_cells(void)
//{
//    bool any_cell_marked;
//
////    if(new Voltage readings)  //TODO do we need to have this check??
////    {
//    /* mark cell for discharge - CELL[ 1..15] */
//    if (sensorVector[V_DwnfIdx].realValue < energy_bank_settings.max_allowed_voltage_energy_cell)
//    {
//        CLLC_VI.DischargeFlag1 = 0;
//    }
//
//    /* mark cell for discharge - CELL[16..30] */
//    if (sensorVector[V_UpfIdx].realValue < energy_bank_settings.max_allowed_voltage_energy_cell)
//    {
//        CLLC_VI.DischargeFlag2 = 0;
//    }
//
//    any_cell_marked = (CLLC_VI.DischargeFlag1 || CLLC_VI.DischargeFlag2 ? true : false);
////    } else
////    {   /* no change */
////        any_cell_marked = true;
////    }
//
//    return any_cell_marked;
//}
//
//
//
//
//
///* are we ready for calculating new SoH
// * threshold of 50% discharge
// * */
//static bool is_energy_bank_discharged_enough_for_recalculating_SoH(
//                                            bool *first_CC_charge_stage_done)
//{
//    bool retVal = false;
//
//    //TODO update 50% to what is good and reasonable
//    /* calculate Voltage level after at least 50% discharge
//     * min + ((max - min) / 2)
//     */
//    uint16_t energy_bank_voltage_threshold =
//             energy_bank_settings.min_voltage_applied_to_energy_bank +
//           ((energy_bank_settings.max_voltage_applied_to_energy_bank -
//             energy_bank_settings.min_voltage_applied_to_energy_bank) / 2);
//
//    /* > 50% discharged */
//    if(sensorVector[VStoreIdx].realValue < (energy_bank_voltage_threshold))
//    {
//        /* reset variable to start procedure */
//        *first_CC_charge_stage_done = false;
//
//        retVal = true;
//    }
//
//    return retVal;
//}
//
///* copy initial Voltages to shared variable for CPU1 to work with
// * CPU1 uses these values for calculating the capacitance and
// * state of health for each cell and for the energy bank
// *
// * to reduce the error of measurements, measure each Voltage several times and
// * take the mean value of them
// *
// * steps of procedure:
// * 1 measure Voltage over cell 1 and cell 15
// * 2 increment to next cell, cell 2 and cell 16
// * 3 do step 1 [1..2] until all cells have got their Voltage measured once
// * 4 do step [1..3] ten times
// * 5 take the mean value for each cell
// * 6 store the Voltage for each cell and the energy bank in with CPU1 shared
// *   variable
// * */
///* save_voltages_before_first_cc_charge() returns true if initial
// * values have been stored
// *
// * save_voltages_before_first_cc_charge() returns true if we are not
// * discharged enough to be able to do a new SoH calculation
// *
// * save_voltages_before_first_cc_charge() returns false if we are in
// * a state there we can't jump directly to state Charge due to
// * needed recalculation of SoH and for that we need to store new
// * Voltage measurements before jumping to state Charge
// */
//static bool save_voltages_before_first_cc_charge(bool *first_CC_charge_stage_done,
//                                                 float *cellvoltages,
//                                                 float *energyBankVoltage)
//{
//
//    static bool cellVoltageOverThreshold = false;
//    bool done = true;
//
//    /* check if previous values have been used by CPU1
//     *
//     * do not write over previous values before CPU1 has used them,
//     * if so, CPU1 will get corrupted calculation of SoH
//     * */
//    if(!IPC_isFlagBusyLtoR(IPC_CPU2_L_CPU1_R,
//                       IPC_NEW_CONSTANT_CURRENT_CHARGE_DONE))
//        return done;
//
//    /* if needed, store cell Voltages of all cells before going to CC Charge */
//    if(is_energy_bank_discharged_enough_for_recalculating_SoH(first_CC_charge_stage_done))
//    {
//        if((done = ReadCellVoltagesStateMachine(cellvoltages, energyBankVoltage, &cellVoltageOverThreshold)))
//        {
//            /* we have a mean value of ten measurements for each energy cell */
//
//            /* copy initial Voltages to shared variable for CPU1 to work with
//             * CPU1 uses these values for calculating the capacitance and
//             * state of health for each cell and for the energy bank
//             * */
//            for(int i = 0; i < 30; i++)
//            {
//                sharedVars_cpu2toCpu1.energy_bank.cellVoltageBeforeFirstCharge[i] =
//                                                              cellvoltages[i];
//                PRINT("Before CellVoltage[%d]=%6.3f\r\n", i, cellvoltages[i] );
//            }
//
//            /* save Voltage before charging for energy bank */
//            sharedVars_cpu2toCpu1.energy_bank.energyBankVoltageBeforeFirstCharge =
//                                                            *energyBankVoltage;
//
//            /* reset charge timer */
//            sharedVars_cpu2toCpu1.energy_bank.chargeTime = 0;
//        }
//    }
//
//    return done;
//}
//
///* copy initial Voltages to shared variable for CPU1 to work with
// * CPU1 uses these values for calculating the capacitance and
// * state of health for each cell and for the energy bank
// *
// * to reduce the error of measurements, measure each Voltage several times and
// * take the mean value of them
// *
// * steps of procedure:
// * 1 measure Voltage over cell 1 and cell 15
// * 2 increment to next cell, cell 2 and cell 16
// * 3 do step 1 [1..2] until all cells have got their Voltage measured once
// * 4 do step [1..3] ten times
// * 5 take the mean value for each cell
// * 6 store the Voltage for each cell and the energy bank in with CPU1 shared
// *   variable
// * */
//static bool save_voltages_after_first_cc_charge(bool *first_CC_charge_stage_done,
//                                                float *cellvoltages,
//                                                float *energyBankVoltage)
//{
//    static bool cellVoltageOverThreshold = false;
//    bool done = true;
//
//    if (!*first_CC_charge_stage_done)
//    {
//        if((done = ReadCellVoltagesStateMachine(cellvoltages, energyBankVoltage, &cellVoltageOverThreshold)))
//        {
//
//            /* we have a mean value of ten measurements for each energy cell */
//
//            /* copy initial Voltages to shared variable for CPU1 to work with
//             * CPU1 uses these values for calculating the capacitance and
//             * state of health for each cell and for the energy bank
//             * */
//            for (int i = 0; i < 30; i++)
//            {
//                sharedVars_cpu2toCpu1.energy_bank.cellVoltageAfterFirstCharge[i] =
//                                                              cellvoltages[i];
//                PRINT("After CellVoltage[%d]=%6.3f\r\n", i, cellvoltages[i] );
//            }
//
//            /* save Voltage after charging for energy bank */
//            sharedVars_cpu2toCpu1.energy_bank.energyBankVoltageAfterFirstCharge =
//                                                            *energyBankVoltage;
//
//            /* save the used charging current */
//            sharedVars_cpu2toCpu1.energy_bank.chargeCurrent =
//                                            sensorVector[ISen2fIdx].realValue;
//
//            /* signal to CPU1 it is time to recalculate SoH */
//            IPC_setFlagLtoR(IPC_CPU2_L_CPU1_R,
//                            IPC_NEW_CONSTANT_CURRENT_CHARGE_DONE);
//
//            /* do this only before first balancing */
//            *first_CC_charge_stage_done = true;
//        }
//    }
//
//    return done;
//}
//
//static void SettingSwitchMatrix_Discharge(uint16_t LowSideCurrentIndex,
//                                          uint16_t HighSideCurrentIndex)
//{
//    if (CLLC_VI.DischargeFlag1 == 1)
//    {
//        /* cell zero resets the matrix */
//        switch_matrix_connect_cell(LowSideCurrentIndex + 1);    /* starts at zero */
//        //CLLC_VI.CelldischargeNumber1 ++;
//    }
//
//    if (CLLC_VI.DischargeFlag2 == 1)
//    {
//        /* cell zero resets the matrix */
//        switch_matrix_connect_cell(HighSideCurrentIndex + 16);  /* starts at zero */
//    }
//    //  CLLC_VI.CelldischargeNumber1 ++;
//}


//static bool balancing(float cellvoltages[30])
//{
//    static States_t balancing_state = { BALANCE_INIT };
//    bool done = false;
//
//    switch (balancing_state.State_Current)
//    {
//    case BALANCE_INIT:
//        switch_matrix_reset();
//
//        /* sort cells, highest Voltage first */
//        SortCellVoltageIndex(&cellvoltages[0], 0);  /* CELL[ 1..15] */
//        SortCellVoltageIndex(&cellvoltages[15], 1); /* CELL[16..30] */
//
////        for(int i = 0; i < 15; i++)
////            PRINT("%f ", cellvoltages[CellInformations.Index_Lowside[i]]);
////        PRINT("\r\n");
////        for(int i = 0; i < 15; i++)
////            PRINT("%f ", cellvoltages[CellInformations.Index_Highside[i] + 15]);
////        PRINT("\r\n");
//
//        CellInformations.CurrentIndexLowSide  = 0;  /* CELL[ 1..15] */
//        CellInformations.CurrentIndexHighSide = 0;  /* CELL[16..30] */
//
//        balancing_state.State_Next = BALANCE_CONNECT;
//        break;
//
//    case BALANCE_CONNECT:
//        /* mark cells for discharge
//         * these indexes starts at zero
//         * cell numbers start with one and sixteen
//         *
//         * index in CellInformations.CurrentIndexLowSide  starts at zero
//         * index in CellInformations.CurrentIndexHighSide starts at zero
//         * */
//        if (mark_cells_for_discharge(
//                cellvoltages, CellInformations.CurrentIndexLowSide, /* CELL[ 1..15] */
//                CellInformations.CurrentIndexHighSide))/* CELL[16..30] */
//        {
//            /* at least one cell is marked for discharge */
//            /* connect cells
//             * these indexes starts at zero
//             * cell numbers start with one and sixteen
//             *
//             * index in CellInformations.CurrentIndexLowSide  starts at zero
//             * index in CellInformations.CurrentIndexHighSide starts at zero
//             * */
//            SettingSwitchMatrix_Discharge(
//                    CellInformations.CurrentIndexLowSide,   /* CELL[ 1..15] */
//                    CellInformations.CurrentIndexHighSide); /* CELL[16..30] */
//
//            balancing_state.State_Next = BALANCE_DISCHARGE;
//        }
//        else
//        {
//            balancing_state.State_Next = BALANCE_NEXT_ITERATION;
//        }
//        break;
//
//    case BALANCE_DISCHARGE:
//        {
//            bool continue_dischargning;
//
//            /* mark_cells_for_discharge() returns true if
//             * at least one cell is marked to be discharged
//             *
//             * index in CellInformations.CurrentIndexLowSide  starts at zero
//             * index in CellInformations.CurrentIndexHighSide starts at zero
//             */
//            continue_dischargning = unmark_discharged_cells();
//
//            /* discharge cells
//             * I-loop
//             * */
//            DischargeCells();
//
//            if (!continue_dischargning) /* equals if(done) */
//            {
//                balancing_state.State_Next = BALANCE_NEXT_ITERATION;
//            }
//        }
//        break;
//
//    case BALANCE_NEXT_ITERATION:
//        /* prepare for next iteration / next pair of cells */
//        /* reset matrix
//         * break before make
//         */
//        switch_matrix_reset();
//
//        /* none of these pair of cells are marked for discharge
//         * increment index and try the next pair
//         * */
//        CellInformations.CurrentIndexLowSide++;
//        CellInformations.CurrentIndexHighSide++;
//
//        if (CellInformations.CurrentIndexLowSide  >= 15 ||  /* CELL[ 1..15] */
//            CellInformations.CurrentIndexHighSide >= 15)    /* CELL[16..30] */
//        {
//            balancing_state.State_Next = BALANCE_DONE;
//        }
//
//        balancing_state.State_Next = BALANCE_CONNECT;
//        break;
//
//    case BALANCE_DONE:
//        /* prepare for next balancing */
//        balancing_state.State_Next = BALANCE_INIT;
//
//        done = true;
//
//        break;
//
//    default: /* should never enter here */
//        sharedVars_cpu2toCpu1.error_code |= (1UL << ERROR_BALANCING); //TODO not handled, needed?
//        //TODO next safe state?
//        break;
//    }
//
//    /* update previous state */
//    balancing_state.State_Before = balancing_state.State_Current;
//
//    /* print next state then state changes */
////    if (balancing_state.State_Current != balancing_state.State_Next)
////        PRINT("Next balancing_state %02d\r\n", balancing_state.State_Next);
//
//    /* update current state */
//    balancing_state.State_Current = balancing_state.State_Next;
//
//    return done;
//}

/* sort to falling values
 * highest value on index 0
 * */
//static void SortCellVoltageIndex(float cellvoltages[], uint16_t Position)
//{
//    int size = 15;
//    int index[15];
//
//    for (int i = 0; i < size; i++)
//    {
//        index[i] = i;
//    }
//
//    for (int i = 0; i < size; i++)
//    {
//        for (int j = i + 1; j < size; j++)
//        {
//            float a = cellvoltages[index[i]];
//            float b = cellvoltages[index[j]];
//
//            if (a < b)
//            {
//                int temp = index[i];
//                index[i] = index[j];
//                index[j] = temp;
//            }
//        }
//    }
//
//    if (Position == 0)
//    {
//        memcpy(CellInformations.Index_Lowside, index, 15 * sizeof(float));
//    }
//    else
//    {
//        memcpy(CellInformations.Index_Highside, index, 15 * sizeof(float));
//    }
//}
