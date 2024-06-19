/*
 * check_CPU2.c
 *
 *  Created on: 22 aug. 2023
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#include <error_handling.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "co_canopen.h"
#include "common.h"
#include "convert.h"
#include "ext_flash.h"
#include "gen_indices.h"
#include "initialization_app.h"
#include "serial.h"
#include "shared_variables.h"
#include "state_of_health.h"
#include "timer.h"

extern struct Serial cli_serial;

float initial_cap_energy_bank = 0;
float         cap_energy_bank = 0;
float initial_cap_energy_cell[30] = {0};
float         cap_energy_cell[30] = {0};
static bool initial_values_read   = false;
static bool initial_values_stored = false;

static bool check_SoH(void)
{
    bool values_updated = false;
    static uint16_t currState = 0;
    static uint16_t nextState = 0;

    //Save caoacitances recived from CPU2 to flash
    switch( currState ) {
        case 0:
            sharedVars_cpu1toCpu2.newCapacitanceSaved = false;
            if( sharedVars_cpu2toCpu1.newCapacitanceAvailable == true ) {
                nextState = 1;
            }
            break;

        case 1:
            saveCapacitanceToFlash( sharedVars_cpu2toCpu1.initialCapacitance, sharedVars_cpu2toCpu1.currentCapacitance );
            sharedVars_cpu1toCpu2.newCapacitanceSaved = true;
            nextState  = 2;
            break;

        case 2:
            if( sharedVars_cpu2toCpu1.newCapacitanceAvailable == false ) {
                nextState  = 0;
                values_updated = true;
            }
    }
    currState = nextState;

    retrieveInitalCapacitanceFromFlash( &sharedVars_cpu1toCpu2.initialCapacitance, &sharedVars_cpu1toCpu2.currentCapacitance );

    return values_updated;
}

static bool check_current_state(void)
{
    static sharedVars_cpu2toCpu1_t last_read_values = {0,0};
    bool values_updated = false;

    /* update current state of DPMU */
    if(last_read_values.current_state != sharedVars_cpu2toCpu1.current_state)
    {
        /* copy state to last read state variable */
        last_read_values.current_state = sharedVars_cpu2toCpu1.current_state;

        /* update state in CAN open OD */
        coOdPutObj_u8(I_DPMU_STATE, S_DPMU_OPERATION_CURRENT_STATE,
                      last_read_values.current_state);

        if(last_read_values.current_state == Fault ) {
            ResetDPMUAppInfoInitializeVars();
        }

        values_updated = true;
    }

    return values_updated;
}

bool check_changes_from_CPU2(void)
{
    bool values_updated = false;

    /* update current state of DPMU */
    values_updated |= check_current_state();

    /* update SoC */
//    values_updated |= check_SoC();

    /* update SoH */
    values_updated |= check_SoH();

    return values_updated;
}



/*
 * returns true if sector in external flash were found
 */
static void read_initial_capacitances_from_flash(float *data, size_t length)
{
    /* set sector in external flash */
    ext_flash_desc_t sector_info;
    sector_info.sector = EXT_FLASH_SA1;

    /* look up address corresponding to start of sector */
    look_up_start_address_of_sector(&sector_info);

    /* red buffer from external flash */

    ext_flash_read_buf(sector_info.addr, (uint16_t*)data, length);
}

static void write_initial_capacitances_to_flash(float *data, size_t length)
{
    /* set sector in external flash */
    ext_flash_desc_t sector_info;
    sector_info.sector = EXT_FLASH_SA1;

    /* look up address corresponding to start of sector */
    look_up_start_address_of_sector(&sector_info);

    /* red buffer from external flash */
    ext_flash_write_buf(sector_info.addr, (uint16_t*)data, length);
}

/* brief: read initial capacitance values from external FLASH
 *
 * details: checks if initial values are already present
 *          if flag initial_values_read is set -> no read
 *
 * requirements:
 *
 * argument: none
 *
 * return: RET_OK - successful reading or,
 *                  values already present in initial_cap_energy_bank/-cell[]
*          RET_FLASH_EMPTY - no values in FLASH
*          RET_FLASH_ERROR - could not read FLASH
 *
 * note: non-blocking
 *
 * presumptions:
 *
 */
