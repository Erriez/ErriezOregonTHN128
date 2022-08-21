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

#include <Arduino.h>
#include <ErriezOregonTHN128Transmit.h> // https://github.com/Erriez/ErriezOregonTHN128

#if defined(ARDUINO_ARCH_AVR)
#include <LowPower.h>           // https://github.com/LowPowerLab/LowPower
#define RF_TX_PIN           3   // Any DIGITAL pin
#elif defined(ARDUINO_ARCH_ESP8266)
#define RF_TX_PIN           4   // NodeMCU D2
#elif defined(ARDUINO_ARCH_ESP32)
#define RF_TX_PIN           22
#else
#error "May work, but not tested on this target"
#endif

OregonTHN128Data_t data = {
    .rawData = 0,           // Raw data filled in by driver
    .rollingAddress = 5,    // Rolling address 0..7
    .channel = 1,           // Channel 1, 2 or 3
    .temperature = 0,       // Temperature -99.9 .. 99.9 multiplied by 10
    .lowBattery = false,
};


static void printData()
{
    static unsigned long txCount = 0;
    char temperatureStr[10];
    char msg[80];

    // Convert data structure to 32-bit raw data;
    data.rawData = OregonTHN128_DataToRaw(&data);

    OregonTHN128_TempToString(temperatureStr, sizeof(temperatureStr), data.temperature);
    snprintf_P(msg, sizeof(msg),
               PSTR("TX %lu: Rol: %d, Channel %d, Temp: %s, Low batt: %d (0x%08lX)"),
               txCount++,
               data.rollingAddress, data.channel, temperatureStr, data.lowBattery, (unsigned long)data.rawData);
    Serial.println(msg);
}

void setup()
{
    // Initialize serial
    Serial.begin(115200);
    Serial.println(F("\nErriez Oregon THN128 433MHz temperature transmit"));

    // Initialize built-in LED
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    // Initialize random
    randomSeed(analogRead(0));

    // Initialize pins
    OregonTHN128_TxBegin(RF_TX_PIN);
}

void loop()
{
    // Set random temperature
    data.temperature = (int16_t)random(-130, 400);

    // Print diagnostics
    printData();

    // Send temperature
    OregonTHN128_Transmit(&data);

#if defined(ARDUINO_ARCH_AVR)
    // Blink LED within 100ms space between two packets
    Serial.flush();
    digitalWrite(LED_BUILTIN, HIGH);
    LowPower.powerDown(SLEEP_15MS, ADC_OFF, BOD_OFF);
    digitalWrite(LED_BUILTIN, LOW);
    LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF);
    LowPower.powerDown(SLEEP_15MS, ADC_OFF, BOD_OFF);
#else
    delay(100);
#endif

    // Send temperature again
    OregonTHN128_Transmit(&data);

    // Wait ~30 seconds before sending next temperature
#if defined(ARDUINO_ARCH_AVR)
    Serial.flush();
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
#else
    delay(30 * 1000);
#endif
}