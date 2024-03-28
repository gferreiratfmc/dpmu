/**
 *  @brief  timer.h
 *
 *  Created on: 20 dec. 2022
 *
 *  @author vb, us
 */

#ifndef APP_INC_TIMER_H_
#define APP_INC_TIMER_H_

#define PERIOD_1_MS  1
#define PERIOD_10_MS (PERIOD_1_MS * 10)
#define PERIOD_1_S   (PERIOD_1_MS * 1000)

#include <stdint.h>

typedef struct timer_t timer_t;
typedef void(tq_cbfun_t)(timer_t *entry);

struct timer_t
{
    timer_t *next;          // points to next timer in timer queue
    timer_t *prev;          // points to previous timer in timer queue
    const char *name;       // name of this timer
    int32_t delay;          // amount of delay for this timer
    int32_t time;           // number of millisecs left of delay
    bool sleeping;          // true if this timer is sleeping
    bool restart;           // true if elapsed timer should be restarted
    tq_cbfun_t *cbfun;      // function to call when timer elapses
    void *cbdata;           // pass to callback function
};

typedef struct
{
    uint32_t seconds;       // # of seconds since last time set or system reset
    uint32_t can_time;      // # of 100 milliseconds since last time set or system reset
    uint32_t milliseconds;  // # of milliseconds within current second (0..999)
} timer_time_t;

void timerq_init(void);
bool timerq_tick(void);

void timer_init(timer_t *t, int32_t delay, char *name, tq_cbfun_t *cbfun, void *cbdata, bool restart);
void timer_start(timer_t *timer);
void timer_stop(timer_t *timer);
bool timer_elapsed(timer_t *timer);
void timer_reset_ticks(void);
uint32_t timer_get_ticks(void);
uint32_t timer_get_seconds(void);
void timer_get_time(timer_time_t *ptime);
void timer_set_can_time(uint32_t seconds);
uint32_t timer_get_can_time(void);
void timer_blocking_wait(uint32_t dt);

#endif /* APP_INC_TIMER_H_ */
