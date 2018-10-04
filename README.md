# MIST

_MIST_ is a recursive acronym for "MIST is somehow terrible". Below are some introductions, help and decisions about mist project. Originally it was derived from the german word mist, which might be translated as _crap_.

## Things to be done

* Add gdb stub for debugging on remote device via serial port
* Find better place for `serial_init` than `tty_init`

## Building the project

### Configuring

```bash
mkdir build
cd build
### configure with one of the following commands
../configure --host arm-none-eabi --enable-device=rpi1_a --enable-debug --enable-kernel-print
../configure --host arm-none-eabi --enable-device=rpi1_a_plus --enable-debug --enable-kernel-print
../configure --host arm-none-eabi --enable-device=rpi1_b --enable-debug --enable-kernel-print
../configure --host arm-none-eabi --enable-device=rpi1_b_plus --enable-debug --enable-kernel-print
../configure --host arm-none-eabi --enable-device=rpi2_b --enable-debug --enable-kernel-print
../configure --host aarch64-none-elf --enable-device=rpi2_b_rev2 --enable-debug --enable-kernel-print
../configure --host aarch64-none-elf --enable-device=rpi3_b --enable-debug --enable-kernel-print
../configure --host aarch64-none-elf --enable-device=rpi3_b_plus --enable-debug --enable-kernel-print
../configure --host arm-none-eabi --enable-device=rpi_zero --enable-debug --enable-kernel-print
../configure --host arm-none-eabi --enable-device=rpi_zero_w --enable-debug --enable-kernel-print
../configure --host arm-none-eabi --enable-device=beagleboard --enable-debug --enable-kernel-print
../configure --host arm-none-eabi --enable-device=n8x0 --enable-debug --enable-kernel-print
../configure --host arm-none-eabi --enable-device=n900 --enable-debug --enable-kernel-print
```

### Building

```bash
# just call make for building the project
make clean && make
```

### Remote debugging

For remote debugging configure the kernel with `--enable-debug`, rebuild and copy it to remote device. After that, depending on the remote arch, execute one of the following commands
```bash
### debug 32 bit arm device
../toolchain/opt/cross/bin/arm-none-eabi-gdb -b 115200 --tty=/dev/ttyUSB0 ./src/kernel/vendor/rpi/kernel.zwerg ./src/kernel/vendor/rpi/kernel.map

### debug 64 bit arm device
../toolchain/opt/cross/bin/aarch64-none-elf-gdb -b 115200 --tty=/dev/ttyUSB0 ./src/kernel/vendor/rpi/kernel.zwerg ./src/kernel/vendor/rpi/kernel.map
```

### Real hardware

Getting some debug-print on real hardware, like debug debug-print is done via serial. In case of raspberry pi it is UART0. So to get execution debug-print, you'll need an TTL-RS232 to USB convert cable. When that is existing, wire it up like on the following [page](https://blog.christophersmart.com/2016/10/27/building-and-booting-upstream-linux-and-u-boot-for-raspberry-pi-23-arm-boards/) and connect it to the development machine. The most simple way to get the serial debug-print is using screen with the connected usb to serial.

```bash
# Connect to usb tty port via screen
screen /dev/ttyUSB0 115200

# Exit tty screen session
Ctrl+a Shift+k y
```

### Emulation

Emulation of the project with qemu during development is not for all platforms recommended. The platform for raspberry pi is very limited and lack necessary features. Further more the n900 or the beagleboard are completely not supported. We recommend testing on real hardware with remote debugging via serial port.

Emulation of the kernel project with qemu during development may be done at all with the following commands. When kernel debugging is necessary, add the parameter shorthand `-s` or the long version `-gdb tcp:1234` to qemu call:

```bash
# raspberry pi 2B rev 1 kernel emulation
qemu-system-arm -m 1024 -M raspi2 -cpu cortex-a7 -no-reboot -serial stdio -kernel ./src/kernel/vendor/rpi/kernel.zwerg

# raspberry pi 2B rev 2 kernel emulation
qemu-system-aarch64 -m 1024 -M raspi2 -cpu cortex-a53 -no-reboot -serial stdio -kernel ./src/kernel/vendor/rpi/kernel.zwerg

# raspberry pi 3B and 3B+ kernel emulation
qemu-system-aarch64 -m 1024 -M raspi2 -cpu cortex-a53 -no-reboot -serial stdio -kernel ./src/kernel/vendor/rpi/kernel.zwerg
```

## Planned support

Currently the following targets are planned to be supported:

### Broadcom SoC

* armv6 ( 32 bit )
  * RPI A
  * RPI A+
  * RPI B
  * RPI B+
  * RPI zero
  * RPI zero w

* armv7-a ( 32 bit )
  * RPI 2B r.1

* armv8-a ( 64 bit - rpi 2 r.2 and rpi 3 series )
  * RPI 2B r.2
  * RPI 3B
  * RPI 3B+

### Texas Instruments OMAP SoC

* armv6 ( 32 bit - OMAP 2420 )
  * nokia n800
  * nokia n810

* armv7-a ( 32 bit - OMAP3530 )
  * beagle board
  * nokia n900

## List of real hardware for testing

* Raspberry PI 2 Model B ( rev 1.1 )
* Raspberry PI 3 Model B
