/*
 * rtc.c
 *
 *  Driver for the M41T83 RTC
 *  Created on: 4 okt. 2022
 *      Author: us
 *
 *  From: https://github.com/wdreeveii/avr9p
 *
 *  M41T83 operation code example
 *  Whitham D. Reeve II
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "rtc.h"
#include "board.h"

#define ATOMIC_BLOCK(f)
#define ATOMIC_RESTORESTATE 0

/* Lets start off with defining the date and time bits of the memory mapped
 * registers on the m41t83
 * Page 21 of the m41t83 datasheet has a table with more detail
 */
struct GeneralDateBlock
{
    uint16_t hundsecs     : 4;   // 0x00
    uint16_t tenthsecs    : 4;

    uint16_t secs         : 4;   // 0x01
    uint16_t tsecs        : 3;
    uint16_t st           : 1;

    uint16_t mins         : 4;   // 0x02
    uint16_t tmins        : 3;
    uint16_t placeholder1 : 1;

    uint16_t hours        : 4;   // 0x03
    uint16_t thours       : 2;
    uint16_t CB0          : 1;
    uint16_t CB1          : 1;

    uint16_t DayOfWeek    : 3;   // 0x04
    uint16_t VBATEN       : 1;
    uint16_t Vbat         : 1;
    uint16_t OSCON        : 1;
    uint16_t placeholder2 : 2;

    uint16_t DayOfMonth   : 4;   // 0x05
    uint16_t tDayOfMonth  : 2;
    uint16_t placeholder3 : 2;

    uint16_t Month        : 4;   // 0x06
    uint16_t tMonth       : 1;
    uint16_t LP           : 1;
    uint16_t placeholder4 : 2;

    uint16_t Year         : 4;   // 0x07
    uint16_t tYear        : 4;
};

_Static_assert(sizeof(struct GeneralDateBlock) == 4, "Bad size of GeneralDataBlock");

/* Calculate the number of leap years that happen before the specified date.
 * Takes one argument the year to count leap years up to.
 * Returns the number of leap years that have happened before the specified date
 */
uint32_t leapYearsBefore(uint32_t year)
{
    year--;
    return (year / (4ul)) - (year / (100ul)) + (year / (400ul));
}

/* Test whether the provided year is a leap year
 * Takes one argument the year to be tested
 * returns true if the specified year is a leap year, other wise false.
 */
char isLeapYear(uint32_t year)
{
    if (year % 400 == 0)
        return 1;
    else if (year % 100 == 0)
        return 0;
    else if (year % 4 == 0)
        return 1;
    else
        return 0;
}
/* int set_time(time_t timestamp)
 * Convert a unix timestamp into the m41t83 memory block format and save to device
 * Takes one argument the timestamp in the number of seconds since midnight January 1 1970.
 * Returns -1 on error
 */
