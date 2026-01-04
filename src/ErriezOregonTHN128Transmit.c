/*
 * MIT License
 *
 * Copyright (c) 2020-2026 Erriez
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
#include "ErriezOregonTHN128Transmit.h"

/* Function prototypes */
void delay100ms(void) __attribute__((weak));
extern void delay100ms(void);

/* Static variables */
#if defined(ARDUINO_ARCH_AVR)
#include <util/delay.h>

static int8_t _rfTxPort = -1;
static int8_t _rfTxBit = -1;

/*!
 * \defgroup Transmit pin control
 * \details
 *      Optimized for AVR microcontrollers
 * @{
 */

/*!
 * \def RF_TX_PIN_INIT(rfTxPin)
 * \brief Initialize RF transmit pin
 * \param rfTxPin
 *      TX pin to external interrupt pin (INT0 or INT1)
 */
#define RF_TX_PIN_INIT(rfTxPin)        {            \
    _rfTxPort = digitalPinToPort(rfTxPin);          \
    _rfTxBit = digitalPinToBitMask(rfTxPin);        \
    *portModeRegister(_rfTxPort) |= _rfTxBit;       \
}

/*!
 * \def RF_TX_PIN_DISABLE()
 * \brief TX pin disable
 */
#define RF_TX_PIN_DISABLE()             {           \
    if ((_rfTxPort >= 0) && (_rfTxBit >= 0)) {      \
        *portModeRegister(_rfTxPort) &= ~_rfTxBit;  \
    }                                               \
}

/*!
 * \def IS_RF_TX_PIN_INITIALIZED()
 * \brief Check if pin is initialized
 * \retval True: RF TX pin initialized, False: RF TX pin not initialized
 */
#define IS_RF_TX_PIN_INITIALIZED()  ((_rfTxPort >= 0) && (_rfTxBit >= 0))

/*!
 * \def RF_TX_PIN_HIGH()
 * \brief TX pin high
 */
#define RF_TX_PIN_HIGH()            { *portOutputRegister(_rfTxPort) |= _rfTxBit; }

/*!
 * \def RF_TX_PIN_LOW()
 * \brief TX pin low
 */
#define RF_TX_PIN_LOW()             { *portOutputRegister(_rfTxPort) &= ~_rfTxBit; }

/*!
 * \def RF_TX_DELAY_US
 * \brief Optimized AVR delay in us
 */
#define RF_TX_DELAY_US(us)          _delay_us(us)

/*!
 * \def RF_TX_DELAY_MS
 * \brief Optimized AVR delay in ms
 */
#define RF_TX_DELAY_MS(ms)          _delay_ms(ms)

#elif defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
static int8_t _rfTxPin = -1;

/*!
 * \def RF_TX_PIN_INIT(rfTxPin)
 * \brief Initialize RF transmit pin
 * \param rfTxPin
 *      TX pin to external interrupt pin (INT0 or INT1)
 */
#define RF_TX_PIN_INIT(rfTxPin)  {                  \
    _rfTxPin= rfTxPin;                              \
    pinMode(_rfTxPin, OUTPUT);                      \
}

/*!
 * \def RF_TX_PIN_DISABLE()
 * \brief TX pin disable
 */
#define RF_TX_PIN_DISABLE() {                       \
    if (_rfTxPin >= 0) {                            \
        pinMode(_rfTxPin, INPUT);                   \
    }                                               \
}

/*!
 * \def IS_RF_TX_PIN_INITIALIZED()
 * \brief Check if pin is initialized
 * \retval True: RF TX pin initialized, False: RF TX pin not initialized
 */
#define IS_RF_TX_PIN_INITIALIZED()  (_rfTxPin >= 0)

/*!
 * \def RF_TX_PIN_HIGH()
 * \brief TX pin high
 */
#define RF_TX_PIN_HIGH()            { digitalWrite(_rfTxPin, HIGH); }

/*!
 * \def RF_TX_PIN_LOW()
 * \brief TX pin low
 */
#define RF_TX_PIN_LOW()             { digitalWrite(_rfTxPin, LOW); }

/*!
 * \def RF_TX_DELAY_US
 * \brief Generic delay in us
 */
#define RF_TX_DELAY_US(us)          delayMicroseconds(us)

/*!
 * \def RF_TX_DELAY_MS
 * \brief Generic delay in ms
 */
#define RF_TX_DELAY_MS(ms)          delay(ms)
#else
#error "May work, but not tested on this target"
#endif

/*! @} */


/*!
 * \brief Transmit sync pulse
 */
static void txSync()
{
    /* Transmit sync pulse */
    RF_TX_PIN_HIGH();
    RF_TX_DELAY_US(T_SYNC_US);
    RF_TX_PIN_LOW();
    RF_TX_DELAY_US(T_SYNC_US);
}

/*!
 * \brief Transmit data bit 0
 */
static void txBit0()
{
    /* Transmit data bit 0 pulse */
    RF_TX_PIN_LOW();
    RF_TX_DELAY_US(T_BIT_US);
    RF_TX_PIN_HIGH();
    RF_TX_DELAY_US(T_BIT_US);
}

/*!
 * \brief Transmit data bit 1
 */
static void txBit1()
{
    /* Transmit data bit 1 pulse */
    RF_TX_PIN_HIGH();
    RF_TX_DELAY_US(T_BIT_US);
    RF_TX_PIN_LOW();
    RF_TX_DELAY_US(T_BIT_US);
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
    RF_TX_DELAY_US(T_PREAMBLE_SPACE_US);
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
 * \details
 *      Connect rfTxPin to any DIGITAL pin
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
    if (!IS_RF_TX_PIN_INITIALIZED()) {
        return;
    }

    /* Transmit */
    txPreamble();
    txSync();
    txData(rawData);
    txDisable();
}

/*!
 * \brief Transmit
 *      Transmit data
 * \details
 *      The application should call OregonTHN128_TxRawData() twice at 100ms interval.
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