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
 * \brief Erriez ESP32 Oregon THN128 example with MQTT and SSL for Homeassistant
 * \details
 *      Source:         https://github.com/Erriez/ErriezOregonTHN128
 *      Documentation:  https://erriez.github.io/ErriezOregonTHN128
 *
 *  Hardware:
 *  - Tested with NodeMCU-32S
 *  - LED_PIN:   LED pin on the ESP32
 *  - RF_RX_PIN: 433MHz receiver pin
 *  
 *  WiFi:
 *  - WIFI_SSID:        WiFi SSID
 *  - WIFI_PASSWORD:    WiFi password
 *
 *  SSL:
 *  - USE_SSL: Enable SSL when defined. This requires SSL certificates when SSL is enabled:
 *    - root_ca
 *    - client_cert
 *    - client_key
 * 
 *  Network (optional):
 *  - staticIP / gateway / subnet / dns1 / dns2
 *  - Uncomment and configure code in wifiConnect()
 *
 *  MQTT:
 *  - MQTT_HOST       MQTT host
 *  - MQTT_PORT       MQTT port
 *  - MQTT_USERNAME   Optional: MQTT username (optional in combination with MQTT_PASSWORD)
 *  - MQTT_PASSWORD   Optional: MQTT password (optional in combination with MQTT_USERNAME)
 *  - MQTT_DEVICE_ID  MQTT unique device ID
 */

// Enable SSL
#define USE_SSL

#include <WiFi.h>
#ifdef USE_SSL
#include <WiFiClientSecure.h>
#else
#include <WiFiClient.h>
#endif
#include <ArduinoJson.h>                  // https://github.com/bblanchon/ArduinoJson.git v6.19.4
#include <MQTTClient.h>                   // https://github.com/256dpi/arduino-mqtt v2.5.0
#include <ErriezOregonTHN128Receive.h>    // https://github.com/Erriez/ErriezOregonTHN128 v1.1.0

#ifndef ARDUINO_ARCH_ESP32
#error "This example has been tested on ESP32 only"
#endif

// LED pin
#define LED_PIN             LED_BUILTIN
#define RF_RX_PIN           19  // GPIO19
#define RX_CH_TIMETOUT_MS   (2 * 60 * 1000)

// WiFi SSID and password
#define WIFI_SSID       "YOUR_SSID"
#define WIFI_PASSWORD   "YOUR_PASSWORD"

// Optional: Fixed network configuration
//IPAddress staticIP(192, 168, 16, 170);
//IPAddress gateway(192, 168, 16, 1);
//IPAddress subnet(255, 255, 255, 0);
//IPAddress dns1(192, 168, 16, 1);
//IPAddress dns2(192, 168, 16, 200);

// MQTT Broker host
#define MQTT_HOST       "mosquitto.yourdomain.nl"

// MQTT Broker port
#ifdef USE_SSL
#define MQTT_PORT       8883
#else
#define MQTT_PORT       1883
#endif

// Optional: MQTT username and password, or comment both out
#define MQTT_USERNAME   "YOUR_MQTT_USERNAME"
#define MQTT_PASSWORD   "YOUR_MQTT_PASSWORD"

// MQTT device ID, subscribe topic and publish topic
#define MQTT_DEVICE_ID  "esp32.oregon_thn128"

// WiFi client
#ifdef USE_SSL
WiFiClientSecure wifiClient;
#else
WiFiClient wifiClient;
#endif

// Increase MQTT internal buffer size
#define MQTT_BUF_SIZE   300

// MQTT client
MQTTClient mqtt(MQTT_BUF_SIZE);

volatile bool ha_online = false;

typedef struct {
    long   tlastupdate;
    String temperature;
    String battery;
} OregonTHN128Device_t;

OregonTHN128Device_t devices[3] = {
    { .tlastupdate = 0, .temperature = "unknown", .battery = "unknown" },
    { .tlastupdate = 0, .temperature = "unknown", .battery = "unknown" },
    { .tlastupdate = 0, .temperature = "unknown", .battery = "unknown" }
};

