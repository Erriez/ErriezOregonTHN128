/*
 * MIT License
 *
 * Copyright (c) 2020-2022 Erriez
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

#ifndef ERRIEZ_OREGON_THN128_H_
#define ERRIEZ_OREGON_THN128_H_

/* Check platform */
#if !defined(AVR)
#error "Platform not supported."
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* Timing micro's in micro seconds */
#define T_RX_TOLERANCE_US       400
#define T_PREAMBLE_SPACE_US     3000
#define T_SYNC_US               5500
#define T_BIT_US                1450
#define T_SPACE_FRAMES_MS       100

#define T_SYNC_H_MIN        (T_SYNC_US - T_RX_TOLERANCE_US)
#define T_SYNC_H_MAX        (T_SYNC_US + T_RX_TOLERANCE_US)

#define T_SYNC_L_MIN_0      (T_SYNC_US + T_BIT_US - T_RX_TOLERANCE_US)
#define T_SYNC_L_MAX_0      (T_SYNC_US + T_BIT_US + T_RX_TOLERANCE_US)
#define T_SYNC_L_MIN_1      (T_SYNC_US - T_RX_TOLERANCE_US)
#define T_SYNC_L_MAX_1      (T_SYNC_US + T_RX_TOLERANCE_US)

#define T_BIT_SHORT_MIN     (T_BIT_US - T_RX_TOLERANCE_US)
#define T_BIT_SHORT_MAX     (T_BIT_US + T_RX_TOLERANCE_US)
#define T_BIT_LONG_MIN      ((T_BIT_US * 2) - T_RX_TOLERANCE_US)
#define T_BIT_LONG_MAX      ((T_BIT_US * 2) + T_RX_TOLERANCE_US)

/*!
 * \brief Data structure
 */
typedef struct {
    uint32_t rawData;           /*!< Raw data */
    uint8_t rollingAddress;     /*!< Rolling address */
    uint8_t channel;            /*!< Channel */
    int16_t temperature;        /*!< Temperature */
    bool lowBattery;            /*!< Low battery indication */
} OregonTHN128Data_t;

/* Public functions */
bool OregonTHN128_CheckCRC(uint32_t rawData);
void OregonTHN128_TempToString(char *temperatureStr, uint8_t temperatureStrLen, int16_t temperature);
uint32_t OregonTHN128_DataToRaw(OregonTHN128Data_t *data);
bool OregonTHN128_RawToData(uint32_t rawData, OregonTHN128Data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* ERRIEZ_OREGON_THN128_H_ */
