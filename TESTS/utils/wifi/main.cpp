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

#define WIFI 2
#if !(MBED_CONF_TARGET_NETWORK_DEFAULT_INTERFACE_TYPE == WIFI)
#error [NOT_SUPPORTED] test not supported
#endif

using namespace utest::v1;

static control_t scan(const size_t call_count)
{
    WiFiInterface* interface = WiFiInterface::get_default_instance();
    TEST_ASSERT_NOT_NULL_MESSAGE(interface, "failed to instantiate WiFi");

    printf("WiFi MAC: %s\r\n", interface->get_mac_address());

    WiFiAccessPoint* aps = new WiFiAccessPoint[10];
    TEST_ASSERT_NOT_NULL_MESSAGE(aps, "failed to allocate access point array");

    int count = interface->scan(aps, 10);

    if (count > 0)
    {
        for (size_t index = 0; index < count; index++)
        {
            printf("%d: ", index);

            printf("%d %d %s\r\n", aps[index].get_channel(), aps[index].get_rssi(), aps[index].get_ssid());
        }
    }
    else
    {
        printf("No access points in range\r\n");
    }

#if 0
    const char *get_ssid() const;

    /** Get an access point's bssid
     *
     *  @return The bssid of the access point
     */
    const uint8_t *get_bssid() const;

    /** Get an access point's security
     *
     *  @return The security type of the access point
     */
    nsapi_security_t get_security() const;

    /** Gets the radio signal strength for the access point
     *
     *  @return         Connection strength in dBm (negative value),
     *                  or 0 if measurement impossible
     */
    int8_t get_rssi() const;

    /** Get the access point's channel
     *
     *  @return The channel of the access point
     */
    uint8_t get_channel() const;
#endif

    delete[] aps;

    return CaseNext;
}

utest::v1::status_t greentea_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(30*60, "default_auto");
    return greentea_test_setup_handler(number_of_cases);
}

Case cases[] = {
    Case("Scan", scan),
};

Specification specification(greentea_setup, cases);

int main()
{
    return !Harness::run(specification);
}