static RET_T read_initial_caps_from_external_flash(float *data)
{
    RET_T ret = RET_OK;

    /* 16 bit is easier to work with for this 16 bit DSP
     * and we have plenty of space in the external FLASH
     *
     * Array starts with a mark Byte
     *
     * If mark Byte is 0xffff (unwritten),
     * no initial capacitances have been written to FLASH.
     *
     * If mark Byte is 0x0, initial values have been written.
     * */

    /* check if we already have read the initial capacitances */
    if(initial_values_read)
    {
        ret = RET_OK;
    } else
    {
        /* read initial capacitance from memory
         * store in array data[]
         * */
        read_initial_capacitances_from_flash(data, sizeof(data));

        /* check if initial values have previously been stored */
        if(0 == data[0])
        {
            int i = 1;

            /* copy initial capacitance of energy bank */
            initial_cap_energy_bank = data[i++];

            /* copy initial capacitance for each cell */
            for(int j = 0; j < 30; i++, j++)
            {
                initial_cap_energy_cell[j] = data[i];
            }

            /* we do not need to do this again */
            initial_values_read = true;

            ret = RET_OK;
        } else
        {
            initial_values_read = false;

            ret = RET_FLASH_EMPTY;
        }
    }

    return ret;
}

/* brief: check if initial values exists in memory or external FLASH
 *
 * details: checks if initial values are already present
 *
 * requirements:
 *
 * argument: none
 *
 * return: RET_OK - values present in memory or external FLASH
*          RET_FLASH_EMPTY - no values in FLASH
*          RET_FLASH_ERROR - could not read FLASH
 *
 * note: non-blocking
 *
 * presumptions:
 *
 */
static RET_T initial_caps_exists_in_external_flash(float *data)
{
    RET_T ret;

    /* 16 bit is easier to work with for this 16 bit DSP
     * Starts with a mark Byte, if it is not 0xffff (unwritten),
     * no initial capacitances have been written to FLASH
     * */

    /* check if the initial capacitances is present */
    ret = read_initial_caps_from_external_flash(data);

    return ret;
}

/* brief: store initial capacitance values in external FLASH
 *
 * details: checks if initial values are already present
 *          if initial values are already present, read them back
 *
 * requirements:
 *
 * argument: none
 *
 * return: RET_OK - could store values in FLASH
 *         RET_ALREADY_INITIALIZED - values already present in FLASH
 *         RET_ERROR_STORE - could not store values in FLASH
 *
 * note: non-blocking
 *
 * presumptions:
 *
 */
static RET_T store_initial_caps_in_external_flash(RET_T status, float *data)
{
    RET_T ret = RET_OK;

    /* 16 bit is easier to work with for this 16 bit DSP
     * Starts with a mark Byte, if it is not 0xffff (unwritten),
     * no initial capacitances have been written to FLASH
     * */

    if(initial_values_stored)
    {
        ret = RET_ALREADY_INITIALIZED;
    } else
    {
        /* check if initial values exist on external FLASH
         * read them back to initial_cap_energy_bank
         * and to initial_cap_energy_cell[]
         */
//        ret = read_initial_caps_from_external_flash(data);

        /* could we read external FLASH */
        if(RET_FLASH_EMPTY == status)
        {
            /* check if initial values have previously been stored */
            if(0 == data[0])
            {
                initial_values_read   = true;
                initial_values_stored = true;

                ret = RET_ALREADY_INITIALIZED;
            } else
            {
                /* build up the array */
                int i = 0;

                /* set mark byte to use */
                data[i++] = 0;

                /* add capacitance of energy bank */
                data[i++] = initial_cap_energy_bank;

                /* add capacitance for each cell */
                for(int j = 0; j < 30; i++, j++)
                {
                  data[i] = initial_cap_energy_cell[j];
                }

                write_initial_capacitances_to_flash(data, sizeof(data));
                /* marked as written above if 'if(0 == data[0])' is true
                 * set 'initial_values_stored = true' is set the second time we
                 * call this function, guaranteeing that flash has been written
                 * to
                 */
            }
        }
    }

    return ret;
}

