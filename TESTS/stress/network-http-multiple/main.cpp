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
#include "mbed_trace.h"

#include <string>

using namespace utest::v1;

#define TRACE_GROUP  "STRS"

#ifndef MBED_CONF_APP_DEFAULT_DOWNLOAD_SIZE
#define MBED_CONF_APP_DEFAULT_DOWNLOAD_SIZE 1024
#endif

#ifdef MBED_CONF_APP_MAIN_STACK_SIZE
#define THREAD_STACK_SIZE MBED_CONF_APP_MAIN_STACK_SIZE
#else
#define THREAD_STACK_SIZE (5 * 1024)
#endif

#include MBED_CONF_APP_PROTAGONIST_DOWNLOAD
#include "certificate_aws_s3.h"

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

    /* setup TCP socket */
    TCPSocket* tcpsocket = new TCPSocket();
    TEST_ASSERT_NOT_NULL_MESSAGE(tcpsocket, "failed to instantiate tlssocket");

    result = tcpsocket->open(interface);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "failed to open socket");

    for (int tries = 0; tries < MAX_RETRIES; tries++) {
        result = tcpsocket->connect("lootbox.s3.dualstack.us-west-2.amazonaws.com", 80);
        TEST_ASSERT_MESSAGE(result != NSAPI_ERROR_NO_SOCKET, "out of sockets");

        if (result == 0) {
            break;
        }

        tr_debug("%lu: connection failed: %d. retry %d of %d", thread_id, result, tries, MAX_RETRIES);
        ThisThread::sleep_for(1000);
    }
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "failed to connect");

    tcpsocket->set_blocking(false);
    tr_debug("%lu: non-blocking mode set", thread_id);

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
    tr_debug("%lu: registered callback function", thread_id);

    /* setup request */
    /* -1 to remove h from .h in header file name */
    size_t request_size = strlen(part1) + strlen(filename) - 1 + strlen(part2) + 1;
    char *request = new char[request_size]();

    /* construct request */
    memcpy(&request[0], part1, strlen(part1));
    memcpy(&request[strlen(part1)], filename, strlen(filename) - 1);
    memcpy(&request[strlen(part1) + strlen(filename) - 1], part2, strlen(part2));

    tr_debug("%lu: request: %s[end]", thread_id, request);


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
    while (received_bytes < expected_bytes) {
        /* wait for async event */
        while(!event_fired[thread_id]) {
            ThisThread::yield();
        }
        event_fired[thread_id] = false;

        /* loop until all data has been read from socket */
        do {
            result = tcpsocket->recv(receive_buffer, size);
            TEST_ASSERT_MESSAGE((result == NSAPI_ERROR_WOULD_BLOCK) || (result >= 0), "failed to read socket");

//            tr_debug("result: %d", result);

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
//                tr_debug("%s", receive_buffer);
                tr_debug("%lu: received_bytes: %u", thread_id, received_bytes);
            }
        }
        while ((result > 0) && (received_bytes < expected_bytes));
    }

    delete request;
    delete tcpsocket;
    delete[] receive_buffer;

    tr_debug("%lu: done", thread_id);
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
    printf("IP address is '%s'\n", interface->get_ip_address());
    printf("MAC address is '%s'\n", interface->get_mac_address());

    return CaseNext;
}

static control_t download_1(const size_t call_count)
{
    Thread* t1 = new Thread(osPriorityNormal, THREAD_STACK_SIZE);
    TEST_ASSERT_NOT_NULL_MESSAGE(t1, "failed to allocate thread 1");

    shared_thread_counter = 0;
    t1->start(download);

    t1->join();

    return CaseNext;
}

static control_t download_2(const size_t call_count)
{
    Thread* t1 = new Thread(osPriorityNormal, THREAD_STACK_SIZE);
    TEST_ASSERT_NOT_NULL_MESSAGE(t1, "failed to allocate thread 1");

    Thread* t2 = new Thread(osPriorityNormal, THREAD_STACK_SIZE);
    TEST_ASSERT_NOT_NULL_MESSAGE(t2, "failed to allocate thread 2");

    shared_thread_counter = 0;
    t1->start(download);
    t2->start(download);

    t1->join();
    t2->join();

    return CaseNext;
}