int set_time(time_t timestamp)
{
    struct GeneralDateBlock dateblock;
    int rv = 0;

    uint8_t month_lens[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint8_t month_index = 0;

    uint32_t total_days = timestamp / (86400ul);
    uint32_t rem_secs = timestamp % (86400ul);

    uint32_t guess_leap_years = leapYearsBefore(1970ul + (total_days / (365ul))) - leapYearsBefore(1970ul);
    uint32_t total_years = (1970ul) + ((total_days - guess_leap_years) / (365ul));
    uint32_t rem_days = ((total_days - guess_leap_years) % 365ul);

    dateblock.tYear = (total_years % (100ul)) / 10ul;
    dateblock.Year = (total_years % (100ul)) % 10ul;

    if (isLeapYear(total_years))
        month_lens[1] = 29;

    for (; month_index < 12 && rem_days > month_lens[month_index]; month_index++)
        rem_days -= month_lens[month_index];

    dateblock.tMonth = (month_index + 1) / 10;
    dateblock.Month = (month_index + 1) % 10;

    dateblock.tDayOfMonth = (rem_days) / 10;
    dateblock.DayOfMonth = (rem_days) % 10;

    dateblock.thours = (rem_secs / 3600) / 10;
    dateblock.hours = (rem_secs / 3600) % 10;

    rem_secs = (rem_secs % 3600);

    dateblock.tmins = (rem_secs / 60) / 10;
    dateblock.mins = (rem_secs / 60) % 10;

    rem_secs = (rem_secs % 60);

    dateblock.tsecs = rem_secs / 10;
    dateblock.secs = rem_secs % 10;
    dateblock.st = 1;
    dateblock.VBATEN = 1;
    rv = i2c_write_bytes(0x00, 7, (void *)&dateblock);
    if (rv == -1)
        printf("RTC set_time WRITE ERROR\n");
    return 0;
}

/* time_t get_time()
 * Return the current unix time supplied by the RTC.
 * Reads the m41t83 date/time block from the device and converts into unix time.
 * Takes no arguments
 * Returns the number of seconds since midnight January 1 1970
 */
time_t _get_time()
{
    uint8_t month_lens[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int rv = 0;
    struct GeneralDateBlock dateblock;

    rv = i2c_read_bytes(0x00, 7, (void *)(&dateblock));
    if (rv == -1)
        printf("RTC time READ ERROR\n");

    uint32_t year = 2000 + dateblock.Year + (dateblock.tYear * 10);
    uint32_t year_days = ((year - (1970ul)) * (365ul)) + (leapYearsBefore(year) - leapYearsBefore(1970ul));

    uint8_t month = dateblock.Month + (dateblock.tMonth * (10ul));
    if (isLeapYear(year))
        month_lens[1] = 29;

    uint16_t month_days = 0;
    for (rv = 1; rv < month; rv++)
        month_days += month_lens[rv - 1];
    uint32_t total_days = year_days + month_days + dateblock.DayOfMonth + (dateblock.tDayOfMonth * (10ul));
    uint32_t total_secs = (total_days * (86400ul)) + ((dateblock.hours + (dateblock.thours * (10ul))) * (3600ul)) +
        ((dateblock.mins + (dateblock.tmins * (10ul))) * (60ul)) + (dateblock.secs + (dateblock.tsecs * (10ul)));

    return total_secs;
}

/* time_t time()
 * get a copy of the current time stored in the global variable.
 * Returns a cached copy of the number of seconds since midnight Jan 1 1970
 */
time_t current_time_global;
time_t get_time()
{
    time_t copy;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        copy = current_time_global;
    }
    return copy;
}

void mcp7940_start()
{
    if (rtc_check_start()) {
        printf("Resetting..\n");
        rtc_start_reset();
    }
    if (rtc_check_vbat()) {
        printf("Enabling external battery..\n");
        rtc_vbat_enable();
    }
    if (rtc_check_config()) {
        printf("Setting RTC config..\n");
        rtc_mk_config();
    }
}

void RTC_Init(void)
{
#if 0
    /* frequency = cpuspeed / (16 + 2(TWBR) * 4^(TWSR)) */
    /* 50000 = 20000000 / (16 + 2(48) * 4^1) */

    /* set the prescaler to 4^1 */
    TWSR = 1;

    TWBR = 40;

    // configure pin change interrupt for the square wave output from the rtc
    // enable PCIE2
    PCICR = (1 << 2);
    PCMSK2 = (1 << 2);

    PORTC &= ~(1 << 2);
    DDRC &= ~(1 << 2);

    current_time_global = get_time();
    mcp7940_start();
#endif
}

void print_time(void)
{
#if 0
    time_t timestamp = time();
    uint8_t datestring[16];
    ultoa(timestamp, (char *)datestring, 10);
    USART_Send(0, datestring, strlen((char *)datestring));
    USART_Send(0, (uint8_t *)"\n", 1);
#endif
}

#if 0
/* ISR Pin Change Interrupt 1
 * NOBLOCK because there is a lot to do here.
 * Execute everything that needs to happen once a second
 */
ISR(PCINT2_vect, ISR_NOBLOCK)
{
    time_t copy;
    // issue print request
    // The square wave is like any square wave,
    // and thus it changes value twice in 1 period.
    if (PINC & (1 << 2)) {
        copy = _get_time();
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
            current_time_global = copy;
        }
        print_time();
        mucron_tick();
        soft_timer_tick();
        // DO WHAT EVER YOU NEED TO DO ONCE A SECOND HERE
    } else {
    }   // do nothing
}
#endif

/* uint8_t rtc_check_vbat(void)
 * Ask the rtc chip if the vbat power source is enabled
 */
uint8_t rtc_check_vbat(void)
{
    int rv = 0;
    uint8_t databuffer[1];
    rv = i2c_read_bytes(0x03, 1, databuffer);
    if (rv == -1) {
        printf("RTC check_vbat READ ERROR\n");
        return 1;
    }
    if (!(databuffer[0] & (1 << 3))) {
        printf("WARNING: VBAT NOT ENABLED\n");
        return 1;
    }
    return 0;
}

