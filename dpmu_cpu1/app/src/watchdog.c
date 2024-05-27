/*
 * watchdog.c
 *
 *  Created on: 29 jan. 2024
 *      Author: Henrik Borg henrik.borg@ekpower.se hb
 */


#include <driverlib.h>

#include "watchdog.h"

void watchdog_init(void)
{
#ifdef USE_WATCHDOG
    /* configure and start the watchdog */
    SysCtl_enableWatchdogReset();
    SysCtl_resetWatchdog();

    /* we want a reset, not an interrupt */
    SysCtl_setWatchdogMode(SYSCTL_WD_MODE_RESET);

    /* total down-scaling of WD clock is
     * WDCLK = INTOSC1 / (SYSCTL_WD_PREDIV_4096 * SYSCTL_WD_PRESCALE_64)
     * */
    SysCtl_setWatchdogPredivider(SYSCTL_WD_PREDIV_4096);    // 4096 is maximum
    SysCtl_setWatchdogPrescaler(SYSCTL_WD_PRESCALE_64);     //   64 is maximum
//    Interrupt_enable(INT_WAKE);   /* we want a reset, not an interrupt */
    SysCtl_serviceWatchdog();
    SysCtl_enableWatchdog();
#endif
}

void watchdog_feed(void)
{
#ifdef USE_WATCHDOG
    SysCtl_serviceWatchdog();
#endif
}