void store_SoH_in_OD(uint8_t soH_energy_bank, float cell[30])
{
    uint8_t soH_energy_cell[30];

    convert_soh_all_energy_cell_to_OD(cell, soH_energy_cell);

    /* save calculated SoH for energy bank in CANopen OD */
    coOdPutObj_u8(I_ENERGY_BANK_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_BANK,
                  soH_energy_bank);
    /* save calculated SoH per cell in CANopen OD */
    int i = 0;
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_01,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_02,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_03,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_04,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_05,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_06,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_07,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_08,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_09,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_10,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_11,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_12,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_13,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_14,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_15,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_16,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_17,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_18,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_19,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_20,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_21,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_22,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_23,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_24,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_25,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_26,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_27,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_28,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_29,
                  soH_energy_cell[i++]);
    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_HEALTH_OF_ENERGY_CELL_30,
                  soH_energy_cell[i++]);
}

void calculate_capacitance(void)
{
//    uint16_t charge_time    = sharedVars_cpu2toCpu1.energy_bank.chargeTime;
//    float    charge_time_sec;
//    float    charge_current = sharedVars_cpu2toCpu1.energy_bank.chargeCurrent;
//    float    time_current;  /* charge_time_sec * charge_current_float */
//
//    charge_time_sec = convert_charge_time_to_seconds(charge_time);
//    time_current = charge_time_sec * charge_current;
//
//    /* calculate present capacitance per cell
//     *
//     * C[n] = i_charge       * (T_end - T_start) / (V_end[n] - V_start[n])
//     * C[n] = charge_current * (charge_time)     / (voltage_delta)
//     * C[n] = time_current                       / (voltage_delta)
//     * there 'n' is the number of the individual cell
//     * */
//    for (int i = 0; i <= 30; i++)
//    {
//        /* calculate delta Voltage */
//        float voltage_delta = sharedVars_cpu2toCpu1.energy_bank.cellVoltageAfterFirstCharge[i]
//                - sharedVars_cpu2toCpu1.energy_bank.cellVoltageBeforeFirstCharge[i];
//        /* calculate capacitance of cell */
//        float cap = time_current / voltage_delta;
//        cap_energy_cell[i] = (uint8_t) cap;
//        /* add capacitance to energy bank capacitance */
//        cap_energy_bank += cap;
//    }
}

RET_T store_initial_capacitances(void)
{
    RET_T status;

    /* 16 bit is easier to work with for this 16 bit DSP
     * Starts with a mark Byte, if it is not 0xffff (unwritten),
     * no initial capacitances have been written to FLASH
     * */
    float data[32];

    /* check for initial capacitances */
    status = initial_caps_exists_in_external_flash(data);
    if(RET_FLASH_EMPTY == status){
        /* save initial capacitance of the energy bank */
        initial_cap_energy_bank = cap_energy_bank;
        /* save initial capacitance of each cell */
        for (int i = 0; i <= 30; i++)
        {
            /* save calculated value as initial capacitance of the cell */
            initial_cap_energy_cell[i] = cap_energy_cell[i];
        }
        /* store initial capacitance values
         * keep the return value, it is also the return value for this function
         * */
        status = store_initial_caps_in_external_flash(status, data);
    }
    return status;
}

void calculate_SoH_energy_bank(float soH_energy_cell[30],
                               float *soH_energy_bank)
{
    /* calculate SoH per cell */
    for (int i = 0; i <= 30; i++)
    {
        /* times 100 to get result in full percent */
        if(initial_cap_energy_cell[i]) /* only if denominator is not zero */
            soH_energy_cell[i] = (100 * cap_energy_cell[i])
                    / initial_cap_energy_cell[i];
    }
    /* calculate SoH for energy bank
     * times 100 to get result in full percent
     * */
    if(initial_cap_energy_bank) /* only if denominator is not zero */
        *soH_energy_bank = (100 * cap_energy_bank) / initial_cap_energy_bank;
}

