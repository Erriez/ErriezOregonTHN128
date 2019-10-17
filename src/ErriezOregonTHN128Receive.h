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
 * \file ErriezOregonTHN128Receiver.h
 * \brief Oregon THN128 433MHz temperature receive library for Arduino
 * \details
 *      Source:         https://github.com/Erriez/ErriezOregonTHN128
 *      Documentation:  https://erriez.github.io/ErriezOregonTHN128
 *
 * Protocol:
 *
 * Transmit temperature twice every 30 seconds:
 *
 *  Bit:       0    7 0    7 0    7 0    7
 *  +----+----+------+------+------+------+           +----+----+------+--
 *  |PREA|SYNC|Byte 0|Byte 1|Byte 2|Byte 3|           |PREA|SYNC|Byte 0|  ...
 *  +----+----+------+------+------+------+----/\/----+----+----+------+--
 *  |<--------------- 144ms ------------->|<- 100ms ->|                  30 sec
 *
 *
 *   Logic '0':     Logic '1':
 *       +----+     +----+
 *       |               |
 *  +----+               +----+
 *   1400 1500       1500 1400  (us)
 *
 *
 *  PREA: Preamble 12x logic '1', 3000us low

 *  SYNC:
 *   +--------+
 *   |        |
 *   +        +--------+
 *     5500us   5500us
 *
 *  Byte 0:
 *  - Bit 0..3: Rolling address (Random value after power cycle)
 *  - Bit 6..7: Channel: (0 = channel 1 .. 2 = channel 3)
 *
 *  Byte 1:
 *  - Bit 0..3: TH3
 *  - Bit 4..7: TH2
 *
 *  Byte 2:
 *  - Bit 0..3: TH1
 *  - Bit 5:    Sign
 *  - Bit 7:    Low battery
 *
 *  Byte 3:
 *  - Bit 0..7: CRC
 *
 * Example: Rolling address = 5, channel = 1, temperature = 27.8 `C, low battery = false
 *    TH1 = 2, TH2 = 7, TH3 = 8:
 *    Byte 0: 0x05
 *    Byte 1: 0x78
 *    Byte 2: 0x02
 *    Byte 3: 0x7f
 *
 * Bits in time:
 *    PRE=1        S B0=0x05  B1=0x78  B2=0x02  B3=0x7f
 *    111111111111 S 10100000 00011110 01000000 11111110
 */

#ifndef ERRIEZ_OREGON_THN128_RECEIVER_H_
#define ERRIEZ_OREGON_THN128_RECEIVER_H_

#include <stdint.h>
#include "ErriezOregonTHN128.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Public functions */
void OregonTHN128_RxBegin(uint8_t extIntPin);
void OregonTHN128_RxEnable();
void OregonTHN128_RxDisable();
bool OregonTHN128_Available(void);
uint32_t OregonTHN128_GetRawData();
bool OregonTHN128_Read(OregonTHN128Data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* ERRIEZ_OREGON_THN128_RECEIVER_H_ */
