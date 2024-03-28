/**
 *  @file timer.c
 *
 *  @author vb
 *
 *  Created on: 20 dec. 2022
 */

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "device.h"
#include "timer.h"

typedef void (*pfcb)(void *);

typedef struct {
    pfcb callback;
} async_wait_t;

static volatile uint32_t ticks_vol = 0;
static uint32_t prev_ticks = 0;
static uint32_t ticks = 0;
static timer_t timerq;

__interrupt void INT_myCPUTIMER2_ISR(void)
{
    //    ticks_vol++;

    // Use intrinsic function to increment ticks_vol atomically. (Don't know if this is necessary)
    __addl((long *)&ticks_vol, 1);
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
    ticks = 0;
    prev_ticks = 0;

    // Subtract ticks_vol from itself to clear it atomically. (NEEDS TESTING!)
    __subl((long *)&ticks_vol, (long)ticks_vol);
}

uint32_t timer_get_ticks(void)
{
    uint32_t tmp = 0;

    // Use intrinsic function to read ticks_vol atomically. (NEEDS TESTING!)
    __addl((long *)&tmp, (long)ticks_vol);
    ticks = tmp;

    return ticks;
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
