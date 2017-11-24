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

using namespace utest::v1;

#if DEVICE_FLASH

#include "mbed_stress_test_flash.h"

#include MBED_CONF_APP_PROTAGONIST_FLASH

void flash_test(void)
{
    FlashIAP flash;

    uint32_t page_size = flash.get_page_size();
    uint32_t flash_size = flash.get_flash_size();

    mbed_stress_test_erase_flash();

    printf("fill flash with multiple stories\r\n");

    for (size_t offset = MBED_CONF_APP_ESTIMATED_APPLICATION_SIZE; offset < flash_size; )
    {
        printf("write new story\r\n");

        size_t write_size = sizeof(story);

        if (write_size > (flash_size - offset))
        {
            write_size = flash_size - offset;
        }

        /* round down to page size boundary */
        write_size = (write_size / page_size) * page_size;

        if (write_size > 0)
        {
            mbed_stress_test_write_flash(offset, story, write_size);
            offset += write_size;
        }
        else
        {
            break;
        }

    }

    printf("read stories\r\n");

    for (size_t offset = MBED_CONF_APP_ESTIMATED_APPLICATION_SIZE; offset < flash_size; )
    {
        printf("read story\r\n");

        size_t read_size = sizeof(story);

        if (read_size > (flash_size - offset))
        {
            read_size = flash_size - offset;
        }

        /* round down to page size boundary */
        read_size = (read_size / page_size) * page_size;

        if (read_size > 0)
        {
            mbed_stress_test_compare_flash(offset, story, read_size);
            offset += read_size;
        }
        else
        {
            break;
        }
    }
}

Case cases[] = {
    Case("Flash test", flash_test),
};

#else
#warning FlashIAP test not supported on this target

void dummy(void)
{
}

Case cases[] = {
    Case("Dummy test", dummy),
};

#endif /* DEVICE_FLASH */

utest::v1::status_t greentea_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(5*60, "default_auto");
    return greentea_test_setup_handler(number_of_cases);
}

Specification specification(greentea_setup, cases, greentea_test_teardown_handler);

int main()
{
    return !Harness::run(specification);
}