/* void rtc_vbat_enable(void)
 * Tell the rtc chip to enable battery power
 */
void rtc_vbat_enable(void)
{
    int rv = 0;
    uint8_t databuffer[1];
    rv = i2c_read_bytes(0x03, 1, databuffer);
    if (rv == -1)
        printf("RTC VBAT enable READ ERROR\n");
    databuffer[0] |= (1 << 3);
    rv = i2c_write_bytes(0x03, 1, databuffer);
    if (rv == -1)
        printf("RTC VBAT enable WRITE ERROR\n");
}

/* void rtc_mk_config(void)
 * Setup configuration options in the RTC chip
 */
void rtc_mk_config(void)
{
    int rv = 0;
    uint8_t databuffer[1];
    databuffer[0] = (1 << 6) | (1 << 3);
    rv = i2c_write_bytes(0x07, 1, databuffer);
    if (rv == -1)
        printf("RTC config WRITE ERROR\n");
}

/* uint8_t rtc_check_config(void)
 * Ask the rtc chip for the configuration settings
 */
uint8_t rtc_check_config(void)
{
    int rv = 0;
    uint8_t databuffer[1];
    rv = i2c_read_bytes(0x07, 1, databuffer);
    if (rv == -1) {
        printf("RTC check_config READ ERROR\n");
        return 1;
    }
    if (databuffer[0] != ((1 << 6) | (1 << 3))) {
        rv = databuffer[0];
        printf("%x\n", rv);
        printf("WARNING: Improper configuration.\n");
        return 1;
    }
    return 0;
}

/* uint8_t rtc_check_stop(void)
 * Ask the rtc chip if the oscillator is started
 */
uint8_t rtc_check_start(void)
{
    int rv = 0;
    uint8_t databuffer[1];
    rv = i2c_read_bytes(0x00, 1, databuffer);
    if (rv == -1) {
        printf("RTC check_stop READ ERROR\n");
        return 1;
    }
    if (!(databuffer[0] & 0x80)) {
        printf("WARNING: START BIT NOT SET\n");
        return 1;
    }
    return 0;
}

/* void rtc_start_reset(void)
 * Tell the rtc chip to start the oscillator
 */
void rtc_start_reset(void)
{
    int rv = 0;
    uint8_t databuffer[1];
    // pump the oscillator
    databuffer[0] = 0x80;
    rv = i2c_write_bytes(0x00, 1, databuffer);
    if (rv == -1)
        printf("RTC stop_reset WRITE ERROR\n");
}

/* BEGIN IIC support code */

#define I2C_SLAVE_ADDR 0xDE

// bus cycles to wait before timeout on lengthy operations like write
#define MAX_ITER 200

/*
 * Number of bytes that can be written in a row.
 */
#define PAGE_SIZE 8

/*
 * Saved TWI status register.
 */
uint8_t twst;

/*
 * Note [7]
 *
 * Read "len" bytes from EEPROM starting at "eeaddr" into "buf".
 *
 * This requires two bus cycles: during the first cycle, the device
 * will be selected (master transmitter mode), and the address
 * transfered.
 * Address bits exceeding 256 are transfered in the
 * E2/E1/E0 bits (subaddress bits) of the device selector.
 * Address is sent in two dedicated 8 bit transfers
 * for 16 bit address devices (larger EEPROM devices)
 *
 * The second bus cycle will reselect the device (repeated start
 * condition, going into master receiver mode), and transfer the data
 * from the device to the TWI master.  Multiple bytes can be
 * transfered by ACKing the client's transfer.  The last transfer will
 * be NACKed, which the client will take as an indication to not
 * initiate further transfers.
 */
