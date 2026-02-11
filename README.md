# YUIOP29RE - DIY keyboard with rotaly encoder

[![Build](https://github.com/koron/yuiop29re/actions/workflows/build.yml/badge.svg)](https://github.com/koron/yuiop29re/actions/workflows/build.yml)

## Requirements

*   [PICO SDK][picosdk] (v2.2.0)
    * cmake
    * compiler or so
*   [picotool][picotool]
*   [raspberrypi/openocd][openocd] (OPTIONAL: when writing a program using [RaspberryPi Debug Probe][probe])
    
[picosdk]:https://github.com/raspberrypi/pico-sdk
[picotool]:https://github.com/raspberrypi/picotool
[openocd]:https://github.com/raspberrypi/openocd
[probe]:https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html

## Getting started

### How to build

To build for RP2040:

```console
$ make PICO_PLATFORM=rp2040
```

To build for RP2350:

```console
$ make PICO_PLATFORM=rp2350
```

### How to write a program

To write the built program via a [RaspberryPi Debug Probe][probe]:

```console
$ ./bin/program ./build/rp2040/tests/helloworld/helloworld.elf
```

To write the built program in BOOTSEL mode:

1.  Press and hold the RESET button
2.  Press and hold the BOOT button
3.  Release the RESET button
4.  Release the BOOT button
5.  Copy the built program to the device directory/drive

    ```console
    $ cp ./build/rp2040/tests/helloworld/helloworld.elf /e/
    ```