//static RET_T calculate_SoH(void)
//{
//    RET_T    ret;
//    float  soH_energy_bank = 0;
//    float  soH_energy_cell[30] = {0};
//
//    /* reset value */
//    cap_energy_bank = 0;
//
//    /* calculate present capacitances */
//    calculate_capacitance();
//
//    /* signal to CPU2 the values have been used and can be overwritten */
//    IPC_ackFlagRtoL(IPC_CPU1_L_CPU2_R,
//                    IPC_NEW_CONSTANT_CURRENT_CHARGE_DONE);
//
//    /* store initial capacitances */
//    ret = store_initial_capacitances();
//
//    /* check if we have initial capacitances */
//    if(RET_FLASH_ERROR == ret)
//    {
//        return ret;
//    } /* else if(RET_OK == ret)
//       * initial values present in FLASH -> continue
//       * no need to test this
//      * */
//
//    /* calculate SoH per cell */
//    calculate_SoH_energy_bank(soH_energy_cell, &soH_energy_bank);
//
//    /* store calculated SoH in CANopen OD */
//    store_SoH_in_OD(soH_energy_bank, soH_energy_cell);
//    return ret;
//}



//static void store_SoC_in_OD(float soC_energy_bank, float cell[30])
//{
//    uint8_t soC_energy_cell[30];
//    convert_soc_all_energy_cell_to_OD(cell, soC_energy_cell);
//
//    /* save calculated SoC for energy bank in CANopen OD */
//    coOdPutObj_u8(I_ENERGY_BANK_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_BANK,
//                  soC_energy_bank);
//    /* save calculated SoC per cell in CANopen OD */
//    int i = 0;
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_01,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_02,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_03,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_04,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_05,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_06,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_07,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_08,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_09,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_10,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_11,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_12,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_13,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_14,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_15,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_16,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_17,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_18,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_19,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_20,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_21,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_22,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_23,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_24,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_25,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_26,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_27,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_28,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_29,
//                  soC_energy_cell[i++]);
//    coOdPutObj_u8(I_ENERGY_CELL_SUMMARY, S_STATE_OF_CHARGE_OF_ENERGY_CELL_30,
//                  soC_energy_cell[i++]);
//}

