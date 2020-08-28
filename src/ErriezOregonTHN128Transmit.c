/*
 * MIT License
 *
 * Copyright (c) 2020 Erriez
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*!
 * \file ErriezOregonTHN128Transmit.c
 * \brief Oregon THN128 433MHz temperature transmit library for Arduino
 * \details
 *      Source:         https://github.com/Erriez/ErriezOregonTHN128
 *      Documentation:  https://erriez.github.io/ErriezOregonTHN128
 */

#include <Arduino.h>
#include <util/delay.h>
#include "ErriezOregonTHN128Transmit.h"

/* Function prototypes */
void delay100ms(void) __attribute__((weak));
extern void delay100ms(void);

/* Static variables */
static int8_t _rfTxPort = -1;
static int8_t _rfTxBit = -1;

/* Optimized RF transmit pin macro's */
#define RF_TX_PIN_INIT(rfTxPin)        {            \
    _rfTxPort = digitalPinToPort(rfTxPin);          \
    _rfTxBit = digitalPinToBitMask(rfTxPin);        \
    *portModeRegister(_rfTxPort) |= _rfTxBit;       \
}
#define RF_TX_PIN_DISABLE()             {           \
    if ((_rfTxPort >= 0) && (_rfTxBit >= 0)) {      \
        *portModeRegister(_rfTxPort) &= ~_rfTxBit;  \
    }                                               \
}
#define RF_TX_PIN_HIGH()        { *portOutputRegister(_rfTxPort) |= _rfTxBit; }
#define RF_TX_PIN_LOW()         { *portOutputRegister(_rfTxPort) &= ~_rfTxBit; }


/*!
 * \brief Transmit sync pulse
 */
static void txSync()
{
    /* Transmit sync pulse */
    RF_TX_PIN_HIGH();
    _delay_us(T_SYNC_US);
    RF_TX_PIN_LOW();
    _delay_us(T_SYNC_US);
}

/*!
 * \brief Transmit data bit 0
 */
static void txBit0()
{
    /* Transmit data bit 0 pulse */
    RF_TX_PIN_LOW();
    _delay_us(T_BIT_US);
    RF_TX_PIN_HIGH();
    _delay_us(T_BIT_US);
}

/*!
 * \brief Transmit data bit 1
 */
static void txBit1()
{
    /* Transmit data bit 1 pulse */
    RF_TX_PIN_HIGH();
    _delay_us(T_BIT_US);
    RF_TX_PIN_LOW();
    _delay_us(T_BIT_US);
}

/*!
 * \brief Disable transmit
 */
static void txDisable()
{
    /* Transmit pin low */
    RF_TX_PIN_LOW();
}

/*!
 * \brief Transmit preamble
 */
static void txPreamble()
{
    /* Transmit 12 preamble bits 1 */
    for (uint8_t i = 0; i < 12; i++) {
        txBit1();
    }
    _delay_us(T_PREAMBLE_SPACE_US);
}

/* Transmit 32-bit data */
static void txData(uint32_t data)
{
    /* Transmit 32 data bits */
    for (uint8_t i = 0; i < 32; i++) {
        if (data & (1UL << i)) {
            txBit1();
        } else {
            txBit0();
        }
    }
}

/*------------------------------------------------------------------------------------------------*/
/*                                     Public functions                                           */
/*------------------------------------------------------------------------------------------------*/
/*!
 * \brief Transmit begin
 * \param rfTxPin
 *      Arduino transmit pin
 */
void OregonTHN128_TxBegin(uint8_t rfTxPin)
{
    /* Set RF transmit pin output */
    RF_TX_PIN_INIT(rfTxPin);
}

/*!
 * \brief Disable transmit
 * \details
 *      Set transmit pin to input
 */
void OregonTHN128_TxEnd(void)
{
    /* Set RF transmit pin input */
    RF_TX_PIN_DISABLE();
}

/*!
 * \brief Transmit data
 * \param rawData
 *      32-bit raw data input
 */
void OregonTHN128_TxRawData(uint32_t rawData)
{
    /* Check RF transmit pin initialized */
    if ((_rfTxPort < 0) && (_rfTxBit < 0)) {
        return;
    }

    /* First transmit */
    txPreamble();
    txSync();
    txData(rawData);
    txDisable();

    /* Wait 100ms between frames */
    if (delay100ms != NULL) {
        /* Call 100ms user delay (weak symbol) */
        delay100ms();
    } else {
        /* Use optimized AVR delay */
        _delay_ms(T_SPACE_FRAMES_MS);
    }

    /* Second transmit */
    txPreamble();
    txSync();
    txData(rawData);
    txDisable();
}

/*!
 * \brief Transmit
 *      Transmit data
 * \param data
 *      Oregon THN128 input structure
 */
void OregonTHN128_Transmit(OregonTHN128Data_t *data)
{
    // Convert data structure to 32-bit raw data;
    data->rawData = OregonTHN128_DataToRaw(data);

    // Send raw data
    OregonTHN128_TxRawData(data->rawData);
}