int i2c_read_bytes(uint16_t eeaddr, int len, uint8_t *buf)
{
    uint8_t sla, twcr, n = 0;
    int rv = 0;

#ifndef WORD_ADDRESS_16BIT
    /* patch high bits of EEPROM address into SLA */
    sla = I2C_SLAVE_ADDR | (((eeaddr >> 8) & 0x07) << 1);
#else
    /* 16-bit address devices need only TWI Device Address */
    sla = I2C_SLAVE_ADDR;
#endif

/*
   * Note [8]
   * First cycle: master transmitter mode
   */
restart:
    if (n++ >= MAX_ITER)
        return -1;
begin:

    I2C_sendStartCondition(I2C_BUS_BASE);

    TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN); /* send start condition */
    while ((TWCR & _BV(TWINT)) == 0)
        ; /* wait for transmission */
    switch ((twst = TW_STATUS)) {
    case TW_REP_START: /* OK, but should not happen */
    case TW_START:
        break;

    case TW_MT_ARB_LOST: /* Note [9] */
        goto begin;

    default:
        return -1; /* error: not in start condition */
                   /* NB: do /not/ send stop condition */
    }

    /* Note [10] */
    /* send SLA+W */
    TWDR = sla | TW_WRITE;
    TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
    while ((TWCR & _BV(TWINT)) == 0)
        ; /* wait for transmission */
    switch ((twst = TW_STATUS)) {
    case TW_MT_SLA_ACK:
        break;

    case TW_MT_SLA_NACK: /* nack during select: device busy writing */
                         /* Note [11] */
        goto restart;

    case TW_MT_ARB_LOST: /* re-arbitrate */
        goto begin;

    default:
        goto error; /* must send stop condition */
    }

#ifdef WORD_ADDRESS_16BIT
    TWDR = (eeaddr >> 8);          /* 16-bit word address device, send high 8 bits of addr */
    TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
    while ((TWCR & _BV(TWINT)) == 0)
        ; /* wait for transmission */
    switch ((twst = TW_STATUS)) {
    case TW_MT_DATA_ACK:
        break;

    case TW_MT_DATA_NACK:
        goto quit;

    case TW_MT_ARB_LOST:
        goto begin;

    default:
        goto error; /* must send stop condition */
    }
#endif

    TWDR = eeaddr;                 /* low 8 bits of addr */
    TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
    while ((TWCR & _BV(TWINT)) == 0)
        ; /* wait for transmission */
    switch ((twst = TW_STATUS)) {
    case TW_MT_DATA_ACK:
        break;

    case TW_MT_DATA_NACK:
        goto quit;

    case TW_MT_ARB_LOST:
        goto begin;

    default:
        goto error; /* must send stop condition */
    }

    /*
   * Note [12]
   * Next cycle(s): master receiver mode
   */
    TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN); /* send (rep.) start condition */
    while ((TWCR & _BV(TWINT)) == 0)
        ; /* wait for transmission */
    switch ((twst = TW_STATUS)) {
    case TW_START: /* OK, but should not happen */
    case TW_REP_START:
        break;

    case TW_MT_ARB_LOST:
        goto begin;

    default:
        goto error;
    }

    /* send SLA+R */
    TWDR = sla | TW_READ;
    TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
    while ((TWCR & _BV(TWINT)) == 0)
        ; /* wait for transmission */
    switch ((twst = TW_STATUS)) {
    case TW_MR_SLA_ACK:
        break;

    case TW_MR_SLA_NACK:
        goto quit;

    case TW_MR_ARB_LOST:
        goto begin;

    default:
        goto error;
    }

    for (twcr = _BV(TWINT) | _BV(TWEN) | _BV(TWEA) /* Note [13] */; len > 0; len--) {
        if (len == 1)
            twcr = _BV(TWINT) | _BV(TWEN); /* send NAK this time */
        TWCR = twcr;                       /* clear int to start transmission */
        while ((TWCR & _BV(TWINT)) == 0)
            ; /* wait for transmission */
        switch ((twst = TW_STATUS)) {
        case TW_MR_DATA_NACK:
            len = 0; /* force end of loop */
                     /* FALLTHROUGH */
        case TW_MR_DATA_ACK:
            *buf++ = TWDR;
            rv++;
            if (twst == TW_MR_DATA_NACK)
                goto quit;
            break;

        default:
            goto error;
        }
    }
quit:
    /* Note [14] */
    TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN); /* send stop condition */

    return rv;

error:
    rv = -1;
    goto quit;
}

