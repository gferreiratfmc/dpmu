/*
 * convert.c
 *
 *  Created on: 13 sep. 2023
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */

#include <math.h>
#include <stdint.h>

#include "common.h"
#include "convert.h"


/* see SPC70057501
 * Table 6 p.15
 * Appendix A
 * parameter: Max_Allowed_DC_Bus_Voltage
 *
 * type: uint8_t
 * comment: 1V resolution (0.55% of 180V)
 */

/* see SPC70057501
 * Table 6 p.22
 * Appendix A
 * parameter: Target_Voltage_At_DC_Voltage
 *
 * type: uint8_t
 * comment: 1V resolution (0.55% of 180V)
 */

/* see SPC70057501
 * Table 6 p.16
 * Appendix A
 * parameter: Min_Allowed_DC_Bus_Voltage
 *
 * type: uint8_t
 * comment: 1V resolution (0.55% of 180V)
 */

/* see SPC70057501
 * Appendix A
 * parameter: VDC_Bus_Short_Circuit_Limit
 *
 * type: uint8_t
 * comment: 1V resolution (0.55% of 180V)
 */

/* see SPC70057501
 * Appendix A
 * parameter: Read_Voltage_At_DC_Bus
 *
 * type: uint8_t
 * comment: 1 V resolution (0.55% of 180V)
 */
float convert_dc_bus_voltage_from_OD(uint8_t value)
{
    return (float)value;
}
uint8_t convert_dc_bus_voltage_to_OD(float value)
{
    return (uint8_t)value;
}

/* see SPC70057501
 * Table 6 p.21
 * Appendix A
 * parameter: Max_Allowed_Load_Power
 *
 * type: uint16_t
 * comment: Watt (DC Bus voltage x input current), Resolution of 1W
 */

/* see SPC70057501
 * Table 6 p.17
 * Appendix A
 * parameter: Available_Power_Budget_DC_Input
 *
 * type: uint16_t
 * comment: Watt (DC Bus voltage x input current), Resolution of 1W, 0-65 536 W
 */

/* see SPC70057501
 * Table 6 p.39
 * Appendix A
 * parameter: Power_Consumed_By_Load
 *
 * type: uint16_t
 * comment: Watt (DC Bus voltage x output current), Resolution of 1W
 */

/* see SPC70057501
 * Appendix A
 * parameter: Power_From_DC_Input
 *
 * type: uint16_t
 * comment: Watt (DC Bus voltage x output current), Resolution of 1W
 */
float convert_power_from_OD(uint16_t value)
{
    return (float)value;
}
uint16_t convert_power_to_OD(float value)
{
    return (uint16_t)value;
}

/* see SPC70057501
 * Appendix A
 * Table 6 p.28
 * parameter: Max_Voltage_Applied_To_Storage_Bank
 *
 * type: uint8_t
 * comment: Set to work between 75V and 80V. Maximum voltage of 90V
 * comment: This value shall be set based on the voltage of the supercapacitor
 *          bank. Normally the minimum voltage should be around 30V for the
 *          entire bank.
 */

/* see SPC70057501 //TODO not added yet, added as comment in .pdf
 * Appendix A
 * Table 6 p._
 * parameter: constant_voltage_threshold
 *
 * type: uint8_t
 * comment: Lithium batteries must be charges with a
 *          constant Voltage above a this threshold
 */

/* see SPC70057501
 * Table 6 p.29
 * Appendix A
 * parameter: Safety_Threshold_State_of_Charge
 *
 * type: uint16_t    ///TODO should it be uin16_t and therefore using convert_power_from_OD() ??
 * comment: Fixed amount of energy needed for safe parking [J]. This value is
 *          calculated based on the voltage of the entire bank and the total
 *          capacitance of the bank.
 * comment:
 */
float convert_voltage_energy_bank_from_OD(uint8_t value)
{
    return (float)value;
}
uint8_t convert_voltage_energy_bank_to_OD(float value)
{
    return (uint8_t)value;
}

/* see SPC70057501
 * Table 6 p.27
 * Appendix A
 * parameter: Min_Voltage_Applied_To_Energy_Bank
 *
 * type: uint8_t
 * comment: Fixed point 1111111.1. This value shall be set based on the voltage
 *          of the supercapacitor bank. Normally the minimum voltage should be
 *          around 30V for the entire bank.
 */

/* see SPC70057501 //TODO not added yet, added as comment in .pdf
 * Table 6 p._
 * Appendix A
 * parameter: preconditional_threshold
 *
 * type: uint8_t
 * comment: Lithium Batteries and super capacitor must be trickled charged with
 * a very small current below this threshold
 */
float convert_min_voltage_applied_to_energy_bank_from_OD(uint8_t value)
{
    return (float)value * pow(2, -1);
}
uint8_t convert_min_voltage_applied_to_energy_bank_to_OD(float value)
{
    return (uint8_t)(value * pow(2, 1));
}

/* see SPC70057501
 * Table 6 p.18
 * Appendix A
 * parameter: Max_Voltage_Energy_Cell
 *
 * type: uint8_t
 * comment: Maximum Voltage (Energy Cell).
 *          Fixed point of 1111.1111
 */

/* see SPC70057501
 * Table 6 p.19
 * Appendix A
 * parameter: Min_Voltage_Energy_Cell
 *
 * type: uint8_t
 * comment: Minimum Voltage (Energy Cell).
 *          Fixed point of 1111.1111
 */
float convert_voltage_energy_cell_from_OD(uint16_t value)
{
    return (float)value * pow(2, -4);
}
uint8_t convert_voltage_energy_cell_to_OD(float value)
{
    if( value < 0) {
        return 0;
    }
    return (uint8_t)(value * pow(2, 4));
}

