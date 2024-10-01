/*
 * hal.h
 *
 *  Created on: 28 feb. 2023
 *      Author: Luyu Wang
 *              Henrik Borg henrik.borg@ekpower.se hb
 *
 */

#ifndef APP_INC_HAL_H_
#define APP_INC_HAL_H_

#define BEG_1_2_NORMAL_MODE_FREQ_KHZ 140
#define BEG_1_2_PULSE_MODE_FREQ_KHZ 10

#include <board.h>
#include "GlobalV.h"

/*** PWM ***/





static inline void HAL_PWM_setPhaseShift(uint32_t PWMBase, uint16_t Phase_shiftcount)
{
    EPWM_setPhaseShift(PWMBase, Phase_shiftcount);
}

static inline void HAL_PWM_setCounterCompareValue(uint32_t base, EPWM_CounterCompareModule compModule,
                                 uint16_t compCount)
{
    EPWM_setCounterCompareValue(base, compModule, compCount);
}
static inline uint16_t HAL_PWM_getCounterCompareValue(uint32_t base, EPWM_CounterCompareModule compModule)
{
   return EPWM_getCounterCompareValue(base, compModule);
}

static inline void HAL_PWM_setTimeBasePeriod(uint32_t base, uint16_t periodCount)
{
    EPWM_setTimeBasePeriod(base, periodCount);
}

static inline uint16_t HAL_PWM_getTimeBasePeriod(uint32_t base)
{
    return EPWM_getTimeBasePeriod(base);
}

static inline void HAL_PWM_setDeadBandDelayMode(uint32_t base, EPWM_DeadBandDelayMode delayMode,
                                  bool enableDelayMode)
{
    EPWM_setDeadBandDelayMode(base, delayMode, enableDelayMode);
}

static inline void HAL_StopPwmCllcCellDischarge1(void)
{
    EPWM_forceTripZoneEvent(QABPWM_4_5_BASE, EPWM_TZ_FORCE_EVENT_OST);
    EPWM_forceTripZoneEvent(QABPWM_6_7_BASE, EPWM_TZ_FORCE_EVENT_OST);
}

static inline void HAL_StartPwmCllcCellDischarge1(void)
{
    EPWM_clearTripZoneFlag(QABPWM_4_5_BASE, EPWM_TZ_FORCE_EVENT_OST);
    EPWM_clearTripZoneFlag(QABPWM_6_7_BASE, EPWM_TZ_FORCE_EVENT_OST);
}

static inline void HAL_StopPwmCllcCellDischarge2(void)
{
    EPWM_forceTripZoneEvent(QABPWM_12_13_BASE, EPWM_TZ_FORCE_EVENT_OST);
    EPWM_forceTripZoneEvent(QABPWM_14_15_BASE, EPWM_TZ_FORCE_EVENT_OST);
}

static inline void HAL_StartPwmCllcCellDischarge2(void)
{
    EPWM_clearTripZoneFlag(QABPWM_12_13_BASE, EPWM_TZ_FORCE_EVENT_OST);
    EPWM_clearTripZoneFlag(QABPWM_14_15_BASE, EPWM_TZ_FORCE_EVENT_OST);
}

static inline void HAL_StopPwmDCDC(void)
{

    EPWM_forceTripZoneEvent(BEG_1_2_BASE, EPWM_TZ_FORCE_EVENT_OST);
}

static inline void HAL_StartPwmDCDC(void)
{
    //Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1 ); /* INT_eFuseBB_XINT_INTERRUPT_ACK_GROUP = INTERRUPT_ACK_GROUP1 defined in CPU2*/
    EPWM_clearTripZoneFlag(BEG_1_2_BASE, EPWM_TZ_FORCE_EVENT_OST );
}

