# psp-uart-library

## About

This is the source for the psp-uart-library. It is designed to simplify communication with the PlayStation Portable's UART hardware for serial I/O.

## Building

All that is required to build is the PSPSDK. Run `make` in the root directory to build the libpspuart.a binary.

## Usage

See the [header file](pspuart.h) for a list of functions to interface with the UART hardware.

## Extra Credits

psp-uart-library is a fork of DavisDev's [SioDriver](https://github.com/DavisDev/SioDriver) library, designed to be cleaner and more concise. The following are the extra credits listed under the SioDriver source header.

* Tyranid - SIO based on psplink
* forum.ps2dev.org - More info over the port
* Jean - initial SIO driver