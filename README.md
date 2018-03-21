# mbed-stress-test

### Filesystem and FlashIAP stress testing:

 * Use mbed OS POSIX API to write 800 KiB file to storage using 1 byte - 32KiB buffers.
 * Use mbed OS FlashIAP driver to fill internal flash with data.
 * Test concurrency, read data from file and write to flash.
 
 Test:
 * Filesystem: 
   * Write a large file to filesystem and read it back again. 
   * Tests filesystem works with large files.
 * FlashIAP:
   * Write a large file to internal flash and read it back again. 
   * Tests driver can write to internal flash.
 * File-to-flash:
   * Read a file from filesystem and store it in internal flash.
   * Tests if FlashIAP and SPI can work concurrently.
   
### Network and TLS stress testing

 * Use mbed OS TCP Socket to download file over HTTPS.
 * Use mbedTLS to provide transport layer security.

Test:
 * Network-http:
   * Download large file over HTTP.
   * Tests network robustness.
 * Network-https:
   * Download large file over HTTPS.
   * Tests TLS robustness.
 * Network-multiple-http:
   * Download multiple files simultaneously over HTTP.
   * Tests the network stack can service multiple sockets concurrently.
 * Network-multiple-https:
   * Download multiple files simultaneously over HTTPS.
   * Tests the TLS stack can service multiple contexts concurrently.

### Usage

 * Compile: `mbed test --compile -m TARGET -t TOOLCHAIN --app-config mbed_app.json -n '*stress*'`
 * Run: `mbedgt -vV`
 * Note: the tests can run for 60 minutes.

