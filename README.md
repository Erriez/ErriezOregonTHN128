# Oregon THN128 433MHz temperature sensor transmit/receive library for Arduino
[![Build Status](https://travis-ci.org/Erriez/ErriezOregonTHN128.svg?branch=master)](https://travis-ci.org/Erriez/ErriezOregonTHN128)

This is a transmit/receive library Arduino library with the Oregon THN128 433MHz wireless protocol.

Tested on an Arduino UNO and pro-micro 3.3V 8MHz.

![Oregon THN128](https://raw.githubusercontent.com/Erriez/ErriezOregonTHN128/master/extras/OregonTHN128.png)

## Transmit / receive hardware

This library is optimized for low-power Arduino ATMega328 microcontroller. Other targets are not supported.

![Transmit and receive hardware](extras/transmit-receive-hardware.png)

**Temperature transmitter on the left breadboard:** 

* Pro-Mini 3V3 8MHz (with FTDI232 - USB serial interface).
* Genuine DS18B20 temperature sensor.
  STX802 low-power 433MHz transmitter.

**Receiver on on the right breadboard:**

* SRX882 low-power 433MHz receiver.
* SSD1306 I2C 128x64 OLED display.
* Pro-Mini 3V3 8MHz (with FTDI232 - USB serial interface).

### Hardware notes

* 1 to 3 temperature transmitters are supported, similar to the original Oregon THN128 temperature transmitters.
* Check [list of counterfeit DS18B20 chips](https://github.com/cpetrich/counterfeit_DS18B20) , because this makes a huge difference in accuracy and read errors at 3.3V. Many DS18B20 chips from Aliexpress are counterfeit and won't work reliable at voltages below 3.3V.

* [NiceRF Wireless Technology Co., Ltd.](https://nl.aliexpress.com/store/934254) sells high quality 433MHz transmit (STX802) and receiver modules (STX882) with a good range.
* The Pro-Mini 3V3 8MHz is a good low-power Arduino boards after desoldering the power LED. A transmitter with 18650 battery can operate for at least 6 months with `LowPower.h` functionality implemented. (By sending the temperature every 30 seconds)
* Changing the BOD (Brown Out Detection) fuse to 1.8V allows operation between 1.8 and 4.2V 18650 battery. 
* A 18650 battery (with protection circuit) should be connected directly to the VCC pin (not VIN). 
* The voltage regulator can be desoldered from the pro-micro board when not used for more power reduction.

## Oregon Protocol

A packet is sent twice:

![Oregon THN128 Protocol](https://raw.githubusercontent.com/Erriez/ErriezOregonTHN128/master/extras/OregonTHN128Protocol.png)

Data (see header file [ErriezOregonTHN128Receive.h](https://github.com/Erriez/ErriezOregonTHN128/blob/master/src/ErriezOregonTHN128Receive.h)):

![Oregon THN128 Temperature 16.6](https://raw.githubusercontent.com/Erriez/ErriezOregonTHN128/master/extras/OregonTHN128Temperature16.6.png)


## Example low power receive

```c++
#include <LowPower.h>
#include <ErriezOregonTHN128Receive.h>

// RF pin 2 (INT0) or pin 3 (INT1) defines
#define RF_RX_PIN     2


void printReceivedData(OregonTHN128Data_t *data)
{
    bool negativeTemperature = false;
    static uint32_t rxCount = 0;
    int16_t tempAbs;
    char msg[80];

    // Convert to absolute temperature
    tempAbs = data->temperature;
    if (tempAbs < 0) {
        negativeTemperature = true;
        tempAbs *= -1;
    }
    snprintf(msg, sizeof(msg), "RX %lu: Rol: %d, Channel %d, Temp: %s%d.%d, Low batt: %d (0x%08lx)",
             rxCount++,
             data->rollingAddress, data->channel,
             (negativeTemperature ? "-" : ""), (tempAbs / 10), (tempAbs % 10), data->lowBattery,
             data->rawData);
    Serial.println(msg);
}

void setup()
{
    // Initialize serial port
    Serial.begin(115200);
    Serial.println(F("Oregon THN128 433MHz temperature receive"));

    // Turn LED on
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    // Initialize receiver
    OregonTHN128_RxBegin(RF_RX_PIN);
}

void loop()
{
    OregonTHN128Data_t data;

    // Check temperature received
    if (OregonTHN128_Available()) {
        digitalWrite(LED_BUILTIN, LOW);
      
        // Read temperature
        OregonTHN128_Read(&data);

        // Print received data
        printReceivedData(&data);

        // Wait ~30 seconds before receiving next temperature
        Serial.flush();
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
        LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);

        digitalWrite(LED_BUILTIN, HIGH);

        // Enable receive
        OregonTHN128_RxEnable();
    }
}
```





## Example low power transmit

```c++
#include <LowPower.h>
#include <ErriezOregonTHN128Transmit.h>

// Pin defines
#define RF_TX_PIN           9

OregonTHN128Data_t data = {
    .rawData = 0,           // Raw data filled in by library
    .rollingAddress = 5,    // Rolling address 0..7
    .channel = 1,           // Channel 1, 2 or 3
    .temperature = 0,     	// Temperature -99.9 .. 99.9 multiplied by 10
    .lowBattery = false,	// Low battery true or false
};

#ifdef __cplusplus
extern "C" {
#endif

// Function is called from library
void delay100ms()
{
    Serial.flush();
    digitalWrite(LED_BUILTIN, HIGH);
    LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF);
    digitalWrite(LED_BUILTIN, LOW);
    LowPower.powerDown(SLEEP_30MS, ADC_OFF, BOD_OFF);
}

#ifdef __cplusplus
}
#endif

void setup()
{
    // Initialize pins
    OregonTHN128_TxBegin(RF_TX_PIN);
}

void loop()
{
    // Set temperature
    data.temperature = 123; //12.3`C
    
    // Send temperature
    OregonTHN128_Transmit(&data);
    
    // Wait some time
    // Wait ~30 seconds before sending next temperature
    Serial.flush();
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
}
```
