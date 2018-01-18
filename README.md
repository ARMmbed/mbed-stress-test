# mbed-stress-test

### Filesystem and FlashIAP stress testing:

 * Use mbed OS POSIX API to write 800 KiB file to storage using 1 byte - 32KiB buffers.
 * Use mbed OS FlashIAP driver to fill internal flash with data.
 * Test concurrency, read data from file and write to flash.

### Network and TLS stress testing

 * Use mbed OS TCP Socket to download file over HTTPS.
 * Use mbedTLS to provide transport layer security.

### Usage

 * Compile: `mbed test --compile -m TARGET -t TOOLCHAIN -n '*stress*'`
 * Run: `mbedgt -vV`
 * Note: the tests can run for 60 minutes.

Example output:
```
mbedgt: test case report:
+--------------+---------------+-----------------------------+----------------+--------+--------+--------+--------------------+
| target       | platform_name | test suite                  | test case      | passed | failed | result | elapsed_time (sec) |
+--------------+---------------+-----------------------------+----------------+--------+--------+--------+--------------------+
| K66F-GCC_ARM | K66F          | tests-stress-download-https | Download  1k   | 1      | 0      | OK     | 76.36              |
| K66F-GCC_ARM | K66F          | tests-stress-download-https | Download  2k   | 1      | 0      | OK     | 76.23              |
| K66F-GCC_ARM | K66F          | tests-stress-download-https | Download  4k   | 1      | 0      | OK     | 80.16              |
| K66F-GCC_ARM | K66F          | tests-stress-download-https | Download  8k   | 1      | 0      | OK     | 80.15              |
| K66F-GCC_ARM | K66F          | tests-stress-download-https | Download 16k   | 1      | 0      | OK     | 81.09              |
| K66F-GCC_ARM | K66F          | tests-stress-download-https | Download 32k   | 1      | 0      | OK     | 81.14              |
| K66F-GCC_ARM | K66F          | tests-stress-download-https | Setup network  | 1      | 0      | OK     | 2.55               |
| K66F-GCC_ARM | K66F          | tests-stress-file-to-flash  | Buffer  4k     | 1      | 0      | OK     | 8.74               |
| K66F-GCC_ARM | K66F          | tests-stress-file-to-flash  | Buffer  8k     | 1      | 0      | OK     | 8.51               |
| K66F-GCC_ARM | K66F          | tests-stress-file-to-flash  | Buffer 16k     | 1      | 0      | OK     | 8.37               |
| K66F-GCC_ARM | K66F          | tests-stress-file-to-flash  | Buffer 32k     | 1      | 0      | OK     | 8.26               |
| K66F-GCC_ARM | K66F          | tests-stress-file-to-flash  | Setup          | 1      | 0      | OK     | 21.95              |
| K66F-GCC_ARM | K66F          | tests-stress-filesystem     | Format storage | 1      | 0      | OK     | 18.84              |
| K66F-GCC_ARM | K66F          | tests-stress-filesystem     | story     1    | 1      | 0      | OK     | 6.22               |
| K66F-GCC_ARM | K66F          | tests-stress-filesystem     | story   128    | 1      | 0      | OK     | 4.8                |
| K66F-GCC_ARM | K66F          | tests-stress-filesystem     | story   256    | 1      | 0      | OK     | 4.79               |
| K66F-GCC_ARM | K66F          | tests-stress-filesystem     | story   512    | 1      | 0      | OK     | 4.78               |
| K66F-GCC_ARM | K66F          | tests-stress-filesystem     | story  1k      | 1      | 0      | OK     | 4.82               |
| K66F-GCC_ARM | K66F          | tests-stress-filesystem     | story  2k      | 1      | 0      | OK     | 4.12               |
| K66F-GCC_ARM | K66F          | tests-stress-filesystem     | story  4k      | 1      | 0      | OK     | 3.73               |
| K66F-GCC_ARM | K66F          | tests-stress-filesystem     | story  8k      | 1      | 0      | OK     | 3.56               |
| K66F-GCC_ARM | K66F          | tests-stress-filesystem     | story 16k      | 1      | 0      | OK     | 3.39               |
| K66F-GCC_ARM | K66F          | tests-stress-filesystem     | story 32k      | 1      | 0      | OK     | 3.33               |
| K66F-GCC_ARM | K66F          | tests-stress-flashiap       | Flash test     | 1      | 0      | OK     | 53.34              |
+--------------+---------------+-----------------------------+----------------+--------+--------+--------+--------------------+
mbedgt: test case results: 24 OK
mbedgt: completed in 765.45 sec
```