Example output:
```
mbedgt: test case report:
+--------------+---------------+-------------------------------------+--------------------+--------+--------+--------+--------------------+
| target       | platform_name | test suite                          | test case          | passed | failed | result | elapsed_time (sec) |
+--------------+---------------+-------------------------------------+--------------------+--------+--------+--------+--------------------+
| K66F-GCC_ARM | K66F          | tests-stress-file-to-flash          | Buffer  4k         | 1      | 0      | OK     | 8.67               |
| K66F-GCC_ARM | K66F          | tests-stress-file-to-flash          | Buffer  8k         | 1      | 0      | OK     | 8.47               |
| K66F-GCC_ARM | K66F          | tests-stress-file-to-flash          | Buffer 16k         | 1      | 0      | OK     | 8.34               |
| K66F-GCC_ARM | K66F          | tests-stress-file-to-flash          | Buffer 32k         | 1      | 0      | OK     | 8.2                |
| K66F-GCC_ARM | K66F          | tests-stress-file-to-flash          | Setup              | 1      | 0      | OK     | 9.57               |
| K66F-GCC_ARM | K66F          | tests-stress-filesystem             | Format storage     | 1      | 0      | OK     | 5.97               |
| K66F-GCC_ARM | K66F          | tests-stress-filesystem             | story     1        | 1      | 0      | OK     | 6.89               |
| K66F-GCC_ARM | K66F          | tests-stress-filesystem             | story   128        | 1      | 0      | OK     | 5.58               |
| K66F-GCC_ARM | K66F          | tests-stress-filesystem             | story   256        | 1      | 0      | OK     | 5.74               |
| K66F-GCC_ARM | K66F          | tests-stress-filesystem             | story   512        | 1      | 0      | OK     | 5.26               |
| K66F-GCC_ARM | K66F          | tests-stress-filesystem             | story  1k          | 1      | 0      | OK     | 5.72               |
| K66F-GCC_ARM | K66F          | tests-stress-filesystem             | story  2k          | 1      | 0      | OK     | 4.36               |
| K66F-GCC_ARM | K66F          | tests-stress-filesystem             | story  4k          | 1      | 0      | OK     | 4.32               |
| K66F-GCC_ARM | K66F          | tests-stress-filesystem             | story  8k          | 1      | 0      | OK     | 3.63               |
| K66F-GCC_ARM | K66F          | tests-stress-filesystem             | story 16k          | 1      | 0      | OK     | 3.47               |
| K66F-GCC_ARM | K66F          | tests-stress-filesystem             | story 32k          | 1      | 0      | OK     | 3.47               |
| K66F-GCC_ARM | K66F          | tests-stress-flashiap               | Flash test         | 1      | 0      | OK     | 52.54              |
| K66F-GCC_ARM | K66F          | tests-stress-network-http           | Download  1k       | 1      | 0      | OK     | 79.91              |
| K66F-GCC_ARM | K66F          | tests-stress-network-http           | Download  2k       | 1      | 0      | OK     | 79.9               |
| K66F-GCC_ARM | K66F          | tests-stress-network-http           | Download  4k       | 1      | 0      | OK     | 79.93              |
| K66F-GCC_ARM | K66F          | tests-stress-network-http           | Download  8k       | 1      | 0      | OK     | 79.9               |
| K66F-GCC_ARM | K66F          | tests-stress-network-http           | Download 16k       | 1      | 0      | OK     | 79.95              |
| K66F-GCC_ARM | K66F          | tests-stress-network-http           | Download 32k       | 1      | 0      | OK     | 79.91              |
| K66F-GCC_ARM | K66F          | tests-stress-network-http           | Setup network      | 1      | 0      | OK     | 2.53               |
| K66F-GCC_ARM | K66F          | tests-stress-network-https          | Download  1k       | 1      | 0      | OK     | 76.39              |
| K66F-GCC_ARM | K66F          | tests-stress-network-https          | Download  2k       | 1      | 0      | OK     | 76.22              |
| K66F-GCC_ARM | K66F          | tests-stress-network-https          | Download  4k       | 1      | 0      | OK     | 76.16              |
| K66F-GCC_ARM | K66F          | tests-stress-network-https          | Download  8k       | 1      | 0      | OK     | 80.11              |
| K66F-GCC_ARM | K66F          | tests-stress-network-https          | Download 16k       | 1      | 0      | OK     | 80.11              |
| K66F-GCC_ARM | K66F          | tests-stress-network-https          | Download 32k       | 1      | 0      | OK     | 81.16              |
| K66F-GCC_ARM | K66F          | tests-stress-network-https          | Setup network      | 1      | 0      | OK     | 2.53               |
| K66F-GCC_ARM | K66F          | tests-stress-network-multiple-http  | Download 1 thread  | 1      | 0      | OK     | 5.33               |
| K66F-GCC_ARM | K66F          | tests-stress-network-multiple-http  | Download 2 threads | 1      | 0      | OK     | 5.27               |
| K66F-GCC_ARM | K66F          | tests-stress-network-multiple-http  | Download 3 threads | 1      | 0      | OK     | 6.25               |
| K66F-GCC_ARM | K66F          | tests-stress-network-multiple-http  | Setup network      | 1      | 0      | OK     | 2.55               |
| K66F-GCC_ARM | K66F          | tests-stress-network-multiple-https | Download 1 thread  | 1      | 0      | OK     | 7.01               |
| K66F-GCC_ARM | K66F          | tests-stress-network-multiple-https | Download 2 threads | 1      | 0      | OK     | 10.92              |
| K66F-GCC_ARM | K66F          | tests-stress-network-multiple-https | Setup network      | 1      | 0      | OK     | 3.04               |
| K66F-GCC_ARM | K66F          | tests-stress-network-range-http     | Download  1k       | 1      | 0      | OK     | 180.2              |
| K66F-GCC_ARM | K66F          | tests-stress-network-range-http     | Download  2k       | 1      | 0      | OK     | 171.82             |
| K66F-GCC_ARM | K66F          | tests-stress-network-range-http     | Download  4k       | 1      | 0      | OK     | 126.85             |
| K66F-GCC_ARM | K66F          | tests-stress-network-range-http     | Download  8k       | 1      | 0      | OK     | 104.94             |
| K66F-GCC_ARM | K66F          | tests-stress-network-range-http     | Download 16k       | 1      | 0      | OK     | 84.29              |
| K66F-GCC_ARM | K66F          | tests-stress-network-range-http     | Download 32k       | 1      | 0      | OK     | 83.46              |
| K66F-GCC_ARM | K66F          | tests-stress-network-range-http     | Setup network      | 1      | 0      | OK     | 2.54               |
| K66F-GCC_ARM | K66F          | tests-stress-network-range-https    | Download  1k       | 1      | 0      | OK     | 388.37             |
| K66F-GCC_ARM | K66F          | tests-stress-network-range-https    | Download  2k       | 1      | 0      | OK     | 277.15             |
| K66F-GCC_ARM | K66F          | tests-stress-network-range-https    | Download  4k       | 1      | 0      | OK     | 179.97             |
| K66F-GCC_ARM | K66F          | tests-stress-network-range-https    | Download  8k       | 1      | 0      | OK     | 132.73             |
| K66F-GCC_ARM | K66F          | tests-stress-network-range-https    | Download 16k       | 1      | 0      | OK     | 108.89             |
| K66F-GCC_ARM | K66F          | tests-stress-network-range-https    | Download 32k       | 1      | 0      | OK     | 91.93              |
| K66F-GCC_ARM | K66F          | tests-stress-network-range-https    | Setup network      | 1      | 0      | OK     | 2.54               |
+--------------+---------------+-------------------------------------+--------------------+--------+--------+--------+--------------------+
mbedgt: test case results: 52 OK
mbedgt: completed in 3389.84 sec
```

### Frequently Asked Questions

**Compilation fails due to missing `tls_socket.h`**

* If run `mbed deploy` check for any errors, if you have not installed Mercurial it cannot fetch the [tls_socket.h](https://os.mbed.com/teams/sandbox/code/mbed-http/file/3004056e4661/source/tls_socket.h/), i.e. command `hg` will fail.
* You can install Mercurial in Linux via `sudo apt-get install mercurial`.

**Compilation fails to various SSL-issues**

* Likely the board you are trying does not have Mbed TLS entropy implemented. You can add these macro -definitions to work around that problem (but bear in mind - this is **no way** for a production secure solution).

```json
    "macros": [
        "MBED_CONF_APP_MAIN_STACK_SIZE=10240",  "MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES", "MBEDTLS_TEST_NULL_ENTROPY"

```
You will gets lots of warning during compilation with those, though.