#ifdef USE_SSL
// Root CA certificate
const char root_ca[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
YOUR ROOT CA
-----END CERTIFICATE-----
)EOF";

// Client certificate
const char client_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
YOUR CLIENT CERT
-----END CERTIFICATE-----
)EOF";

// Client certificate key
const char client_key[] PROGMEM = R"EOF(
-----BEGIN PRIVATE KEY-----
YOUR CLIENT KEY
-----END PRIVATE KEY-----
)EOF";
#endif


void mqttPublish(String topic, String payload, bool retain=false, int qos=0)
{
    Serial.println("MQTT publish: topic=" + topic + ", retain=" + retain + ", qos=" + qos + ", payload=" + payload);
    mqtt.publish(topic, payload, retain, qos);
}

void mqttPublishDeviceChannelConfig(int channel)
{
    String topic = "ha/sensor/oregon_thn128_t" + String(channel) + "/config";
    String payload;

    // Allocate static JSON document on stack.
    // As the doc object cannot be reused in a for loop, it is placed in a separate
    // function to automatically destroy when leaving this function.
    //
    // Inside the brackets, 300 is the RAM allocated to this document.
    // Don't forget to change this value to match your requirement.
    // Use arduinojson.org/v6/assistant to compute the capacity.
    StaticJsonDocument<300> doc;

    doc["name"]                = "Oregon THN128 CH" + String(channel) + " Temperature";
    doc["unique_id"]           = "sensor.oregon_thn128_ch" + String(channel) + "_temperature";
    doc["device_class"]        = "temperature";
    doc["unit_of_measurement"] = "°C";
    doc["value_template"]      = "{{value_json.t" + String(channel) + "}}";
    doc["state_topic"]         = "ha/sensor/oregon_thn128/state";
    serializeJson(doc, payload);

    // Publish
    mqttPublish(topic, payload, true, 1);

    // doc object is automatically destroyed from stack
}

void mqttPublishDeviceBatteryConfig(int channel)
{
    String topic = "ha/sensor/oregon_thn128_b" + String(channel) + "/config";
    String payload;
    StaticJsonDocument<256> doc;

    doc["name"]                = "Oregon THN128 CH" + String(channel) + " Battery";
    doc["unique_id"]           = "sensor.oregon_thn128_ch" + String(channel) + "_battery";
    doc["device_class"]        = "battery";
    doc["value_template"]      = "{{value_json.b" + String(channel) + "}}";
    doc["state_topic"]         = "ha/sensor/oregon_thn128/state";
    serializeJson(doc, payload);

    // Publish
    mqttPublish(topic, payload, true, 1);
}

void mqttPublishHaConfig()
{
    for (int channel = 1; channel <= 3; channel++) {
        mqttPublishDeviceChannelConfig(channel);
        mqttPublishDeviceBatteryConfig(channel);
    }
}

void mqttPublishStates()
{
    // Convert the value to a char array
    String topic;
    String payload;

    StaticJsonDocument<128> doc;
    doc["t1"] = devices[0].temperature;
    doc["t2"] = devices[1].temperature;
    doc["t3"] = devices[2].temperature;
    doc["b1"] = devices[0].battery;
    doc["b2"] = devices[1].battery;
    doc["b3"] = devices[2].battery;
    serializeJson(doc, payload);

    // Publish
    topic = String("ha/sensor/oregon_thn128/state");
    mqttPublish(topic, payload);
}

void mqttReceive(String &topic, String &payload) 
{
    digitalWrite(LED_PIN, HIGH);

    Serial.println("MQTT received: " + topic + " - " + payload);

    if ((topic == "homeassistant/status") && (payload == "online")) {
        // Re-publish config in main loop after Homeassistant restart
        ha_online = true;
    }

    digitalWrite(LED_PIN, LOW);
}

