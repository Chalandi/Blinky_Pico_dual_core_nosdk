Blinky_Pico_dual_core_nosdk
==================

This repository implements an entirely manually-written,
Blinky Project for the RaspberryPi(R) Pico RP2040 dual-core
ARM(R) Cortex(R)-M0+ (RPI PICO RP2040).
It exhibits pure _bare_ _metal_ programming
on on the RaspberryPi(R) Pico RP2040
dual-core ARM(R) Cortex(R)-M0+ and uses no sdk.

Features include:
  - CPU, dual-core, clock and PLL initialization,
  - timebase derived from SysTick,
  - blinky LED show on the green user LED on `port25`,
  - implementation in C99 with absolute minimal use of assembly.

A clear and easy-to-understand build system based on GNUmake
completes this fun and educational project.

This repository provides keen insight on starting up
a _bare_ _metal_ RaspberryPi(R) Pico RP2040 dual-core ARM(R) Cortex(R)-M0+
controller using no sdk.

## Details on the Application

The application boots from the secondary boot-loader (SBL)
at the start location. This low-level startup boots through
core 0. Core 0 then starts up core 1 (via a specific protocol).
Core 1 subsequently carries out the blinky application,
while core 0 enters an endless, idle loop.

Low-level initialization brings the CPU up to full speed
at $133~MHz$. Hardware settings such as wait states
have seemingly been set by the boot-loader.

The blinky LED show utilizes the green user LED on `port25`.

## Building the Application

Build on `*nix*` is easy using an installed `gcc-arm-none-eabi`

```sh
cd Blinky_Pico_dual_core_nosdk
Rebuild.sh
```

The build results including ELF-file, HEX-mask, MAP-file
and assembly list file are created in the `Output` directory.

If `gcc-arm-none-eabi` is not installed, it can easily
be installed with

```sh
sudo apt install gcc-arm-none-eabi
```

## Continuous Integration

CI runs on pushes and pull-requests with a simple
build and result verification on `ubuntu-latest`
using GutHub Actions.
