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

#include "mbed.h"

#include "utest/utest.h"
#include "unity/unity.h"
#include "greentea-client/test_env.h"

#include "easy-connect.h"
#include "tls_socket.h"

using namespace utest::v1;

#ifndef MBED_CONF_APP_DEFAULT_DOWNLOAD_SIZE
#define MBED_CONF_APP_DEFAULT_DOWNLOAD_SIZE 1024
#endif

#include MBED_CONF_APP_PROTAGONIST_DOWNLOAD
#include "certificate_aws_s3.h"

#warning workaroud in place for missing thread safety in mbedtls_hardware_poll
Mutex tlsbug;

static uint32_t shared_thread_counter = 0;
static volatile bool event_fired[5] = { false, false, false, false, false };

NetworkInterface* interface = NULL;

#define MAX_RETRIES 3

const char part1[] = "GET /firmware/";
const char filename[] = MBED_CONF_APP_PROTAGONIST_DOWNLOAD;
const char part2[] = "txt HTTP/1.1\nHost: lootbox.s3.dualstack.us-west-2.amazonaws.com\n\n";

static void socket_event_0(void)
{
    event_fired[0] = true;
}

static void socket_event_1(void)
{
    event_fired[1] = true;
}

static void socket_event_2(void)
{
    event_fired[2] = true;
}

static void socket_event_3(void)
{
    event_fired[3] = true;
}

static void socket_event_4(void)
{
    event_fired[4] = true;
}

void download(void)
{
    size_t size = MBED_CONF_APP_DEFAULT_DOWNLOAD_SIZE;
    int result = -1;

    uint32_t thread_id = core_util_atomic_incr_u32(&shared_thread_counter, 1) - 1;

    /* setup TLS socket */
    TLSSocket* tlssocket = new TLSSocket(interface, "lootbox.s3.dualstack.us-west-2.amazonaws.com", 443, SSL_CA_PEM);
    TEST_ASSERT_NOT_NULL_MESSAGE(tlssocket, "failed to instantiate tlssocket");

    tlssocket->set_debug(true);
    printf("%lu: debug mode set\r\n", thread_id);

    for (int tries = 0; tries < MAX_RETRIES; tries++) {

        tlsbug.lock();
        result = tlssocket->connect();
        TEST_ASSERT_MESSAGE(result != NSAPI_ERROR_NO_SOCKET, "out of sockets");
        tlsbug.unlock();

        if (result == 0) {
            break;
        }

        printf("%lu: connection failed: %d. retry %d of %d\r\n", thread_id, result, tries, MAX_RETRIES);
        Thread::wait(1000);
    }
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "failed to connect");

    TCPSocket* tcpsocket = tlssocket->get_tcp_socket();
    TEST_ASSERT_NOT_NULL_MESSAGE(tlssocket, "failed to get underlying TCPSocket");

    tcpsocket->set_blocking(false);
    printf("%lu: non-blocking mode set\r\n", thread_id);

    if (thread_id == 0) {
        tcpsocket->sigio(socket_event_0);
    } else if (thread_id == 1) {
        tcpsocket->sigio(socket_event_1);
    } else if (thread_id == 2) {
        tcpsocket->sigio(socket_event_2);
    } else if (thread_id == 3) {
        tcpsocket->sigio(socket_event_3);
    } else if (thread_id == 4) {
        tcpsocket->sigio(socket_event_4);
    } else {
        TEST_ASSERT_MESSAGE(0, "wrong thread id");
    }
    printf("%lu: registered callback function\r\n", thread_id);

    /* setup request */
    /* -1 to remove h from .h in header file name */
    size_t request_size = strlen(part1) + strlen(filename) - 1 + strlen(part2);
    char request[request_size];

    /* construct request */
    memcpy(&request[0], part1, strlen(part1));
    memcpy(&request[strlen(part1)], filename, strlen(filename) - 1);
    memcpy(&request[strlen(part1) + strlen(filename) - 1], part2, strlen(part2));

    printf("%lu: request: %s[end]\r\n", thread_id, request);


    /* send request to server */
    result = mbedtls_ssl_write(tlssocket->get_ssl_context(), (const unsigned char*) request, request_size);
    TEST_ASSERT_MESSAGE(result != MBEDTLS_ERR_SSL_WANT_READ, "failed to write ssl");
    TEST_ASSERT_MESSAGE(result != MBEDTLS_ERR_SSL_WANT_WRITE, "failed to write ssl");
    TEST_ASSERT_EQUAL_INT_MESSAGE(request_size, result, "failed to write ssl");


    /* read response */
    char* receive_buffer = new char[size];
    TEST_ASSERT_NOT_NULL_MESSAGE(receive_buffer, "failed to allocate receive buffer");

    size_t expected_bytes = sizeof(story);
    size_t received_bytes = 0;
    uint32_t body_index = 0;


    /* loop until all expected bytes have been received */
    while (received_bytes < expected_bytes) {

        /* wait for async event */
        while(!event_fired[thread_id]) {

            Thread::yield();
        }
        event_fired[thread_id] = false;

        /* loop until all data has been read from socket */
        do {

            result = mbedtls_ssl_read(tlssocket->get_ssl_context(), (unsigned char*) receive_buffer, size - 1);
            TEST_ASSERT_MESSAGE((result == MBEDTLS_ERR_SSL_WANT_READ) || (result >= 0), "failed to read ssl");

//            printf("result: %d\r\n", result);

            if (result > 0) {

                /* skip HTTP header */
                if (body_index == 0) {

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

                } else {

                    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(&story[received_bytes],
                                                         receive_buffer,
                                                         result,
                                                         "character mismatch");

                    received_bytes += result;
                }

//                receive_buffer[result] = '\0';
//                printf("%s", receive_buffer);
                printf("%lu: received_bytes: %u\r\n", thread_id, received_bytes);
            }
        }
        while ((result > 0) && (received_bytes < expected_bytes));

        if (result == MBEDTLS_ERR_SSL_WANT_WRITE) {

            printf("%lu: MBEDTLS_ERR_SSL_WANT_WRITE: %d\r\n", thread_id, MBEDTLS_ERR_SSL_WANT_WRITE);
            break;
        }
    }

    delete tlssocket;
    delete[] receive_buffer;

    printf("%lu: done\r\n", thread_id);
}