static inline void TurnOffGload_3(void)
{
    EPWM_setActionQualifierAction(EPWM9_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
}

static inline void TurnOnGload_3(void)
{
    EPWM_setActionQualifierAction(EPWM9_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
    EPWM_clearTripZoneFlag(EPWM9_BASE, EPWM_TZ_FORCE_EVENT_OST);
}

static inline void TurnOffGload_4(void)
{
    EPWM_setActionQualifierAction(EPWM9_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
}

static inline void TurnOnGload_4(void)
{
    //Enable interrupt for PWM when a trip occurs from on of the EFUSEs
    EPWM_clearTripZoneFlag(EPWM9_BASE, EPWM_TZ_FLAG_OST | EPWM_TZ_INTERRUPT);
    EPWM_setActionQualifierAction(EPWM9_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
    EPWM_clearTripZoneFlag(EPWM9_BASE, EPWM_TZ_FORCE_EVENT_OST);
}

static inline void HAL_DcdcPulseModePwmSetting(void)
{
    HAL_PWM_setTimeBasePeriod(BEG_1_2_BASE, ( 100000 / BEG_1_2_PULSE_MODE_FREQ_KHZ ) ); /* fsys[KHz] / fpwm[kHz] */
    HAL_PWM_setCounterCompareValue(BEG_1_2_BASE, EPWM_COUNTER_COMPARE_A, ( 100000 / BEG_1_2_PULSE_MODE_FREQ_KHZ ) * 0.005 );
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPB);
    EPWM_setActionQualifierShadowLoadMode(BEG_1_2_BASE, EPWM_ACTION_QUALIFIER_B, EPWM_AQ_LOAD_ON_CNTR_PERIOD);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPB);
    HAL_PWM_setDeadBandDelayMode(BEG_1_2_BASE, EPWM_DB_RED, false);
    HAL_PWM_setDeadBandDelayMode(BEG_1_2_BASE, EPWM_DB_FED, false);
    EPWM_setDeadBandOutputSwapMode(BEG_1_2_BASE, EPWM_DB_OUTPUT_A, false);
    EPWM_setDeadBandOutputSwapMode(BEG_1_2_BASE, EPWM_DB_OUTPUT_B, false);
}


static inline void HAL_DcdcNormalModePwmSetting(void)
{
    HAL_PWM_setTimeBasePeriod(BEG_1_2_BASE, ( 100000 / BEG_1_2_NORMAL_MODE_FREQ_KHZ ) ); /* fsys[KHz] / fpwm[kHz] */
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPB);
    EPWM_setActionQualifierShadowLoadMode(BEG_1_2_BASE, EPWM_ACTION_QUALIFIER_B, EPWM_AQ_LOAD_ON_CNTR_PERIOD);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPB);
    HAL_PWM_setDeadBandDelayMode(BEG_1_2_BASE, EPWM_DB_RED, false);
    HAL_PWM_setDeadBandDelayMode(BEG_1_2_BASE, EPWM_DB_FED, false);
    EPWM_setDeadBandOutputSwapMode(BEG_1_2_BASE, EPWM_DB_OUTPUT_A, false);
    EPWM_setDeadBandOutputSwapMode(BEG_1_2_BASE, EPWM_DB_OUTPUT_B, false);
}


static inline void HAL_DcdcNormalModePwmSettingOri(void)
{
    HAL_PWM_setTimeBasePeriod(BEG_1_2_BASE, 100000 / BEG_1_2_NORMAL_MODE_FREQ_KHZ); /* fsys[KHz] / fpwm[kHz] */
    //HAL_PWM_setCounterCompareValue(BEG_1_2_BASE, EPWM_COUNTER_COMPARE_A, 100000 / BEG_1_2_NORMAL_MODE_FREQ_KHZ * 0.1); /* fsys[KHz] / fpwm[kHz] */
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPB);
    EPWM_setActionQualifierShadowLoadMode(BEG_1_2_BASE, EPWM_ACTION_QUALIFIER_B, EPWM_AQ_LOAD_ON_CNTR_PERIOD);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPB);
    HAL_PWM_setDeadBandDelayMode(BEG_1_2_BASE, EPWM_DB_RED, true);
    HAL_PWM_setDeadBandDelayMode(BEG_1_2_BASE, EPWM_DB_FED, true);
    EPWM_setDeadBandOutputSwapMode(BEG_1_2_BASE, EPWM_DB_OUTPUT_A, true);
    EPWM_setDeadBandOutputSwapMode(BEG_1_2_BASE, EPWM_DB_OUTPUT_B, true);
}

static inline void HAL_DcdcRegulateVoltageAndCurrentModePwmSetting(void)
{
    HAL_PWM_setTimeBasePeriod(BEG_1_2_BASE, 100000 / BEG_1_2_NORMAL_MODE_FREQ_KHZ); /* fsys[KHz] / fpwm[kHz] */
    //HAL_PWM_setCounterCompareValue(BEG_1_2_BASE, EPWM_COUNTER_COMPARE_A, 100000 / BEG_1_2_NORMAL_MODE_FREQ_KHZ * 0.1); /* fsys[KHz] / fpwm[kHz] */
    HAL_PWM_setCounterCompareValue(BEG_1_2_BASE, EPWM_COUNTER_COMPARE_A, ( 100000 / BEG_1_2_PULSE_MODE_FREQ_KHZ ) + 2);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPB);
    EPWM_setActionQualifierShadowLoadMode(BEG_1_2_BASE, EPWM_ACTION_QUALIFIER_B, EPWM_AQ_LOAD_ON_CNTR_PERIOD);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPB);
    HAL_PWM_setDeadBandDelayMode(BEG_1_2_BASE, EPWM_DB_RED, false);
    HAL_PWM_setDeadBandDelayMode(BEG_1_2_BASE, EPWM_DB_FED, false);
    EPWM_setDeadBandOutputSwapMode(BEG_1_2_BASE, EPWM_DB_OUTPUT_A, false);
    EPWM_setDeadBandOutputSwapMode(BEG_1_2_BASE, EPWM_DB_OUTPUT_B, false);
}