/* see SPC70057501
 * Table 6 p.44
 * Appendix A
 * parameter: ESS_Current
 *
 * type: uint8_t
 * comment: Max Current (ESS) to load energy cells, secondary side of LLC.
 *          Fixed point of 1111.1111 (maximum current of 5A at this branch)
 */
float convert_ess_current_from_OD(uint16_t value)
{
    return (float)value * pow(2, -4);
}
uint8_t convert_ess_current_to_OD(float value)
{
    return (uint8_t)(value * pow(2, 4));
}



/* see SPC70057501
 * Table 6 p.33
 * Appendix A
 * parameter: State_of_Charge_of_Energy_Bank
 *
 * type: uint8_t
 * comment: Voltage at Vstor_iso - DPMU Main, max 90V, resolution around 0.353V
 *          (90/255). This value is calculated based on the sum of the
 *          individual supercapacitor SoC
 *          //TODO this sounds suspicious, why exactly 90V, non-binary limit ??
 */
uint16_t convert_soc_energy_bank_to_OD(float value)
{
    return (uint16_t)value;
}

/* see SPC70057501
 * Table 6 p.35
 * Appendix A
 * parameter: State_of_Health_of_Energy_Bank
 *
 * type: uint8_t
 * comment: Average of SoH of each energy cell, resolution of 0.5%, fixed point
 *          1111111.1. This value is calculated based on the capacitance
            calculated on each SoH, proper manipulation and compared to the
            initial SoH of the bank. This value shall represent a % of the
            initial SoH value
 */
float convert_soh_energy_bank_from_OD(uint8_t value)
{
    return (float)value * pow(2, -1);
}
uint8_t convert_soh_energy_bank_to_OD(float value)
{
    return (uint8_t)(value * pow(2, 1));
}

/* see SPC70057501
 * Table 6 p.40
 * Appendix A
 * parameter: Remaining_Energy_To_Min_SoC_At_Energy_Bank
 *
 * type: uint16_t
 * comment: Joule (P x t) - Remaining useful Energy at the Supercapacitor Bank.
 *          This includes the Budget to operate the system and the budget to
 *          safe parking the RVC
 */
float convert_energy_soc_energy_bank_from_OD(uint16_t value)
{
    return (float)value;
}
uint16_t convert_energy_soc_energy_bank_to_OD(float value)
{
    return (uint16_t)value;
}

/* see SPC70057501
 * Table 6 p.34
 * Appendix A
 * parameter: State_of_Charge_of_Energy_Cell
 *
 * type: uint8_t [30]
 * comment: Voltage at Vcell - DPMU Mezzanine. This is calculated based on the
 *          current and time to build up a voltage at individual supercapacitor.
 *          Fixed point of 1111.1111
 */
float convert_soc_energy_cell_from_OD(uint8_t value)
{
    return (float)value * pow(2, -4);
}
uint8_t convert_soc_energy_cell_to_OD(float value)
{
    return (uint8_t)(value * pow(2, 4));
}

/* brief: converts all remaining energy to min soc energy cell to float
 *
 * details: converts it from CANopen OD format
 *          fixed point of 1111111.1
 *
 * requirements:
 *
 * argument: uint16_t *cell        - pointer to array to be converted
 *           float *cell_converted - pointer to array to store conversion in
 *
 * return: none
 *
 * note: non-blocking
 *
 * presumptions:
 *
 */
void convert_soc_all_energy_cell_to_OD(float *cell, uint16_t *cell_converted)
{
    for(int i = 0; i < 30; i++)
        cell_converted[i] = convert_soc_energy_cell_to_OD(cell[i]);
}

/* see SPC70057501
 * Table 6 p.36
 * Appendix A
 * parameter: State_of_Health_of_Each_Energy_Cell
 *
 * type: uint8_t [30]
 * comment: Time to charge a specific cell until reaches a known voltage with a
 *          known current, resolution of 0.5%, % of initial capacitance value.
 *          Fixed point of 1111111.1
 */
float convert_soh_energy_cell_from_OD(uint16_t value)
{
    return (float)value * pow(2, -1);
}
uint8_t convert_soh_energy_cell_to_OD(float value)
{
    return (uint8_t)(value * pow(2, 1));
}

/* brief: converts all remaining energy to min soh energy cell to float
 *
 * details: converts it from CANopen OD format
 *          fixed point of 1111111.1
 *
 * requirements:
 *
 * argument: uint16_t *cell        - pointer to array to be converted
 *           float *cell_converted - pointer to array to store conversion in
 *
 * return: none
 *
 * note: non-blocking
 *
 * presumptions:
 *
 */
void convert_soh_all_energy_cell_to_OD(float *cell, uint16_t *cell_converted)
{
    for(int i = 0; i < 30; i++)
        cell_converted[i] = convert_soh_energy_cell_to_OD(cell[i]);
}

/* brief: converts from charge time units measured in CPU2 to seconds
 *
 * details:
 *
 * requirements:
 *
 * argument: none
 *
 * return: charge time in seconds
 *
 * note: non-blocking
 *
 * presumptions:
 *
 */
float convert_charge_time_to_seconds(uint16_t charge_time)
{
    /* TODO what time scale will it be ??
     * microseconds (constant = 1000)
     * state machine cycles, SM cycles (constant = 1000 * 20 ??)
     */
    int time_convert_constant = 1;

    return (float)charge_time / time_convert_constant;
}


int8_t convert_dc_load_current_to_OD(float value)
{
    return (int8_t)value;
}
