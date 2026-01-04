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

#include <Arduino.h>
#include <OneWire.h>                    // https://github.com/PaulStoffregen/OneWire
#include <DallasTemperature.h>          // https://github.com/milesburton/Arduino-Temperature-Control-Library
#include <ErriezOregonTHN128Transmit.h> // https://github.com/Erriez/ErriezOregonTHN128

#if defined(ARDUINO_ARCH_AVR)

#include <LowPower.h>           // https://github.com/LowPowerLab/LowPower
#define ONE_WIRE_BUS        2   // Any DIGITAL pin
#define RF_TX_PIN           3   // Any DIGITAL pin

#elif defined(ARDUINO_ARCH_ESP8266)

extern "C" {
#include "user_interface.h"
}

#define ONE_WIRE_BUS        2   // NodeMCU D4
#define RF_TX_PIN           4   // NodeMCU D2

ADC_MODE(ADC_VCC);              // Set ADC in VCC read mode

#elif defined(ARDUINO_ARCH_ESP32)

#define ONE_WIRE_BUS        21
#define RF_TX_PIN           22

#else
#error "May work, but not tested on this target"
#endif

// Create OneWire and DS1820 objects
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds1820(&oneWire);

OregonTHN128Data_t data = {
        .rawData = 0,           // Raw data filled in by driver
        .rollingAddress = 5,    // Rolling address 0..7
        .channel = 1,           // Channel 1, 2 or 3
        .temperature = 0,       // Temperature -99.9 .. 99.9 multiplied by 10
        .lowBattery = false,
};


static bool isLowBatt()
{
    bool lowBatt = false;

#if defined(ARDUINO_ARCH_AVR)
// Read 1.1V reference against AVcc
// set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
#else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

    delay(2); // Wait for Vref to settle
    ADCSRA |= _BV(ADSC); // Start conversion
    while (bit_is_set(ADCSRA,ADSC)); // measuring

    uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
    uint8_t high = ADCH; // unlocks both

    long result = (high<<8) | low;

    result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
    if (result < 2850) {
        lowBatt = true;
    }

#elif defined(ARDUINO_ARCH_ESP8266)
    if (ESP.getVcc() < 2500) {
        lowBatt = true;
    }

#else
#warning "Todo: readVcc() not implemented yet"
#endif

    return lowBatt;
}

static void printData()
{
    static unsigned long txCount = 0;
    char temperatureStr[10];
    char msg[80];

    // Convert data structure to 32-bit raw data;
    data.rawData = OregonTHN128_DataToRaw(&data);

    OregonTHN128_TempToString(temperatureStr, sizeof(temperatureStr), data.temperature);
    snprintf(msg, sizeof(msg), "TX %lu: Rol: %d, Channel %d, Temp: %s, Low batt: %d (0x%08lX)",
             txCount++,
             data.rollingAddress, data.channel, temperatureStr, data.lowBattery, (unsigned long)data.rawData);
    Serial.println(msg);
}

void setup()
{
    // Initialize serial
    Serial.begin(115200);
    Serial.println(F("\nErriez Oregon THN128 433MHz DS1820 temperature transmit"));

    // Initialize built-in LED
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    // Initialize DS1820 temperature sensor
    ds1820.begin();
    ds1820.setWaitForConversion(true);
    ds1820.requestTemperatures();
    (void)ds1820.getTempCByIndex(0);

    // Initialize random
    randomSeed(analogRead(0));

    // Initialize pins
    OregonTHN128_TxBegin(RF_TX_PIN);
}

void loop()
{
    float temp;

    // Check battery voltage
    data.lowBattery = isLowBatt();

    // Read temperature
    ds1820.requestTemperatures();
    temp = ds1820.getTempCByIndex(0);

    // Set random temperature
    data.temperature = (int16_t)(temp * 10);

    // Print diagnostics
    printData();

    delay(1000);

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