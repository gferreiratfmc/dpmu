/**
 *  @file timer.c
 *
 *  @author vb
 *
 *  Created on: 20 dec. 2022
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#include "device.h"
#include "timer.h"

typedef void (*pfcb)(void *);

typedef struct {
    pfcb callback;
} async_wait_t;

static volatile uint32_t ticks = 0;
static uint32_t prev_ticks = 0;

static volatile uint32_t secs = 0;

static volatile uint32_t can_time = 0;

#define can_time_period 100 /* in milliseconds */
#define secs_timer_period_retive_can_timer (1000 / can_time_period)

static timer_t timerq;

__interrupt void INT_myCPUTIMER2_ISR(void)
{
    // Use intrinsic function to increment ticks atomically.
    __addl((long*) &ticks, 1);

//    if ((ticks % 1000) == 0) {
//        // Use intrinsic function to increment secs atomically.
//        __addl((long*) &secs, 1);

    if ((ticks % can_time_period) == 0) {
        // Use intrinsic function to increment can_time atomically.
        __addl((long*) &can_time, 1);

        if(can_time % secs_timer_period_retive_can_timer)  {
            // Subtract secs from itself to clear it atomically.
            __subl((long *)&secs, (long)secs);

            /* Set seconds to one tenth of can_time -> timers syncronized */
            __addl((long*) &secs,
                           can_time / secs_timer_period_retive_can_timer);
        }
        else {
            asm("  RPT #2 || NOP"); /* clock increment time consistent */
        }
    } else {
        asm("  RPT #5 || NOP");     /* clock increment time consistent */
    }
}

//void timer_enable(void)
//{
//    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
//
//    // Enable sync and clock to PWM
//    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
//}

void timer_reset_ticks(void)
{
    prev_ticks = 0;

    // Subtract ticks from itself to clear it atomically.
    __subl((long *)&ticks, (long)ticks);

    // Subtract secs from itself to clear it atomically.
    __subl((long *)&can_time, (long)can_time);

    // Subtract secs from itself to clear it atomically.
    __subl((long *)&secs, (long)secs);
}

uint32_t timer_get_ticks(void)
{
    uint32_t tmp = 0;

    // Use intrinsic function to read ticks atomically.
    __addl((long *)&tmp, (long)ticks);

    return tmp;
}

uint32_t timer_get_seconds(void)
{
    uint32_t tmp = 0;

    // Use intrinsic function to read secs atomically.
    __addl((long *)&tmp, (long)secs);

    return tmp;
}

uint32_t timer_get_can_time(void)
{
    uint32_t tmp = 0;

    // Use intrinsic function to read secs atomically.
    __addl((long *)&tmp, (long)can_time);

    return tmp;
}

void timer_get_time(timer_time_t *ptime)
{
    ptime->milliseconds = 0;
    ptime->can_time = 0;
    ptime->seconds = 0;

    __addl((long *)&ptime->milliseconds, (long)ticks);
    __addl((long *)&ptime->can_time, (long)can_time);
    __addl((long *)&ptime->seconds, (long)secs);

    if (ptime->milliseconds != ticks) {
        __addl((long *)&ptime->milliseconds, (long)ticks);
        __addl((long *)&ptime->can_time, (long)can_time);
        __addl((long *)&ptime->seconds, (long)secs);
    }
}

void timer_set_can_time(uint32_t can_new_time)
{
    timer_reset_ticks();

    /* set can_time */
    __addl((long*) &can_time, can_new_time);

    /* any shew will will be handled in next increment of can_time */
    __addl((long*) &secs, can_new_time / secs_timer_period_retive_can_timer);
}

static inline void timer_remove_from_queue(timer_t *p)
{
    // Unlink timer from queue.
    p->next->prev = p->prev;
    p->prev->next = p->next;

    p->sleeping = 0;
    if (p->next->time != INT_MAX)
        p->next->time += p->time;
    p->time = 0;
}

void timerq_init(void)
{
    timerq.next = timerq.prev = &timerq;
    timerq.time = INT_MAX;
}

bool timerq_tick(void)
{
    timer_t *p;

    uint32_t tnow = timer_get_ticks();

    if (timerq.next != &timerq) {
        if (tnow != prev_ticks) {
            prev_ticks = tnow;

            p = (timer_t *)timerq.next;

            p->time--;

            while (p->time <= 0) {
                // Timer elapsed, unlink from queue.
                p->next->prev = p->prev;
                p->prev->next = p->next;

                p->time = 0;
                p->sleeping = 0;
                if (p->restart) {
                    timer_start(p);
                }

                if (p->cbfun != NULL) {
                    p->cbfun(p);
                }

                p = p->next;
            }
        }

        return true;
    }

    return false;
}

void timer_init(timer_t *t, int32_t delay, char *name, tq_cbfun_t *cbfun, void *cbdata, bool restart)
{
    t->name = name;
    t->delay = delay;
    t->sleeping = false;
    t->restart = restart;
    t->cbfun = cbfun;
    t->cbdata = cbdata;
}

void timer_start(timer_t *timer)
{
    int32_t t = timer->delay;

    timer->sleeping = true;

    timer_t *q = timerq.next;

    while ((t -= q->time) > 0)
        q = q->next;

    timer->time = (t += q->time);

    if (q->next != timerq.next)
        q->time -= t;

    timer->next = q;
    timer->prev = q->prev;

    q->prev->next = timer;
    q->prev = timer;

#if 0
    q = timerq.next;
    do {
        printf("%s ", q->name);
        q = q->next;
    } while (q != &timerq);
    printf("\n");
#endif
}

void timer_stop(timer_t *timer)
{
    timer_remove_from_queue(timer);
}

bool timer_elapsed(timer_t *timer)
{
    return false == timer->sleeping;
}

void timer_blocking_wait(uint32_t dt)
{
    uint32_t now = timer_get_ticks();

    while (timer_get_ticks() - now < dt);
}
