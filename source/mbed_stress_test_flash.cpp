/* mbed Microcontroller Library
 * Copyright (c) 2016 ARM Limited
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

#if DEVICE_FLASH

#include "mbed.h"
#include "unity/unity.h"
#include <inttypes.h>

FlashIAP flash;

void mbed_stress_test_erase_flash(void)
{
    int result;

    printf("Initialzie FlashIAP\r\n");

    result = flash.init();
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "failed to initialize FlashIAP");

    uint32_t flash_start = flash.get_flash_start();
    printf("start address: %" PRIX32 "\r\n", flash_start);
    TEST_ASSERT_MESSAGE(MBED_FLASH_INVALID_SIZE != flash_start, "invalid start address");

    uint32_t flash_size = flash.get_flash_size();
    printf("flash size: %" PRIX32 "\r\n", flash_size);
    TEST_ASSERT_MESSAGE(MBED_FLASH_INVALID_SIZE != flash_size, "invalid flash size");

    uint32_t page_size = flash.get_page_size();
    printf("page size: %" PRIu32 "\r\n", page_size);
    TEST_ASSERT_MESSAGE(MBED_FLASH_INVALID_SIZE != page_size, "invalid page size");

    uint32_t last_sector_size = flash.get_sector_size(flash_start + flash_size - 1);
    printf("last sector size: %" PRIu32 "\r\n", last_sector_size);
    TEST_ASSERT_MESSAGE(MBED_FLASH_INVALID_SIZE != last_sector_size, "invalid sector size");

    uint32_t start_address = flash_start + MBED_CONF_APP_ESTIMATED_APPLICATION_SIZE;
    uint32_t erase_size = flash_size - MBED_CONF_APP_ESTIMATED_APPLICATION_SIZE;
    printf("Erase flash: %" PRIX32 " %" PRIX32 "\r\n", start_address, erase_size);

    result = flash.erase(start_address, erase_size);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "failed to erase flash");
}

void mbed_stress_test_write_flash(size_t offset, const unsigned char* buffer, size_t buffer_length)
{
    uint32_t flash_start = flash.get_flash_start();
    uint32_t flash_size = flash.get_flash_size();
    uint32_t page_size = flash.get_page_size();

    uint32_t start_address = flash_start + offset;

    printf("program: %" PRIX32 " %u\r\n", start_address, buffer_length);

    size_t index = 0;
    while (index < buffer_length)
    {
        TEST_ASSERT_MESSAGE(start_address + index + page_size <= flash_size, "address and data out of bounds");

        int result = flash.program(&buffer[index], start_address + index, page_size);

        if (result != 0)
        {
            printf("program: %" PRIX32 " %" PRIu32 "\r\n", start_address + index, page_size);

        }
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "failed to program flash");

        index += page_size;
    }
}

void mbed_stress_test_compare_flash(size_t offset, const unsigned char* data, size_t data_length)
{
    uint32_t flash_start = flash.get_flash_start();
    uint32_t flash_size = flash.get_flash_size();
    uint32_t page_size = flash.get_page_size();

    size_t start_address = flash_start + offset;

    char* buffer = (char*) malloc(page_size);
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "could not allocate buffer");

    size_t index = 0;
    while (index < data_length)
    {
        uint32_t read_length = data_length - index;

        if (read_length > page_size)
        {
            read_length = page_size;
        }

        if ((page_size >= 1024) || (index % (page_size * 0x1000) == 0))
        {
            printf("read: %u %" PRIu32 "\r\n", start_address + index, page_size);
        }

        int result = flash.read(buffer, start_address + index, read_length);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "failed to read flash");
        TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(buffer, &data[index], read_length, "character mismatch");

        index += read_length;
    }
    TEST_ASSERT_EQUAL_UINT_MESSAGE(index, data_length, "wrong length");

    free(buffer);
}

#endif /* DEVICE_FLASH */
