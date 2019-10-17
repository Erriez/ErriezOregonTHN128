# Oregon THN128 433MHz temperature sensor transmit/receive library for Arduino

This is a transmit/receive library Arduino library with the Oregon THN128 433MHz wireless protocol.

 ![OregonTHN128](D:\Arduino\libraries\ErriezOregonTHN128\extras\OregonTHN128.png)



## Protocol

![OregonTHN128Protocol](D:\Arduino\libraries\ErriezOregonTHN128\extras\OregonTHN128Protocol.png)



## Data

![OregonTHN128Temperature16.6](D:\Arduino\libraries\ErriezOregonTHN128\extras\OregonTHN128Temperature16.6.png)



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

