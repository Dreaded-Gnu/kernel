# MIST

_MIST_ is a recursive acronym for "MIST is somehow terrible". Below are some introductions, help and decisions about mist project. Originally it was derived from the german word mist, which might be translated as _crap_.

## Things to be done

* [x] Cleanup current code mess
  * [x] Move memory barrier header to arm as it is arm related
  * [x] Change peripheral base from constant to function due to later virtual remap
  * [ ] Strip out ATAG and flat device tree parsing into library
  * [x] Prefix folders used by automake with an underscore
* [x] Serial output done within `kernel/vendor/{vendor}`
* [x] TTY for printing debug messages done within `kernel/vendor/{vendor}`
  * [x] printf implementation for kernel environment
* [x] Interrupt requests and fast interrupts
* [ ] Memory management
  * [ ] Transform kernel to higher half with initial mmu ( kernel load address at 0xC0008000 / 0xC0080000 )
    * [ ] Lay initial startup setting up paging within `kernel/arch/{architecture}/{sub architecture}`
    * [ ] Call vendor related startup code
    * [ ] Check exception handler after higher half init
  * [ ] Physical memory management
    * [ ] Get max memory from vendor
    * [ ] Generic physical handling done within `kernel` via memory bitmap
  * [ ] Virtual memory management done within `kernel/arch/{architecture}/{sub architecture}`
  * [ ] Heap management for dynamic memory allocation done within `kernel` using architecture related code
* [ ] Event system for mapping service routines `Needs to be planned`
  * [ ] ~~Generic code for `register` and `unregister` an event done within `kernel`~~
  * [ ] ~~Vendor related mapping~~
    * [ ] ~~Interrupt requests~~
    * [ ] ~~Fast interrupt requests~~
    * [ ] ~~Software interrupts~~
* [ ] Remote debugging support via GDB and serial

## Unordered list of things to be done and ideas

* [x] Add memory barriers for arm necessary for e.g. mailbox on rpi
* [ ] Rework recursive autotools behaviour to non recursive
* [ ] Libraries
  * [ ] Think about merge of compiler library `libgcc.a` and local `libstdc.a`
  * [ ] `libavl.a`
  * [ ] `libtar.a`
  * [ ] Add generic library for atag parsing
* [ ] Device tree
  * [ ] Remove atag parsing
  * [ ] Add parse of device tree
  * [ ] Panic, when there is no device tree
* [ ] Add irq and isrs register handling
  * [x] Get irq with cpu mode switch and register dump working
  * [x] Merge irq functions with isrs functions where possible
  * [ ] Prohibit mapping of interrupt routines
  * [ ] Add event system with `register` and `unregister`
    * [ ] Provide map of events for `irq`, `fiq`, `swi`
    * [ ] Fire events for `irq`, `fiq`, `swi`
* [ ] Memory management
  * [ ] Gather rpi memory from mailbox ( store physical memory map generally per vendor )
  * [ ] Add physical memory management
  * [ ] Add virtual memory management
  * [ ] Add heap management
  * [ ] Consider peripherals per board within mmu as not cachable
  * [ ] Enable CPU caches
* [ ] initrd and vfs
  * [ ] Add parse of some custom initrd.tar
  * [ ] Add virtual file system for initrd
* [ ] Provide kernel implementation for `malloc`, `calloc`, `realloc` and `free`
* [ ] Add gdb stub for debugging on remote device via serial port
  * [ ] Use dynamic memory allocation
  * [ ] Find better place for `serial_init` than `tty_init`
  * [ ] Finish debug launch.json when remote debugging is possible
* [ ] Create a draft for build system with vendor driver/app packaging
* [ ] Documentation ( man pages or markdown )
  * [ ] Getting started after checkout
  * [ ] Cross compiler toolchain
  * [ ] Configuring target overview

## Building the project

### Autotools related

Initial necessary commands after checkout:

```bash
aclocal -I m4
autoheader
automake --foreign --add-missing
autoconf
```

Necessary commands after adding new files:

```bash
autoreconf -i
```

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

Emulation of the project with qemu during development is **not** recommended. The platforms itself are most time very limited or completely not supported. Raspberry pi is very limited and lack necessary , also the n900 or the beagleboard are completely not supported. We recommend testing on real hardware with remote debugging via serial port.

Emulation of the kernel project with qemu during development may be done at all with the following commands. When kernel debugging is necessary, add the parameter shorthand `-s` or the long version `-gdb tcp:1234` to qemu call:

```bash
# raspberry pi 2B rev 1 kernel emulation
qemu-system-arm -M raspi2 -cpu cortex-a7 -m 1G -no-reboot -serial stdio -kernel ./src/kernel/vendor/rpi/kernel_qemu -dtb ../src/kernel/vendor/rpi/device/bcm2709-rpi-2-b.dtb

# raspberry pi 2B rev 2 kernel emulation ( to be tested )
qemu-system-aarch64 -m 1024 -M raspi2 -cpu cortex-a53 -m 1G -no-reboot -serial stdio -kernel ./src/kernel/vendor/rpi/kernel_qemu -dtb ../src/kernel/vendor/rpi/device/bcm2709-rpi-2-b.dtb

# raspberry pi 3B and 3B+ kernel emulation
qemu-system-aarch64 -m 1024 -M raspi3 -cpu cortex-a53 -m 1G -no-reboot -serial stdio -kernel ./src/kernel/vendor/rpi/kernel_qemu -dtb ../src/kernel/vendor/rpi/device/bcm2710-rpi-3-b.dtb
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

* ~~armv6 ( 32 bit - OMAP 2420 )~~
  * ~~nokia n800~~
  * ~~nokia n810~~

* armv7-a ( 32 bit - OMAP3530 )
  * beagle board
  * ~~nokia n900~~

## List of real hardware for testing

* Raspberry PI 2 Model B ( rev 1.1 )
* Raspberry PI 3 Model B
