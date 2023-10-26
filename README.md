# Console8 MOS recover utility
## Overview
This utility provides a means to (bare-metal) program a new MOS firmware to a Console8 ez80 flashchip. It needs to be programmed to an external ESP32 development board, such as a ESP32-DevKit.

The external ESP32 needs to be connected to the Console8's 6-pin ZDI interface, using 3 jumpercables, connected as detailed below. After programming the utility, the user simply places the required MOS.bin firmware file on the Console8's SD
card, which the utility picks up and flashes to the Console8 ez80 flashchip.

The utility makes use of the onboard LED to communicate status to the user. The appropriate pins to drive the LED for different development boards are programmed in the utility.

## Usage
After the utility has been programmed to the external ESP32 development board and all requirements have been met, the onboard LED of the board shows the connection status:
- LED Blinking: Cabling issue, or no power on the Console8
- LED Steady: Communication established

Place the required 'MOS.bin' firmware file in the root of the Console8's SD card and press the 'boot' button for at least 2sec, to start the programming cycle.

As there is no direct feedback from the Console8 to the development board, the programming is being done in 'one-shot' mode, taking just several seconds. The Console8 should reset itself as part of finishing the process after 5-10sec. Additional programming attempts are possible after the onboard LED stops blinking.

## Cabling details
### ZDI connector
![zdi connector](/media/zdi.png)

### ESP32 development board example
![ESP32](/media/console8_zdi.png)
Other development boards may have pins placed differently, so be sure to check the layout of your particular board.

## Flashing the utility
The utility can be flashed in multiple ways:
1. By using a the graphical 'ESP32 Flash Download Tool' from Espressif. This is probably the easiest option for most people, but requires a Windows PC. Follow the description in the next paragraph
2. By using one of the provided batch/shell scripts provided in the [batchfile-flash](https://github.com/envenomator/console8-recover/tree/master/batchfile-flash) folder. You'll probably need to tweak the serial setting for your specific COM/UART port. The rest of the settings are included in the script, which are fed to the Espressif 'esptool', also provided in the folder. For repeated, single-command flashing of the ESP32, this is my personal preference, but can be a little more challenging for users.
3. By installing PlatformIO and building/flashing the utility to the ESP32. Describing this process is outside the scope of this document. There are some [excellent tutorials](https://randomnerdtutorials.com/vs-code-platformio-ide-esp32-esp8266-arduino/) to be found online to describe the setup of PlatformIO. After setting up PlatformIO, open up the git folder and build the utility from source.

## Flashing using the ESP Flash download tool
The latest version of this tool can be downloaded [from the Espressif support website](https://www.espressif.com/en/support/download/other-tools?keys=&field_type_tid%5B%5D=13)

Connect your ESP32 development board to your PC, using an appropriate USB interface cable.

After installation of the Espressif ESP Flash download tool, run it and fill in the fields exactly as follows (provide your own COM port details), pointing the binaries where you downloaded this git to on disk. Binaries to flash are in the 'batchfile-flash' folder, which I put in 'C:\bin' in this example:

![espressif settings](/media/esp32flash.PNG)

Then press 'Start', wait for the tool to finish and follow instructions on using the utility below.

## Requirements
1. Three (dupont female-male) cables need to be connected between the external ESP32 development board and ZDI ports of the Console8. Please see detailed description below
2. A breadboard to place your ESP32 and connect it up is optional. It might be possible to directly connect your jumper cables to the pinheader of your board.
3. The required MOS version to program needs to be placed on the SD card's root filesystem, named "MOS.bin"
4. Console8 needs to be 'on' using the slider switch, in order for the ZDI interface to function

## Disclaimer
Having subjected my own gear multiple times to this utility, I feel it's safe for general use in recovering the MOS. The responsibility for any issues during and/or after the use of, it lies with the user of this utility