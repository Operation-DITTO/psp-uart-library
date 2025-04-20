# psp-uart-library

## About

This is the source for the psp-uart-library. It is designed to simplify communication with the PlayStation Portable's UART hardware for serial I/O.

## Building

All that is required to build is the PSPSDK. Run `make` in the root directory to build the libpspuart.a binary.

When including this library be sure to also include `-lpsphprm_driver`.

For examle `LIBS = ./libs/libpspuart.a -lpsphprm_driver`.

### Building as a PRX

Currently this library is configured as an Archive (.a) but it can be compiled into a standalone PRX which could be imported into a user mode EBOOT. Note an `exports.exp` has already been provided and configured.

To do so follow these steps:

* Add the following to `main.c`

```
PSP_MODULE_INFO("pspUART", 0x1006, 1, 1);
PSP_NO_CREATE_MAIN_THREAD();

int module_start(SceSize args, void *argp) {
    return 0;
}

int module_stop(void) {
    return 0;
}
```
* Comment out `alib = libpspuart.a` in the `makefile`
* Comment out option `-mno-crt0` in the `LDFLAGS` section of the `makefile`

A `PRX` will now be generated which can be imported using KUBridge for example.

## Usage

See the [header file](pspuart.h) for a list of functions to interface with the UART hardware.

## Extra Credits

psp-uart-library is a fork of DavisDev's [SioDriver](https://github.com/DavisDev/SioDriver) library, designed to be cleaner and more concise. The following are the extra credits listed under the SioDriver source header.

* Tyranid - SIO based on psplink
* forum.ps2dev.org - More info over the port
* Jean - initial SIO driver