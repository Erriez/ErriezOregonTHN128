#/bin/bash
#
#  MIT License
#
#  Copyright (c) 2022 Erriez
#
#  Permission is hereby granted, free of charge, to any person obtaining a copy
#  of this software and associated documentation files (the "Software"), to deal
#  in the Software without restriction, including without limitation the rights
#  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#  copies of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

# Automatic build script
#
# To run locally, execute:
# sudo apt install doxygen graphviz texlive-latex-base texlive-latex-recommended texlive-pictures texlive-latex-extra

# Exit immediately if a command exits with a non-zero status.
set -e

DOXYGEN_PDF="ErriezOregonTHN128.pdf"

echo "Starting auto-build script..."

function setup_virtualenv()
{
    if [ ! -d ".venv" ]; then
        virtualenv .venv
        source .venv/bin/activate
        pip3 install platformio==6.1.4
        deactivate
    fi

    source .venv/bin/activate
}

function autobuild()
{
    # Set environment variables
    BOARDS_AVR="--board uno --board pro16MHzatmega328"
    BOARDS_ESP8266="--board d1_mini --board nodemcuv2"
    BOARDS_ESP32="--board lolin_d32"

    echo "Installing library dependencies"
    # Install library dependency LowPower.h for AVR examples
    pio pkg install --global --library "Low-Power"
    pio pkg install --global --library https://github.com/arduino-libraries/SD
    pio pkg install --global --library "OneWire"
    pio pkg install --global --library "DallasTemperature"
    pio pkg install --global --library "adafruit/Adafruit GFX Library"
    pio pkg install --global --library "adafruit/Adafruit SSD1306"
    pio pkg install --global --library "adafruit/Adafruit BusIO"

    echo "Building examples..."

    # Use option -O "lib_ldf_mode=chain+" to parse defines
    platformio ci -O "lib_ldf_mode=chain+" --lib="." ${BOARDS_AVR} examples/ErriezOregonTHN128Receive/ErriezOregonTHN128Receive.ino
    platformio ci -O "lib_ldf_mode=chain+" --lib="." ${BOARDS_AVR} examples/ErriezOregonTHN128ReceiveSSD1306/ErriezOregonTHN128ReceiveSSD1306.ino
    platformio ci -O "lib_ldf_mode=chain+" --lib="." ${BOARDS_AVR} examples/ErriezOregonTHN128Transmit/ErriezOregonTHN128Transmit.ino
    platformio ci -O "lib_ldf_mode=chain+" --lib="." ${BOARDS_AVR} examples/ErriezOregonTHN128TransmitDS1820/ErriezOregonTHN128TransmitDS1820.ino

    platformio ci -O "lib_ldf_mode=chain+" --lib="." ${BOARDS_ESP8266} ${BOARDS_ESP32} examples/ErriezOregonTHN128Receive/ErriezOregonTHN128Receive.ino
    platformio ci -O "lib_ldf_mode=chain+" --lib="." ${BOARDS_ESP8266} ${BOARDS_ESP32} examples/ErriezOregonTHN128ReceiveSSD1306/ErriezOregonTHN128ReceiveSSD1306.ino
    platformio ci -O "lib_ldf_mode=chain+" --lib="." ${BOARDS_ESP8266} ${BOARDS_ESP32} examples/ErriezOregonTHN128Transmit/ErriezOregonTHN128Transmit.ino
    platformio ci -O "lib_ldf_mode=chain+" --lib="." ${BOARDS_ESP8266} ${BOARDS_ESP32} examples/ErriezOregonTHN128TransmitDS1820/ErriezOregonTHN128TransmitDS1820.ino
}

function generate_doxygen()
{
    if [ ! -f Doxyfile ]; then
        return
    fi

    echo "Generate Doxygen HTML..."

    # Generate Doxygen HTML and Latex
    doxygen Doxyfile

    # Allow filenames starting with an underscore
    echo "" > docs/html/.nojekyll

    # Generate Doxygen PDF
    make -C docs/latex

    # Copy PDF to root directory
    cp docs/latex/refman.pdf ./${DOXYGEN_PDF}

    # Cleanup
    #rm -rf docs
}

setup_virtualenv
autobuild
generate_doxygen