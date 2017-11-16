# mbed-stress-test

Filesystem and FlashIAP stress testing:

* Use mbed OS POSIX API to write 800 KiB file to storage using 1 byte - 32KiB buffers.
* Use mbed OS FlashIAP driver to fill internal flash with data.
* Test concurrency, read data from file and write to flash.

Usage:
* Compile: `mbed test --compile -m TARGET -t TOOLCHAIN -n '*stress*'`
* Run: `mbedgt -vV`
* Note: the tests can run for 60 minutes.

```
