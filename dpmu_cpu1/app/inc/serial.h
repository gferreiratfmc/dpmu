/*
 * serial.h
 *
 *  Created on: 30 mars 2022
 *      Author: us
 */

#ifndef SERIAL_H_
#define SERIAL_H_

// Experimental use of ETL. Comment to use ordinary fifo.
//#define USE_ETL_CIRCULAR_BUFFER

#include <stdint.h>
#include <stdbool.h>
#include <sci.h>

#define FIFO_MASK    (0xf)
#define FIFO_BUFSIZE (FIFO_MASK + 1)

struct SerialSettings
{
    uint32_t interruptNumberRx;
    uint16_t interruptAckGroupRx;
    uint32_t interruptNumberTx;
    uint16_t interruptAckGroupTx;
    uint32_t sciBase;
    uint32_t baudrate;
    uint32_t lspclk;
    uint32_t cfg;
};

#define PRINTBUFSIZE 300

struct Serial
{
    const struct SerialSettings *_settings;

    bool _open;
    volatile bool tx_active;
    char pbuf[PRINTBUFSIZE + 1];   // the output buffer for printf member function

    char tx_buf[FIFO_BUFSIZE];
    char rx_buf[FIFO_BUFSIZE];

    volatile unsigned int tx_fifo_in;
    volatile unsigned int tx_fifo_out;
    volatile unsigned int tx_fifo_len;

    volatile unsigned int rx_fifo_in;
    volatile unsigned int rx_fifo_out;
    volatile unsigned int rx_fifo_len;
};

enum {
    DEBUG_NONE = 0,
    DEBUG_CRITICAL,
    DEBUG_ERROR,
    DEBUG_INFO,
};

extern int debug_level;

void Serial_ctor(struct Serial *dev);
bool Serial_is_open(struct Serial *dev);
bool Serial_open(struct Serial *dev);
void Serial_close(struct Serial *dev);

int Serial_write(struct Serial *dev, const char *buf, int count);
//static int Serial_putchar(struct Serial *dev, char ch);
int Serial_getchar(struct Serial *dev);
bool Serial_xmit_ready(struct Serial *dev);
int Serial_read(struct Serial *dev, char *buf, int count);
//static bool Serial_recv_ready(struct Serial *dev);

int Serial_printf(struct Serial *dev, const char *fmt, ...);
int Serial_debug(uint16_t debugLevel, struct Serial *dev, const char *fmt, ...);
void Serial_set_debug_level(int requested_debug_level);



void Serial_rx_isr(struct Serial *dev);
void Serial_tx_isr(struct Serial *dev);
#endif /* SERIAL_H_ */
