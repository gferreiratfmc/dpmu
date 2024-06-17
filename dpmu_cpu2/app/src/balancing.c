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

#define TIMER_TICK_COUNT_5_SECS 5000

uint16_t NUMBER_OF_READ_ITERATIONS = 5;
uint16_t NCOUNTS_TO_STABLE_VOLTAGE = ( 3.5e-3 / 17.5e-6 ); //Each count takes ~17.5us.
uint16_t NCOUNTS_TO_STABLE_VOLTAGE_VUP_VDOWN = 5 * ( 3.5e-3 / 17.5e-6 );



uint16_t cellNrReadOrder[] = {
     BAT_15, BAT_1, BAT_8, BAT_2, BAT_9, BAT_3, BAT_10, BAT_4, BAT_11, BAT_5, BAT_12, BAT_6, BAT_13, BAT_7, BAT_14,
     BAT_30, BAT_16, BAT_23, BAT_17, BAT_24, BAT_18, BAT_25, BAT_19, BAT_26, BAT_20, BAT_27, BAT_21, BAT_28, BAT_22, BAT_29
    };

bool enableReadCellVoltages = false;
bool readCellVoltagesDone = false;
bool readCellVoltagesBalancingDone = false;


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

void ConfigReadCellVoltagesBalancingDone( bool value ){
    readCellVoltagesBalancingDone = value;
}

bool ReadCellVoltagesBalancingDone( ){
    return readCellVoltagesBalancingDone;
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
            ConfigReadCellVoltagesBalancingDone( true );
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
    static bool done = false;
    static uint16_t totalCellsUnderThreshold = 0;
    volatile float dischargingCellVoltage;
    static uint32_t discharging_initial_time, discharging_elapsed_time;

    switch (balancing_state.State_Current)
    {
        case BALANCE_INIT:
            cellNr = BAT_1;
            cellIndex = ConvCellNrToIdx( cellNr );
            done = false;
            ConfigReadCellVoltagesBalancingDone( false );
            EnableContinuousReadCellVoltages();
            totalCellsUnderThreshold = 0;
            balancing_state.State_Next = BALANCE_READ_CELL_VOLTAGES;
            break;

        case BALANCE_READ_CELL_VOLTAGES:
            if( ReadCellVoltagesBalancingDone() == true ) {
                DisableContinuousReadCellVoltages();
                balancing_state.State_Next = BALANCE_VERIFY_HIGH_THRESHOLD;
            }
            break;

        case BALANCE_VERIFY_HIGH_THRESHOLD:
            cellIndex = ConvCellNrToIdx( cellNr );
            if( cellVoltageVector[cellIndex] >= (energy_bank_settings.max_allowed_voltage_energy_cell * CELL_VOLTAGE_RATIO_HIGH_THRESHOLD) ) {
                balancing_state.State_Next = BALANCE_CONNECT;
                PRINT("Start discharge cellNr:[%d] cellVoltageVector[cellIndex]:[%8.2f]\r\n", cellNr, cellVoltageVector[cellIndex]);
            } else {
                totalCellsUnderThreshold++;
                balancing_state.State_Next = BALANCE_NEXT_ITERATION;
            }
            break;


        case BALANCE_NEXT_ITERATION:
            cellNr = cellNr + 1;
            if( cellNr == BAT_15_N) {
                cellNr = BAT_16;
            }
            if( cellNr > BAT_30) {
                cellNr = BAT_1;
            }
            if( totalCellsUnderThreshold == 30 ) {
                balancing_state.State_Next = BALANCE_DONE;
            } else {
                balancing_state.State_Next = BALANCE_VERIFY_HIGH_THRESHOLD;
            }
            break;

        case BALANCE_CONNECT:
            switch_matrix_reset();
            switch_matrix_connect_cell( cellNr );
            switch_matrix_set_cell_polarity( cellNr );
            StartCllcControlLoop(cellNr);
            discharging_initial_time = timer_get_ticks();
            balancing_state.State_Next = BALANCE_DISCHARGE;
            break;

        case BALANCE_DISCHARGE:
            CllcControlLoop( cellNr );
            discharging_elapsed_time = timer_get_ticks() - discharging_initial_time;
            if( discharging_elapsed_time > TIMER_TICK_COUNT_5_SECS ) {
                discharging_initial_time = timer_get_ticks();
                StopCllcControlLoop();
                switch_matrix_reset();
                switch_matrix_connect_cell( cellNr );
                ConfigReadCellVoltagesBalancingDone( false );
                EnableContinuousReadCellVoltages();
                balancing_state.State_Next = BALANCE_DISCHARGE_READ_CELL_VOLTAGES;
            }
            break;

        case BALANCE_DISCHARGE_READ_CELL_VOLTAGES:
            if( ReadCellVoltagesBalancingDone() == true ) {
                DisableContinuousReadCellVoltages();
                balancing_state.State_Next = BALANCE_VERIFY_LOW_THRESHOLD;
            }
            break;

        case BALANCE_VERIFY_LOW_THRESHOLD:
            cellIndex = ConvCellNrToIdx( cellNr );
            PRINT("Discharging cellNr:[%d] cellVoltageVector[cellIndex]:[%4.2f] Vstore:[%4.2f]\r\n", cellNr, cellVoltageVector[cellIndex], DCDC_VI.avgVStore);
            if( cellVoltageVector[cellIndex] <= (energy_bank_settings.max_allowed_voltage_energy_cell * CELL_VOLTAGE_RATIO_LOW_THRESHOLD) ) {
                balancing_state.State_Next = BALANCE_INIT;
                PRINT("Stop discharge cellNr:[%d] cellVoltageVector[cellIndex]:[%8.2f]\r\n", cellNr, cellVoltageVector[cellIndex]);
            } else {
                balancing_state.State_Next = BALANCE_CONNECT;
            }
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