static control_t download_3(const size_t call_count)
{
    Thread* t1 = new Thread(osPriorityNormal, THREAD_STACK_SIZE);
    TEST_ASSERT_NOT_NULL_MESSAGE(t1, "failed to allocate thread 1");

    Thread* t2 = new Thread(osPriorityNormal, THREAD_STACK_SIZE);
    TEST_ASSERT_NOT_NULL_MESSAGE(t2, "failed to allocate thread 2");

    Thread* t3 = new Thread(osPriorityNormal, THREAD_STACK_SIZE);
    TEST_ASSERT_NOT_NULL_MESSAGE(t3, "failed to allocate thread 3");

    shared_thread_counter = 0;
    t1->start(download);
    t2->start(download);
    t3->start(download);

    t1->join();
    t2->join();
    t3->join();

    return CaseNext;
}

static control_t download_4(const size_t call_count)
{
    Thread* t1 = new Thread(osPriorityNormal, THREAD_STACK_SIZE);
    TEST_ASSERT_NOT_NULL_MESSAGE(t1, "failed to allocate thread 1");

    Thread* t2 = new Thread(osPriorityNormal, THREAD_STACK_SIZE);
    TEST_ASSERT_NOT_NULL_MESSAGE(t2, "failed to allocate thread 2");

    Thread* t3 = new Thread(osPriorityNormal, THREAD_STACK_SIZE);
    TEST_ASSERT_NOT_NULL_MESSAGE(t3, "failed to allocate thread 3");

    Thread* t4 = new Thread(osPriorityNormal, THREAD_STACK_SIZE);
    TEST_ASSERT_NOT_NULL_MESSAGE(t4, "failed to allocate thread 4");

    shared_thread_counter = 0;
    t1->start(download);
    t2->start(download);
    t3->start(download);
    t4->start(download);

    t1->join();
    t2->join();
    t3->join();
    t4->join();

    return CaseNext;
}

static control_t download_5(const size_t call_count)
{
    Thread* t1 = new Thread(osPriorityNormal, THREAD_STACK_SIZE);
    TEST_ASSERT_NOT_NULL_MESSAGE(t1, "failed to allocate thread 1");

    Thread* t2 = new Thread(osPriorityNormal, THREAD_STACK_SIZE);
    TEST_ASSERT_NOT_NULL_MESSAGE(t2, "failed to allocate thread 2");

    Thread* t3 = new Thread(osPriorityNormal, THREAD_STACK_SIZE);
    TEST_ASSERT_NOT_NULL_MESSAGE(t3, "failed to allocate thread 3");

    Thread* t4 = new Thread(osPriorityNormal, THREAD_STACK_SIZE);
    TEST_ASSERT_NOT_NULL_MESSAGE(t4, "failed to allocate thread 4");

    Thread* t5 = new Thread(osPriorityNormal, THREAD_STACK_SIZE);
    TEST_ASSERT_NOT_NULL_MESSAGE(t5, "failed to allocate thread 5");

    shared_thread_counter = 0;
    t1->start(download);
    t2->start(download);
    t3->start(download);
    t4->start(download);
    t5->start(download);

    t1->join();
    t2->join();
    t3->join();
    t4->join();
    t5->join();

    return CaseNext;
}

utest::v1::status_t greentea_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(10*60, "default_auto");
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

rtos::Mutex trace_mutex;

void mbed_trace_helper_mutex_wait(void)
{
    trace_mutex.lock();
}

void mbed_trace_helper_mutex_release(void)
{
    trace_mutex.unlock();
}

int main()
{
    mbed_trace_init();
    mbed_trace_mutex_wait_function_set(mbed_trace_helper_mutex_wait);
    mbed_trace_mutex_release_function_set(mbed_trace_helper_mutex_release);
    mbed_trace_config_set(TRACE_ACTIVE_LEVEL_ALL|TRACE_MODE_COLOR);

    return !Harness::run(specification);
}
