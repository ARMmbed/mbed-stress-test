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

#include <string>

using namespace utest::v1;

#include MBED_CONF_APP_PROTAGONIST_DOWNLOAD
#include "certificate_aws_s3.h"

static volatile bool event_fired = false;

NetworkInterface* interface = NULL;

#define MAX_RETRIES 3

const char part1[] = "GET /firmware/";
const char filename[] = MBED_CONF_APP_PROTAGONIST_DOWNLOAD;
const char part2[] = "txt HTTP/1.1\nHost: lootbox.s3.dualstack.us-west-2.amazonaws.com\n\n";

static void socket_event(void)
{
    event_fired = true;
}

void download(size_t size)
{
    int result = -1;

    /* setup TCP socket */
    TCPSocket* tcpsocket = new TCPSocket();
    TEST_ASSERT_NOT_NULL_MESSAGE(tcpsocket, "failed to instantiate tlssocket");

    result = tcpsocket->open(interface);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "failed to open socket");

    for (int tries = 0; tries < MAX_RETRIES; tries++) {
        SocketAddress address;

        NetworkInterface::get_default_instance()->gethostbyname("lootbox.s3.dualstack.us-west-2.amazonaws.com", &address);
        address.set_port(80);

        result = tcpsocket->connect(address);
        if (result == NSAPI_ERROR_OK) {
            break;
        }
        printf("connection failed. retry %d of %d\r\n", tries, MAX_RETRIES);
    }
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "failed to connect");

    tcpsocket->set_blocking(false);
    printf("non-blocking mode set\r\n");

    tcpsocket->sigio(socket_event);
    printf("registered callback function\r\n");


    /* setup request */
    /* -1 to remove h from .h in header file name */
    size_t request_size = strlen(part1) + strlen(filename) - 1 + strlen(part2) + 1;
    char *request = new char[request_size]();

    /* construct request */
    memcpy(&request[0], part1, strlen(part1));
    memcpy(&request[strlen(part1)], filename, strlen(filename) - 1);
    memcpy(&request[strlen(part1) + strlen(filename) - 1], part2, strlen(part2));

    printf("request: %s[end]\r\n", request);


    /* send request to server */
    result = tcpsocket->send(request, request_size);
    TEST_ASSERT_EQUAL_INT_MESSAGE(request_size, result, "failed to send");


    /* read response */
    char* receive_buffer = new char[size];
    TEST_ASSERT_NOT_NULL_MESSAGE(receive_buffer, "failed to allocate receive buffer");

    size_t expected_bytes = sizeof(story);
    size_t received_bytes = 0;
    uint32_t body_index = 0;


    /* loop until all expected bytes have been received */
    while (received_bytes < expected_bytes)
    {
        /* wait for async event */
        while(!event_fired);
        event_fired = false;

        /* loop until all data has been read from socket */
        do
        {
            result = tcpsocket->recv(receive_buffer, size);
            TEST_ASSERT_MESSAGE((result == NSAPI_ERROR_WOULD_BLOCK) || (result >= 0), "failed to read socket");

//            printf("result: %d\r\n", result);

            if (result > 0)
            {
                /* skip HTTP header */
                if (body_index == 0)
                {
                    std::string header(receive_buffer, result);
                    body_index = header.find("\r\n\r\n");
                    TEST_ASSERT_MESSAGE(body_index != std::string::npos, "failed to find body");

                    /* remove header before comparison */
                    memmove(receive_buffer, &receive_buffer[body_index + 4], result - body_index - 4);

                    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(story,
                                                         receive_buffer,
                                                         result - body_index - 4,
                                                         "character mismatch");

                    received_bytes += (result - body_index - 4);
                }
                else
                {
                    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(&story[received_bytes],
                                                         receive_buffer,
                                                         result,
                                                         "character mismatch");

                    received_bytes += result;
                }

//                receive_buffer[result] = '\0';
//                printf("%s", receive_buffer);
                printf("received_bytes: %u\r\n", received_bytes);
            }
        }
        while ((result > 0) && (received_bytes < expected_bytes));
    }

    delete request;
    delete tcpsocket;
    delete[] receive_buffer;

    printf("done\r\n");
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
    Case("Download  1k", download_1k),
    Case("Download  2k", download_2k),
    Case("Download  4k", download_4k),
    Case("Download  8k", download_8k),
//    Case("Download 16k", download_16k),
//    Case("Download 32k", download_32k),
};

Specification specification(greentea_setup, cases);

int main()
{
    return !Harness::run(specification);
}
