/*
 * mbed Microcontroller Library
 * Copyright (c) 2006-2016 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** @file fopen.cpp Test cases to POSIX file fopen() interface.
 *
 * Please consult the documentation under the test-case functions for
 * a description of the individual test case.
 */

#define WIFI 2
#if !defined(MBED_CONF_TARGET_NETWORK_DEFAULT_INTERFACE_TYPE) || \
    (MBED_CONF_TARGET_NETWORK_DEFAULT_INTERFACE_TYPE == WIFI && !defined(MBED_CONF_NSAPI_DEFAULT_WIFI_SSID))
#error [NOT_SUPPORTED] No network configuration found for this target.
#endif

#include "mbed.h"

#include "utest/utest.h"
#include "unity/unity.h"
#include "greentea-client/test_env.h"

#include "mbed_stress_test_network.h"

using namespace utest::v1;

#include MBED_CONF_APP_PROTAGONIST_DOWNLOAD

NetworkInterface* interface = NULL;

#define MAX_RETRIES 3

char filename[] = MBED_CONF_APP_PROTAGONIST_DOWNLOAD;

void download(size_t size)
{
    /* remove .h from header file name */
    size_t filename_size = sizeof(filename);
    filename[filename_size - 3] = '\0';

    char* buffer = new char[size];

    size_t offset = 0;

    while (offset < sizeof(story))
    {
        size_t actual_bytes = sizeof(story) - offset;

        if (actual_bytes > size)
        {
            actual_bytes = size;
        }

        size_t received_bytes = mbed_stress_test_download(interface, filename, offset, buffer, actual_bytes, false);
        TEST_ASSERT_EQUAL_INT_MESSAGE(actual_bytes, received_bytes, "received incorrect number of bytes");
        TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(&story[offset],
                                             buffer,
                                             actual_bytes,
                                             "character mismatch");

        offset += received_bytes;
    }

    delete buffer;
}

static control_t setup_network(const size_t call_count)
{
    interface = NetworkInterface::get_default_instance();
    TEST_ASSERT_NOT_NULL_MESSAGE(interface, "failed to initialize network");

    nsapi_error_t err = -1;

    for (int tries = 0; tries < MAX_RETRIES; tries++) {
        err = interface->connect();

        if (err == NSAPI_ERROR_OK) {
            break;
        } else {

            printf("Error connecting to network. Retrying %d of %d\r\n", tries, MAX_RETRIES);
        }
    }

    TEST_ASSERT_EQUAL(NSAPI_ERROR_OK, err);
    SocketAddress address;
    interface->get_ip_address(&address);
    printf("IP address is '%s'\r\n", address.get_ip_address());
    printf("MAC address is '%s'\r\n", interface->get_mac_address());

    return CaseNext;
}

Thread thread;

void download_thread(void)
{
}

static control_t download_1k(const size_t call_count)
{
    download(1024);

    return CaseNext;
}

static control_t download_2k(const size_t call_count)
{
    download(2*1024);

    return CaseNext;
}

static control_t download_4k(const size_t call_count)
{
    download(4*1024);

    return CaseNext;
}

static control_t download_8k(const size_t call_count)
{
    download(8*1024);

    return CaseNext;
}

static control_t download_16k(const size_t call_count)
{
    download(16*1024);

    return CaseNext;
}

static control_t download_32k(const size_t call_count)
{
    download(32*1024);

    return CaseNext;
}

utest::v1::status_t greentea_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(10*60, "default_auto");
    return greentea_test_setup_handler(number_of_cases);
}

Case cases[] = {
    Case("Setup network", setup_network),
//    Case("Download  1k", download_1k),
//    Case("Download  2k", download_2k),
//    Case("Download  4k", download_4k),
    Case("Download  8k", download_8k),
//    Case("Download 16k", download_16k),
//    Case("Download 32k", download_32k),
};

Specification specification(greentea_setup, cases);

int main()
{
    return !Harness::run(specification);
}