static inline void HAL_DcdcRegulateVoltageSyncModePwmSetting(void)
{
    HAL_PWM_setTimeBasePeriod(BEG_1_2_BASE, 100000 / BEG_1_2_NORMAL_MODE_FREQ_KHZ); /* fsys[KHz] / fpwm[kHz] */
    //HAL_PWM_setCounterCompareValue(BEG_1_2_BASE, EPWM_COUNTER_COMPARE_A, 100000 / BEG_1_2_NORMAL_MODE_FREQ_KHZ * 0.1); /* fsys[KHz] / fpwm[kHz] */
    HAL_PWM_setCounterCompareValue(BEG_1_2_BASE, EPWM_COUNTER_COMPARE_A, ( 100000 / BEG_1_2_PULSE_MODE_FREQ_KHZ ) + 2);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_A, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPB);

    EPWM_setActionQualifierShadowLoadMode(BEG_1_2_BASE, EPWM_ACTION_QUALIFIER_B, EPWM_AQ_LOAD_ON_CNTR_PERIOD);

    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_LOW, EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_HIGH, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB);
    EPWM_setActionQualifierAction(BEG_1_2_BASE, EPWM_AQ_OUTPUT_B, EPWM_AQ_OUTPUT_NO_CHANGE, EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPB);

    HAL_PWM_setDeadBandDelayMode(BEG_1_2_BASE, EPWM_DB_RED, true);
    HAL_PWM_setDeadBandDelayMode(BEG_1_2_BASE, EPWM_DB_FED, true);
    EPWM_setDeadBandOutputSwapMode(BEG_1_2_BASE, EPWM_DB_OUTPUT_A, true);
    EPWM_setDeadBandOutputSwapMode(BEG_1_2_BASE, EPWM_DB_OUTPUT_B, true);
}


static inline void HAL_StartPwmCounters(void)
{
    EPWM_setTimeBaseCounterMode(BEG_1_2_BASE, EPWM_COUNTER_MODE_UP);
    EPWM_setTimeBaseCounterMode(QABPWM_12_13_BASE, EPWM_COUNTER_MODE_UP);
    EPWM_setTimeBaseCounterMode(QABPWM_14_15_BASE, EPWM_COUNTER_MODE_UP);
    EPWM_setTimeBaseCounterMode(QABPWM_4_5_BASE, EPWM_COUNTER_MODE_UP);
    EPWM_setTimeBaseCounterMode(QABPWM_6_7_BASE, EPWM_COUNTER_MODE_UP);
    EPWM_setTimeBaseCounterMode(InrushCurrentLimit_BASE, EPWM_COUNTER_MODE_UP);
    EPWM_setTimeBaseCounterMode(EPWMTimer_BASE, EPWM_COUNTER_MODE_UP);

    TurnOffGload_3();
    TurnOffGload_4();
    EPWM_setTimeBaseCounterMode(GLOAD_4_3_BASE, EPWM_COUNTER_MODE_UP);
}

static inline void HAL_StopPwmInrushCurrentLimit(void)
{
    EPWM_forceTripZoneEvent(InrushCurrentLimit_BASE, EPWM_TZ_FORCE_EVENT_OST);
}

static inline void HAL_StartPwmInrushCurrentLimit(void)
{
    EPWM_clearTripZoneFlag(InrushCurrentLimit_BASE, EPWM_TZ_FORCE_EVENT_OST);
}

/*** ADC ***/

static inline void HAL_ADC_clearInterruptStatus(uint32_t base, ADC_IntNumber adcIntNum)
{
    ADC_clearInterruptStatus(base, adcIntNum);
}

static inline void HAL_ADC_clearInterruptOverflowStatus(uint32_t base, ADC_IntNumber adcIntNum)
{
    ADC_clearInterruptOverflowStatus(base, adcIntNum);
}

static inline uint16_t HAL_ADC_readResult(uint32_t resultBase, ADC_SOCNumber socNumber)
{
    return ADC_readResult(resultBase, socNumber);
}

/*** INTERRUPT ***/

static inline void HAL_Interrupt_clearPIE(uint16_t group)
{
    Interrupt_clearACKGroup(group);
}

static inline uint16_t HAL_EPWM_getTimeBasePeriod(uint32_t base)
{
    return EPWM_getTimeBasePeriod(base);
}


#endif /* APP_INC_HAL_H_ */