static control_t setup_network(const size_t call_count)
{
    interface = easy_connect(true);
    TEST_ASSERT_NOT_NULL_MESSAGE(interface, "failed to initialize network");

    return CaseNext;
}

static control_t download_1(const size_t call_count)
{
    Thread t1;

    shared_thread_counter = 0;
    t1.start(download);

    t1.join();

    return CaseNext;
}

static control_t download_2(const size_t call_count)
{
    Thread t1;
    Thread t2;

    shared_thread_counter = 0;
    t1.start(download);
    t2.start(download);

    t1.join();
    t2.join();

    return CaseNext;
}

static control_t download_3(const size_t call_count)
{
    Thread t1;
    Thread t2;
    Thread t3;

    shared_thread_counter = 0;
    t1.start(download);
    t2.start(download);
    t3.start(download);

    t1.join();
    t2.join();
    t3.join();

    return CaseNext;
}

static control_t download_4(const size_t call_count)
{
    Thread t1;
    Thread t2;
    Thread t3;
    Thread t4;

    shared_thread_counter = 0;
    t1.start(download);
    t2.start(download);
    t3.start(download);
    t4.start(download);

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    return CaseNext;
}

static control_t download_5(const size_t call_count)
{
    Thread t1;
    Thread t2;
    Thread t3;
    Thread t4;
    Thread t5;

    shared_thread_counter = 0;
    t1.start(download);
    t2.start(download);
    t3.start(download);
    t4.start(download);
    t5.start(download);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();

    return CaseNext;
}

utest::v1::status_t greentea_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(30*60, "default_auto");
    return greentea_test_setup_handler(number_of_cases);
}

Case cases[] = {
    Case("Setup network", setup_network),
    Case("Download 1 thread",  download_1),
    Case("Download 2 threads", download_2),
//    Case("Download 3 threads", download_3),
//    Case("Download 4 threads", download_4),
//    Case("Download 5 threads", download_5),
};

Specification specification(greentea_setup, cases);

int main()
{
    return !Harness::run(specification);
}