void mqttConnect() 
{
    // Loop until we're reconnected
    while (!mqtt.connected()) {
        Serial.print("Connecting to MQTT broker ");
        Serial.print(MQTT_HOST);
        Serial.print(":");
        Serial.print(MQTT_PORT);
        Serial.print("...");

#if defined(MQTT_USERNAME) && defined(MQTT_PASSWORD)
        if (mqtt.connect(MQTT_DEVICE_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
#else
        if (mqtt.connect(MQTT_DEVICE_ID)) {
#endif
            Serial.println("Connected");
            mqtt.subscribe("homeassistant/status");
            mqttPublishHaConfig();
        } else {
            Serial.println("Failed try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void wifiConnect()
{
    Serial.print("Connecting to WiFi ");
    Serial.print(WIFI_SSID);
    Serial.print("...");

    // Optional: Configure static network configuration with DHCP off
//    if (!WiFi.config(staticIP, gateway, subnet, dns1, dns2)) {
//        Serial.println("Configuration failed.");
//    }

    // Connect to WiFi network
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    // Optional: Update network configuration after WiFi connection
//    if (!WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), dns1, dns2)) {
//        Serial.println("Configuration failed.");
//    }

    // Print network configuration
    Serial.println("Connected:");
    Serial.print("  IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("  Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("  Subnet: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("  DNS 0: ");
    Serial.println(WiFi.dnsIP(0));
    Serial.print("  DNS 1: ");
    Serial.println(WiFi.dnsIP(1));

    // Set client SSL certificates
#ifdef USE_SSL
    wifiClient.setCACert(root_ca);
    wifiClient.setCertificate(client_cert);
    wifiClient.setPrivateKey(client_key);
#endif
}

void setup()
{
    Serial.begin(115200);
    Serial.println(F("\nErriez Oregon THN128 ESP32 MQTT Homeassistant example\n"));

    // Initialize MQTT
    mqtt.begin(MQTT_HOST, MQTT_PORT, wifiClient);
    mqtt.onMessage(mqttReceive);

    // Initialize receiver
    OregonTHN128_RxBegin(RF_RX_PIN);

    // Initialize LED
    pinMode(LED_PIN, OUTPUT);
}

void loop() 
{
    static unsigned long rxCount = 0;
    OregonTHN128Data_t data;
    float temperature;
    char msg[80];

    // Connect to WiFi on disconnect
    if (WiFi.status() != WL_CONNECTED) {
        wifiConnect();
    }

    // Reconnect MQTT on disconnect
    if (!mqtt.connected()) {
        mqttConnect();
    }

    // Process MQTT messages
    mqtt.loop();

    // Process Homeassistant online message
    if (ha_online) {
        mqttPublishHaConfig();
        ha_online = false;
    }

    // Check temperature received
    if (OregonTHN128_Available()) {
      
        digitalWrite(LED_PIN, HIGH);

        // Read temperature
        OregonTHN128_Read(&data);
    
        // Print received data
        temperature = (float)data.temperature / 10.0;
        snprintf_P(msg, sizeof(msg),
                   PSTR("RX %lu: Rol: %d, Channel %d, Temp: %.1f, Low batt: %d (0x%08lx)"),
                   rxCount++,
                   data.rollingAddress, data.channel,
                   temperature, data.lowBattery,
                   (unsigned long)data.rawData);
        Serial.println(msg);

        devices[data.channel-1].tlastupdate = millis();
        devices[data.channel-1].temperature =  temperature <= -127 ? "unknown" : String(temperature, 1);
        devices[data.channel-1].battery = data.lowBattery ? 0 : 100;

        // Reset temperature when connection lost
        for (int channel = 0; channel < 3; channel++) {
            if ((millis() - devices[channel].tlastupdate) > RX_CH_TIMETOUT_MS) {
                devices[channel].temperature = "unknown";
                devices[channel].battery = "unknown";
            }
        }

        // Publish temperatures
        mqttPublishStates();

        // Enable receive
        OregonTHN128_RxEnable();

        digitalWrite(LED_PIN, LOW);
    }
}
