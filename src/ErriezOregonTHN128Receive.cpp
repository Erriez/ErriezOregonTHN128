/*
 * MIT License
 *
 * Copyright (c) 2019 Erriez
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
 * \file ErriezOregonTHN128Receiver.c
 * \brief Oregon THN128 433MHz temperature transmit/receive library for Arduino
 * \details
 *      Source:         https://github.com/Erriez/ErriezOregonTHN128
 *      Documentation:  https://erriez.github.io/ErriezOregonTHN128
 */

#include <Arduino.h>
#include <avr/interrupt.h>

#include "ErriezOregonTHN128Receive.h"

typedef enum {
    StateSearchSync = 0,
    StateMid0 = 1,
    StateMid1 = 2,
    StateEnd = 3,
    StateRxComplete = 4
} RxState_t;

/* Static variables */
static uint8_t _rxPinPort;
static uint8_t _rxPinBit;
static uint8_t _rxInt;
static uint32_t _tPulseBegin;
static uint16_t _tPinHigh;
static uint16_t _tPinLow;
static int8_t _rxBit;
static volatile uint32_t _rxData;
static volatile RxState_t _rxState = StateSearchSync;

/* Forward declaration */
void rfPinChange(void);


static void rxEnable()
{
    /* Enable INTx change interrupt */
    attachInterrupt(_rxInt, rfPinChange, CHANGE);

    /* Initialize with search for sync state */
    _rxState = StateSearchSync;
}

static void rxDisable()
{
    /* Disable INTx change interrupt */
    detachInterrupt(_rxInt);
}

static bool isPulseInRange(uint16_t tPulse, uint16_t tMin, uint16_t tMax)
{
    /* Check is pulse length between min and max time */
    if ((tPulse >= tMin) && (tPulse <= tMax)) {
        return true;
    } else {
        return false;
    }
}

static bool findSync()
{
    /* Read sync pulse */
    if (isPulseInRange(_tPinHigh, T_SYNC_H_MIN, T_SYNC_H_MAX)) {
        if (isPulseInRange(_tPinLow, T_SYNC_L_MIN_0, T_SYNC_L_MAX_0)) {
            _rxData = 0;
            _rxState = StateMid1;
            _rxBit = 1;
            return true;
        } else if (isPulseInRange(_tPinLow, T_SYNC_L_MIN_1, T_SYNC_L_MAX_1)) {
            _rxData = 0;
            _rxState = StateEnd;
            _rxBit = 0;
            return true;
        }
    }

    return false;
}

static void storeBit(bool one)
{
    /* Store received bit */
    if (one) {
        _rxData |= (1UL << _rxBit);
    }

    /* Check if all 32 data bits are received */
    _rxBit++;
    if (_rxBit >= 32) {
        if (OregonTHN128_CheckCRC(_rxData)) {
            _rxState = StateRxComplete;
            /* Disable receive */
            rxDisable();
        } else {
            _rxState = StateSearchSync;
        }
    }
}

static void handlePulse()
{
    if (isPulseInRange(_tPinHigh, T_BIT_SHORT_MIN, T_BIT_SHORT_MAX)) {
        if (_rxState == StateEnd) {
            _rxState = StateMid0;
            storeBit(1);
        } else if (_rxState == StateMid1) {
            _rxState = StateEnd;
        } else {
            _rxState = StateSearchSync;
        }
    } else if (isPulseInRange(_tPinHigh, T_BIT_LONG_MIN, T_BIT_LONG_MAX)) {
        if (_rxState == StateMid1) {
            _rxState = StateMid0;
            storeBit(1);
        } else {
            _rxState = StateSearchSync;
        }
    } else {
        _rxState = StateSearchSync;
    }
}

static void handleSpace()
{
    /* State machine */
    if (isPulseInRange(_tPinLow, T_BIT_SHORT_MIN, T_BIT_SHORT_MAX)) {
        if (_rxState == StateEnd) {
            _rxState = StateMid1;
            storeBit(0);
        } else if (_rxState == StateMid0) {
            _rxState = StateEnd;
        } else {
            _rxState = StateSearchSync;
        }
    } else if (isPulseInRange(_tPinLow, T_BIT_LONG_MIN, T_BIT_LONG_MAX)) {
        if (_rxState == StateMid0) {
            _rxState = StateMid1;
            storeBit(0);
        } else {
            _rxState = StateSearchSync;
        }
    } else {
        _rxState = StateSearchSync;
    }
}

void rfPinChange(void)
{
    uint32_t tNow;
    uint16_t _tPulseLength;
    uint8_t rfPinHigh;

    /* Return when previous completed receive is not read */
    if (_rxState == StateRxComplete) {
        return;
    }

    /* Read absolute pulse time in us for sync */
    tNow = micros();
    if (tNow > _tPulseBegin) {
        _tPulseLength = tNow - _tPulseBegin;
    } else {
        _tPulseLength = _tPulseBegin - tNow;
    }

    /* Ignore short pulses */
    if (_tPulseLength < T_RX_TOLERANCE_US) {
        return;
    }
    _tPulseBegin = tNow;

    /* Get RF pin state */
    rfPinHigh = *portInputRegister(_rxPinPort) & _rxPinBit;

    /* Store pulse (high) or space (low) length */
    if (rfPinHigh) {
        _tPinLow = _tPulseLength;
    } else {
        _tPinHigh = _tPulseLength;
    }

    /* Always search for sync */
    if (findSync()) {
        return;
    }

    /* Handle received pulse */
    if (_rxState != StateSearchSync) {
        if (rfPinHigh) {
            handleSpace();
        } else {
            handlePulse();
        }
    }
}

/*------------------------------------------------------------------------------------------------*/
/*                                     Public functions                                           */
/*------------------------------------------------------------------------------------------------*/
void OregonTHN128_RxBegin(uint8_t extIntPin)
{
    /* Save interrupt number of the RF pin */
    _rxInt = digitalPinToInterrupt(extIntPin);
    /* Save pin port and bit */
    _rxPinPort = digitalPinToPort(extIntPin);
    _rxPinBit = digitalPinToBitMask(extIntPin);

    /* Enable receive */
    rxEnable();
}

void OregonTHN128_RxEnable()
{
    /* Enable receive */
    rxEnable();
}

void OregonTHN128_RxDisable()
{
    /* Disable receive */
    rxDisable();
}

bool OregonTHN128_Available()
{
    /* Return receive complete */
    return (_rxState == StateRxComplete) ? true : false;
}

bool OregonTHN128_Read(OregonTHN128Data_t *data)
{
    if (OregonTHN128_Available()) {
        /* Convert raw 32-bit data to data structure */
        OregonTHN128_RawToData(_rxData, data);
        return true;
    } else {
        return false;
    }
}
