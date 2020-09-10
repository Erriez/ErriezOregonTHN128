#/bin/bash

# Automatic build script
#
# To run locally, execute:
# sudo apt install doxygen graphviz texlive-latex-base texlive-latex-recommended texlive-pictures texlive-latex-extra

# Exit immediately if a command exits with a non-zero status.
set -e

echo "Starting auto-build script..."


function autobuild()
{
    # Set environment variables
    BOARDS_AVR="--board uno --board pro16MHzatmega328 --board pro8MHzatmega328"

    echo "Installing library dependencies"
    # Install library dependency LowPower.h for example DHT22LowPower.ino
    platformio lib --global install "Low-Power"
    platformio lib --global install https://github.com/arduino-libraries/SD
    platformio lib --global install "OneWire"
    platformio lib --global install "adafruit/Adafruit GFX Library"
    platformio lib --global install "adafruit/Adafruit SSD1306"
    platformio lib --global install "adafruit/Adafruit BusIO"

    echo "Building examples..."
    platformio ci --lib="." ${BOARDS_AVR} examples/ErriezOregonTHN128Receive/ErriezOregonTHN128Receive.ino
    platformio ci --lib="." ${BOARDS_AVR} examples/ErriezOregonTHN128ReceiveSSD1306/ErriezOregonTHN128ReceiveSSD1306.ino
    platformio ci --lib="." ${BOARDS_AVR} examples/ErriezOregonTHN128Transmit/ErriezOregonTHN128Transmit.ino
    platformio ci --lib="." ${BOARDS_AVR} examples/ErriezOregonTHN128TransmitDS1820/ErriezOregonTHN128TransmitDS1820.ino
}

function generate_doxygen()
{
    echo "Generate Doxygen HTML..."

    DOXYGEN_PDF="ErriezOregonTHN128.pdf"

    # Cleanup
    rm -rf html latex

    # Generate Doxygen HTML and Latex
    doxygen Doxyfile

    # Allow filenames starting with an underscore    
    echo "" > html/.nojekyll

    # Generate PDF when script is not running on Travis-CI
    if [[ -z ${TRAVIS_BUILD_DIR} ]]; then
        # Generate Doxygen PDF
        make -C latex

        # Copy PDF to root directory
        cp latex/refman.pdf ./${DOXYGEN_PDF}
    fi
}

autobuild
generate_doxygen

