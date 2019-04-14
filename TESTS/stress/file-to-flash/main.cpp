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

#if !DEVICE_FLASH
#error [NOT_SUPPORTED] Flash API not supported for this target.
#endif

#if !(COMPONENT_SPIF || COMPONENT_QSPIF || COMPONENT_DATAFLASH || COMPONENT_SD)
#error [NOT_SUPPORTED] Storage not supported for this target.
#endif

#include "mbed.h"

#include "utest/utest.h"
#include "unity/unity.h"
#include "greentea-client/test_env.h"

using namespace utest::v1;

#include "mbed_stress_test_file.h"
#include "mbed_stress_test_flash.h"

#include MBED_CONF_APP_PROTAGONIST_FILE_TO_FLASH

typedef struct {
    size_t size;
    size_t size_max;
    unsigned char* ptr;
} my_buffer_t;

my_buffer_t buffer0 = { 0 };
my_buffer_t buffer1 = { 0 };

Queue<my_buffer_t, 2> mpool;
Queue<my_buffer_t, 2> queue;

void setup(void)
{
    printf("setup file\r\n");

    mbed_stress_test_format_file();
    mbed_stress_test_write_file("mbed-stress-test.txt", 0, story, sizeof(story), 1024);
}

void file_thread(void)
{
    size_t index = 0;
    while (index < sizeof(story))
    {
        /* get buffer from memory queue */
        osEvent event = mpool.get();

        if (event.status == osEventMessage)
        {
            my_buffer_t* buffer = (my_buffer_t*) event.value.p;
            TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "failed to get buffer");
            TEST_ASSERT_NOT_NULL_MESSAGE(buffer->ptr, "failed to get buffer");

            size_t read = mbed_stress_test_read_file("mbed-stress-test.txt", index, buffer->ptr, buffer->size_max);
            buffer->size = read;

            TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(buffer->ptr,
                                                 &story[index],
                                                 buffer->size,
                                                 "character mismatch");

            index += buffer->size;
            printf("download: %u\r\n", index);

            /* put data on consumer queue */
            queue.put(buffer);
        }
        else
        {
            TEST_ASSERT_MESSAGE(false, "queue failed");
        }
    }
    TEST_ASSERT_EQUAL_UINT_MESSAGE(index, sizeof(story), "wrong length");
}

static void test_buffer(size_t size)
{
    printf("\r\nTest buffer: %u\r\n", size);

    /*************************************************************************/
    buffer0.size_max = size;
    buffer0.size = 0;
    buffer0.ptr = (unsigned char*) malloc(size);
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer0.ptr, "memory allocation failed");

    buffer1.size_max = size;
    buffer1.size = 0;
    buffer1.ptr = (unsigned char*) malloc(size);
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer1.ptr, "memory allocation failed");

    osStatus status;
    status = mpool.put(&buffer0);
    TEST_ASSERT_EQUAL_MESSAGE(osOK, status, "failed to put");

    status = mpool.put(&buffer1);
    TEST_ASSERT_EQUAL_MESSAGE(osOK, status, "failed to put");

    /*************************************************************************/

    printf("start file thread\r\n");

    Thread thread;
    thread.start(file_thread);

    /*************************************************************************/

    printf("write to flash\r\n");

    mbed_stress_test_erase_flash();

    size_t index = 0;
    while (index < sizeof(story))
    {
        osEvent event = queue.get(100);
        if (event.status == osEventMessage)
        {
            my_buffer_t* buffer = (my_buffer_t*) event.value.p;

            mbed_stress_test_write_flash(MBED_CONF_APP_ESTIMATED_APPLICATION_SIZE + index, buffer->ptr, buffer->size);

            index += buffer->size;

            mpool.put(buffer);
        }
        else
        {
            TEST_ASSERT_EQUAL_MESSAGE(osEventTimeout, event.status, "queue failed");

            Thread::State state = thread.get_state();
            TEST_ASSERT_NOT_EQUAL_MESSAGE(Thread::Deleted, state, "download complete but still missing data");
        }
    }

    /*************************************************************************/
    printf("\r\nwrite complete - read back\r\n");

    mbed_stress_test_compare_flash(MBED_CONF_APP_ESTIMATED_APPLICATION_SIZE, story, sizeof(story));

    /*************************************************************************/
    mpool.get();
    mpool.get();
    free(buffer0.ptr);
    free(buffer1.ptr);
}

static control_t test_setup(const size_t call_count)
{
    setup();

    return CaseNext;
}

static control_t test_buffer_1k(const size_t call_count)
{
    test_buffer(1*1024);

    return CaseNext;
}

static control_t test_buffer_2k(const size_t call_count)
{
    test_buffer(2*1024);

    return CaseNext;
}

static control_t test_buffer_4k(const size_t call_count)
{
    test_buffer(4*1024);

    return CaseNext;
}

static control_t test_buffer_8k(const size_t call_count)
{
    test_buffer(8*1024);

    return CaseNext;
}

static control_t test_buffer_16k(const size_t call_count)
{
    test_buffer(16*1024);

    return CaseNext;
}

static control_t test_buffer_32k(const size_t call_count)
{
    test_buffer(32*1024);

    return CaseNext;
}

Case cases[] = {
    Case("Setup", test_setup),
    Case("Buffer  1k", test_buffer_1k),
    Case("Buffer  2k", test_buffer_2k),
    Case("Buffer  4k", test_buffer_4k),
    Case("Buffer  8k", test_buffer_8k),
//    Case("Buffer 16k", test_buffer_16k),
//    Case("Buffer 32k", test_buffer_32k),
};

utest::v1::status_t greentea_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(10*60, "default_auto");

    return greentea_test_setup_handler(number_of_cases);
}

Specification specification(greentea_setup, cases);

int main()
{
    return !Harness::run(specification);
}