//TODO how do we get a signal from CPU2 for new values - ipc/sharedVars_cpu2toCpu1
//TODO how do we get the updates values from CPU2 - ipc/sharedVars_cpu2toCpu1
//static bool calculate_SoC(void)
//{
//    bool ret = true;
//    float soC_energy_bank = 0;
//    float soC_energy_cell[30] = {0};
//    float present_voltage_energy_cell[30];
//
//    /* store energy bank Voltage locally */
//    uint16_t present_energy_bank_voltage =
//                            sharedVars_cpu2toCpu1.energy_bank.energyBankVoltageAfterFirstCharge;
//
//    /* store cell Voltages locally */
//    for(int i = 0; i <= 30; i++)
//        present_voltage_energy_cell[i] =
//                            sharedVars_cpu2toCpu1.soc_energy_cell[i];
//
//
//    /* SPC70057501, Rev: 02 - PENDING
//     * Table 7: Data from DPMU to IOP
//     * page 27
//     * point 34
//     *
//     * State_of_Charge_of_Energy_Cell:
//     * Instantaneous energy of each storage cell
//     * based on the instantaneous voltage E=C(V^2)/2.
//     */
//
//    /* SPC70057501, Rev: 02 - PENDING
//     * Table 7: Data from DPMU to IOP
//     * page 27
//     * point 33
//     *
//     * State_of_Charge_of_Energy_Bank:
//     * Instantaneous sum of the energies of each storage cell (see State_of_Charge_of_Energy_Cell).
//     */
//
//    /* E=CV^2/2 per cell [J] */
//    for(int i = 0; i <= 30; i++)
//    {
//        float tmp;
//
//        /* Voltage in in Volt
//         * Capacitance is in Farad
//         * */
//        tmp = present_voltage_energy_cell[i] * present_voltage_energy_cell[i];
//        tmp /= 2;
//        soC_energy_cell[i] = cap_energy_cell[i] * tmp;
//
//        /* sum up for total charge of the energy bank */
//        soC_energy_bank += soC_energy_cell[i];
//    }
//
//    /* store calculated SoC in CANopen OD */
//    store_SoC_in_OD(soC_energy_bank, soC_energy_cell);
//
//    /* retrieve min allowed Voltage over energy bank */
//    uint8_t min_voltage_at_energy_bank_OD = 0;
//    if(RET_OK != coOdGetObj_u8(I_ENERGY_BANK_SUMMARY,
//                                S_MIN_VOLTAGE_APPLIED_TO_ENERGY_BANK,
//                                (uint8_t*)&min_voltage_at_energy_bank_OD))
//        ret = false;
//    float min_voltage_at_energy_bank = convert_voltage_energy_bank_from_OD(min_voltage_at_energy_bank_OD);
//
//    /* calculate charge left in energy bank at min Voltage */
//    /* E=CV^2/2 */
//    float min_level_charge_energy_bank =
//            min_voltage_at_energy_bank * min_voltage_at_energy_bank;
//    min_level_charge_energy_bank *= cap_energy_bank;
//    min_level_charge_energy_bank /= 2;
//
//    /* calculate present charge in energy bank */
//    /* E=CV^2/2 */
//    float present_charge_energy_bank =
//            present_energy_bank_voltage * present_energy_bank_voltage;
//    present_charge_energy_bank *= cap_energy_bank;
//    present_charge_energy_bank /= 2;
//
//    /* calculate remaining energy before reaching
//     * min allowed Voltage over energy bank
//     * */
//    float remaining_energy_to_min_soc = present_charge_energy_bank -
//                                        min_level_charge_energy_bank;
//
//    /* store remaining SoC of energy bank in CANopen OD */
//    int16_t remaining_energy_to_min_soc_OD =
//            convert_energy_soc_energy_bank_to_OD(remaining_energy_to_min_soc);
//    if(RET_OK != coOdPutObj_u16(I_ENERGY_BANK_SUMMARY,
//                                S_REMAINING_ENERGY_TO_MIN_SOC_AT_ENERGY_BANK,
//                                remaining_energy_to_min_soc_OD))
//        ret = false;
//
//    /* retrieve safety threshold SoC for energy bank */
//    uint16_t safety_threshold = 0;
//    if(RET_OK != coOdGetObj_u16(I_ENERGY_BANK_SUMMARY,
//                                S_SAFETY_THRESHOLD_STATE_OF_CHARGE,
//                                (uint8_t*)&safety_threshold))
//        ret = false;
//
//    /* do this only if everything above went well
//     * should not test against a old value */
//    if(true == ret)
//    {
//        /* check remaining SoC relative safety threshold SoC
//         * set error flag if below limit
//         */
//        if(remaining_energy_to_min_soc < safety_threshold) {
//            global_error_code |=  (1UL << ERROR_SOC_BELOW_SAFETY_THRESHOLD);
//        } else {
//            global_error_code &= ~(1UL << ERROR_SOC_BELOW_SAFETY_THRESHOLD);
//        }
//
//        /*TODO necessary ?
//         *     this is the same as comparing the Voltage over energy bank and
//         *     its min allowed Voltage
//         *     if(remaining_energy_to_min_soc < 0), same as
//         *     if(voltage_margin < 0), same as
//         *     if(energyBankVoltage < min_voltage_at_energy_bank)
//         *     this might be checked in CPU2 in dcbus_check() in dcbus.c
//         */
//        /* check remSoC relative min allowed Voltage over energy bank
//         * set error flag if below limit
//         */
//        if(remaining_energy_to_min_soc < 0) {
//            global_error_code |=  (1UL << ERROR_SOC_BELOW_LIMIT);
//        } else {
//            global_error_code &= ~(1UL << ERROR_SOC_BELOW_LIMIT);
//        }
//    }
//
//    return ret;
//}

//static bool check_SoC(void)
//{
//    bool ret;
//    bool values_updated = false;
//
//    //TODO insert a timeout so we do not do this every millisecond
//    ret = calculate_SoC();
//
//    if(true == ret)
//        values_updated = true;
//
//    return values_updated;
//}