/*
 * Write "len" bytes into EEPROM starting at "eeaddr" from "buf".
 *
 * This is a bit simpler than the previous function since both, the
 * address and the data bytes will be transfered in master transmitter
 * mode, thus no reselection of the device is necessary.  However, the
 * EEPROMs are only capable of writing one "page" simultaneously, so
 * care must be taken to not cross a page boundary within one write
 * cycle.  The amount of data one page consists of varies from
 * manufacturer to manufacturer: some vendors only use 8-byte pages
 * for the smaller devices, and 16-byte pages for the larger devices,
 * while other vendors generally use 16-byte pages.  We thus use the
 * smallest common denominator of 8 bytes per page, declared by the
 * macro PAGE_SIZE above.
 *
 * The function simply returns after writing one page, returning the
 * actual number of data byte written.  It is up to the caller to
 * re-invoke it in order to write further data.
 */
int i2c_write_page(uint16_t eeaddr, int len, uint8_t *buf)
{
    uint8_t sla, n = 0;
    int rv = 0;
    uint16_t endaddr;

    if (eeaddr + len <= (eeaddr | (PAGE_SIZE - 1)))
        endaddr = eeaddr + len;
    else
        endaddr = (eeaddr | (PAGE_SIZE - 1)) + 1;
    len = endaddr - eeaddr;

#ifndef WORD_ADDRESS_16BIT
    /* patch high bits of EEPROM address into SLA */
    sla = I2C_SLAVE_ADDR | (((eeaddr >> 8) & 0x07) << 1);
#else
    /* 16-bit address devices need only TWI Device Address */
    sla = I2C_SLAVE_ADDR;
#endif

restart:
    if (n++ >= MAX_ITER)
        return -1;
begin:

    /* Note [15] */
    TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN); /* send start condition */
    while ((TWCR & _BV(TWINT)) == 0)
        ; /* wait for transmission */
    switch ((twst = TW_STATUS)) {
    case TW_REP_START: /* OK, but should not happen */
    case TW_START:
        break;

    case TW_MT_ARB_LOST:
        goto begin;

    default:
        return -1; /* error: not in start condition */
                   /* NB: do /not/ send stop condition */
    }

    /* send SLA+W */
    TWDR = sla | TW_WRITE;
    TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
    while ((TWCR & _BV(TWINT)) == 0)
        ; /* wait for transmission */
    switch ((twst = TW_STATUS)) {
    case TW_MT_SLA_ACK:
        break;

    case TW_MT_SLA_NACK: /* nack during select: device busy writing */
        goto restart;

    case TW_MT_ARB_LOST: /* re-arbitrate */
        goto begin;

    default:
        goto error; /* must send stop condition */
    }

#ifdef WORD_ADDRESS_16BIT
    TWDR = (eeaddr >> 8);          /* 16 bit word address device, send high 8 bits of addr */
    TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
    while ((TWCR & _BV(TWINT)) == 0)
        ; /* wait for transmission */
    switch ((twst = TW_STATUS)) {
    case TW_MT_DATA_ACK:
        break;

    case TW_MT_DATA_NACK:
        goto quit;

    case TW_MT_ARB_LOST:
        goto begin;

    default:
        goto error; /* must send stop condition */
    }
#endif

    TWDR = eeaddr;                 /* low 8 bits of addr */
    TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
    while ((TWCR & _BV(TWINT)) == 0)
        ; /* wait for transmission */
    switch ((twst = TW_STATUS)) {
    case TW_MT_DATA_ACK:
        break;

    case TW_MT_DATA_NACK:
        goto quit;

    case TW_MT_ARB_LOST:
        goto begin;

    default:
        goto error; /* must send stop condition */
    }

    for (; len > 0; len--) {
        TWDR = *buf++;
        TWCR = _BV(TWINT) | _BV(TWEN); /* start transmission */
        while ((TWCR & _BV(TWINT)) == 0)
            ; /* wait for transmission */
        switch ((twst = TW_STATUS)) {
        case TW_MT_DATA_NACK:
            goto error; /* device write protected -- Note [16] */

        case TW_MT_DATA_ACK:
            rv++;
            break;

        default:
            goto error;
        }
    }
quit:
    TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN); /* send stop condition */

    return rv;

error:
    rv = -1;
    goto quit;
}

/*
 * Wrapper around ee24xx_write_page() that repeats calling this
 * function until either an error has been returned, or all bytes
 * have been written.
 */
int i2c_write_bytes(uint16_t eeaddr, int len, uint8_t *buf)
{
    int rv, total;

    total = 0;
    do {
        rv = i2c_write_page(eeaddr, len, buf);

        if (rv == -1)
            return -1;
        eeaddr += rv;
        len -= rv;
        buf += rv;
        total += rv;
    } while (len > 0);

    return total;
}
