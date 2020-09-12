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

#include <Arduino.h>
#include <LowPower.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ErriezOregonTHN128Transmit.h>

// Pin defines (Any DIGITAL pin)
#define RF_TX_PIN           3
#define ONE_WIRE_BUS        2

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


#ifdef __cplusplus
extern "C" {
#endif

// Function is called from library
void delay100ms()
{
    Serial.flush();
    digitalWrite(LED_BUILTIN, HIGH);
    LowPower.powerDown(SLEEP_15MS, ADC_OFF, BOD_OFF);
    digitalWrite(LED_BUILTIN, LOW);
    LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF);
    LowPower.powerDown(SLEEP_15MS, ADC_OFF, BOD_OFF);
}

#ifdef __cplusplus
}
#endif

static long readVcc()
{
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
    return result; // Vcc in millivolts
}

static void printData()
{
    static uint32_t txCount = 0;
    char temperatureStr[10];
    char msg[80];

    // Convert data structure to 32-bit raw data;
    data.rawData = OregonTHN128_DataToRaw(&data);

    OregonTHN128_TempToString(temperatureStr, sizeof(temperatureStr), data.temperature);
    snprintf(msg, sizeof(msg), "TX %lu: Rol: %d, Channel %d, Temp: %s, Low batt: %d (0x%08lX)",
             txCount++,
             data.rollingAddress, data.channel, temperatureStr, data.lowBattery, data.rawData);
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
    if (readVcc() < 2850) {
        data.lowBattery = true;
    } else {
        data.lowBattery = false;
    }

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

    // Wait ~30 seconds before sending next temperature
    Serial.flush();
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
}