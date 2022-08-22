[![Licence MIT](https://img.shields.io/badge/license-MIT-green)](https://github.com/Erriez/ErriezOregonTHN128/blob/master/LICENSE)
[![Language C/C++](https://img.shields.io/badge/language-C%2FC%2B%2B-informational)](https://github.com/Erriez/ErriezOregonTHN128)
[![Release tag](https://img.shields.io/github/v/release/Erriez/ErriezOregonTHN128?display_name=tag)](https://github.com/Erriez/ErriezOregonTHN128/releases)
[![Open issue](https://shields.io/github/issues-raw/Erriez/ErriezOregonTHN128)](https://github.com/Erriez/ErriezOregonTHN128/issues)
[![PlatformIO CI](https://github.com/Erriez/ErriezOregonTHN128/actions/workflows/actions.yml/badge.svg)](https://github.com/Erriez/ErriezOregonTHN128/actions/workflows/actions.yml)


# Oregon THN128 433MHz temperature transmit/receive library for Arduino

This is a 433MHz wireless 3-channel Oregon THN128 temperature transmit/receive Arduino library for ATMega328,
ESP8266 and ESP32 emulating v1 protocol:

![Oregon THN128](https://raw.githubusercontent.com/Erriez/ErriezOregonTHN128/master/extras/OregonTHN128.png)

## Transmit / receive hardware

This Arduino library is optimized for low-power ATMega328 microcontroller (AVR architectures like `Arduino UNO` and 
`Pro Mini 3.3V 8MHz` boards).

![Transmit and receive hardware](extras/transmit-receive-hardware.png)

**Temperature transmitter on the left breadboard:** 

* Pro-Mini 3V3 8MHz.
* Genuine DS18B20 temperature sensor.
* STX802 low-power 433MHz transmitter.

**Receiver on the right breadboard:**

* SRX882 low-power 433MHz receiver.
* SSD1306 I2C 128x64 OLED display.
* Pro-Mini 3V3 8MHz.

### Supported microcontrollers

* ATMega328 AVR designed for low-power
* ESP8266
* ESP32
* Other microcontrollers are not tested and may or may not work


### Hardware notes

Supported hardware:
* AVR designed for low-power
* ESP8266
* ESP32

* For low-power transmitters, a `Pro Mini 3V3 8MHz` bare board with ATMega328 microcontroller is highly recommended. The
  board has no serial interface chip which reduces continuous power consumption. An external FTDI232 - USB serial 
  interface  should be connected for serial console / programming. (See red PCB on the picture)
  The SMD power LED should be desoldered from the Pro Mini to reduce continuous power consumption. 
* A transmitter with (protected) 1500mA 18650 battery can operate for at least 6 months with `LowPower.h` functionality
  implemented. (By sending the temperature every 30 seconds)
* Changing the BOD (Brown Out Detection) fuse to 1.8V allows operation between 1.8 and 4.2V 18650 battery. (Explanation 
  beyond the scope of this project)
* 1 to 3 temperature transmitters are supported, similar to the original Oregon THN128 temperature transmitters.
* Check [list of counterfeit DS18B20 chips](https://github.com/cpetrich/counterfeit_DS18B20) , because this makes a huge
  difference in accuracy and read errors at 3.3V. Many DS18B20 chips from Aliexpress are counterfeit and won't work  
  reliable at voltages below 3.3V.
* [NiceRF Wireless Technology Co., Ltd.](https://nl.aliexpress.com/store/934254) sells high quality 433MHz transmit 
  (STX802) and receiver modules (STX882) with a good range.
* A 18650 battery (with protection circuit) should be connected directly to the VCC pin (not VIN). 
* The voltage regulator can be desoldered from the pro-micro board when not used for more power reduction.


## Oregon Protocol

A packet is sent twice:

![Oregon THN128 Protocol](https://raw.githubusercontent.com/Erriez/ErriezOregonTHN128/master/extras/OregonTHN128Protocol.png)

Data (see header file [ErriezOregonTHN128Receive.h](https://github.com/Erriez/ErriezOregonTHN128/blob/master/src/ErriezOregonTHN128Receive.h)):

*  Byte 0:
   * Bit 0..3: Rolling address (Random value after power cycle)
   * Bit 6..7: Channel: (0 = channel 1 .. 2 = channel 3)
*  Byte 1:
   * Bit 0..3: TH3
   * Bit 4..7: TH2
*  Byte 2:
   * Bit 0..3: TH1
   * Bit 5:    Sign
   * Bit 7:    Low battery
*  Byte 3:
   * Bit 0..7: CRC

![Oregon THN128 Temperature 16.6](https://raw.githubusercontent.com/Erriez/ErriezOregonTHN128/master/extras/OregonTHN128Temperature16.6.png)


## Examples

Please refer to the examples:

* [OregonTHN128 Receive](https://github.com/Erriez/ErriezOregonTHN128/blob/master/examples/ErriezOregonTHN128Receive/ErriezOregonTHN128Receive.ino)
* [OregonTHN128 Receive SSD1306 OLED](https://github.com/Erriez/ErriezOregonTHN128/blob/master/examples/ErriezOregonTHN128ReceiveSSD1306/ErriezOregonTHN128ReceiveSSD1306.ino)
* [OregonTHN128 Transmit random temperature](https://github.com/Erriez/ErriezOregonTHN128/blob/master/examples/ErriezOregonTHN128Transmit/ErriezOregonTHN128Transmit.ino)
* [OregonTHN128 Transmit DS1820 1-wire temperature sensor](https://github.com/Erriez/ErriezOregonTHN128/blob/master/examples/ErriezOregonTHN128TransmitDS1820/ErriezOregonTHN128TransmitDS1820.ino)


## Library Changes

### v1.1.0

The callback function `void delay100ms()` has been removed as this was not compatible with ESP32. The application should
change the code to:

```c++
    // Send temperature twice with 100ms delay between packets
    OregonTHN128_Transmit(&data);
    delay(100);
    OregonTHN128_Transmit(&data);
```

AVR targets can replace `delay(100)` with LowPower usage:

```c++
    LowPower.powerDown(SLEEP_15MS, ADC_OFF, BOD_OFF);
    LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF);
    LowPower.powerDown(SLEEP_15MS, ADC_OFF, BOD_OFF);
```

## Saleae Logic Analyzer

![capture](extras/SaleaeLogicAnalyzer/RX_rol7_channel1_temp20.7_lowbat0.png)

[capture](extras/SaleaeLogicAnalyzer/RX_rol7_channel1_temp20.7_lowbat0.sal) from the Oregon THN128 can be opened with 
https://www.saleae.com/downloads/.

## Generated Arduino Library Doxygen Documentation

* [Online Doxygen HTML](https://erriez.github.io/ErriezOregonTHN128/index.html)
* [Doxygen PDF](https://github.com/Erriez/ErriezOregonTHN128/blob/gh-pages/ErriezOregonTHN128.pdf)

## MIT License

This project is published under [MIT license](https://github.com/Erriez/ErriezOregonTHN128/blob/master/LICENSE)
with an additional end user agreement (next section).

## End User Agreement :ukraine:

End users shall accept the [End User Agreement](https://github.com/Erriez/ErriezOregonTHN128/blob/master/END_USER_AGREEMENT.md)
holding export restrictions to Russia to stop the WAR before using this project.
