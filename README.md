# BlockCraft base

This repository contains embedded code for the Raspberry Pi Pico microcontroller which controls the base unit of the BlockCraft kids toy.

## Building the code

The project uses the Raspberry Pi Pico SDK and is managed by cmake. To build it you need to first set up the SDK and the ARM toolchain. Instructions for this can be found in the [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) handbook. Make sure that the PICO_SDK_PATH is set correctly.

The project also makes use of git submodules. To download them use the following command from the project's root directory:

```
$ git submodule update --init --recursive
```

Once this is done, use cmake to build the project. The following shows how you might do this in Unix (starting in the project's root directory):


```
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .
```

An executable for the main code can be found in `build/src/blockcraft_base/`. Executables for module tests can be found in `build/tests/`. Instructions for uploading executables can be found in the handbook mentioned above, but the simplest way is to plug the Pico into your computer using a USB cable while holding down the BOOTSEL button. It should then show up as a mass storage device. Simply copy the `blockcraft_base.uf2` file into the Pico and it should automatically upload the code and start running it.