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

#include "mbed.h"
#include "unity/unity.h"
#include <inttypes.h>

#include "tls_socket.h"
#include "certificate_aws_s3.h"

static volatile bool event_fired = false;
#define BUFFER_SIZE 1024
#define MAX_RETRIES 3

const char request_template[] = 
    "GET /firmware/%s.txt HTTP/1.1\n"
    "Host: lootbox.s3.dualstack.us-west-2.amazonaws.com\n"
    "Range: bytes=%d-%d\n"
    "\n";

static void socket_event(void)
{
    event_fired = true;
}

size_t mbed_stress_test_download(NetworkInterface* interface, const char* filename, size_t offset, char* data, size_t data_length, bool tls)
{
    int result = -1;

    TCPSocket* tcpsocket = NULL;
    TLSSocket* tlssocket = NULL;

    if (tls)
    {
        /* setup TLS socket */
        tlssocket = new TLSSocket(interface, "lootbox.s3.dualstack.us-west-2.amazonaws.com", 443, SSL_CA_PEM);
        TEST_ASSERT_NOT_NULL_MESSAGE(tlssocket, "failed to instantiate tlssocket");

        tlssocket->set_debug(true);
        printf("debug mode set\r\n");

        for (int tries = 0; tries < MAX_RETRIES; tries++) {
            result = tlssocket->connect();
            if (result == 0) {
                break;
            }
            printf("connection failed. retry %d of %d\r\n", tries, MAX_RETRIES);
        }
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "failed to connect");

        tcpsocket = tlssocket->get_tcp_socket();
        TEST_ASSERT_NOT_NULL_MESSAGE(tcpsocket, "failed to get underlying TCPSocket");        
    }
    else
    {
        /* setup TCP socket */
        tcpsocket = new TCPSocket(interface);
        TEST_ASSERT_NOT_NULL_MESSAGE(tcpsocket, "failed to create TCPSocket");        

        for (int tries = 0; tries < MAX_RETRIES; tries++) {
            result = tcpsocket->connect("lootbox.s3.dualstack.us-west-2.amazonaws.com", 80);
            if (result == 0) {
                break;
            }
            printf("connection failed. retry %d of %d\r\n", tries, MAX_RETRIES);
        }
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "failed to connect");
    }

    tcpsocket->set_blocking(false);
    printf("non-blocking mode set\r\n");

    tcpsocket->sigio(socket_event);
    printf("registered callback function\r\n");


    /* setup request */
    char* request = new char[BUFFER_SIZE];

    size_t request_size = snprintf(request, BUFFER_SIZE, request_template, filename, offset, offset + data_length - 1);
    TEST_ASSERT_MESSAGE(request_size < BUFFER_SIZE, "request buffer overflow");

    printf("request: %s[end]\r\n", request);


    /* send request to server */
    if (tls)
    {
        result = mbedtls_ssl_write(tlssocket->get_ssl_context(), (const unsigned char*) request, request_size);
        TEST_ASSERT_MESSAGE(result != MBEDTLS_ERR_SSL_WANT_READ, "failed to write ssl");
        TEST_ASSERT_MESSAGE(result != MBEDTLS_ERR_SSL_WANT_WRITE, "failed to write ssl");
        TEST_ASSERT_EQUAL_INT_MESSAGE(request_size, result, "failed to write ssl");        
    }
    else
    {
        result = tcpsocket->send(request, request_size);
        TEST_ASSERT_EQUAL_INT_MESSAGE(request_size, result, "failed to write ssl");        
    }


    /* read response */
    size_t expected_bytes = data_length;
    size_t received_bytes = 0;
    uint32_t body_index = 0;


    /* loop until all expected bytes have been received */
    while (received_bytes < expected_bytes)
    {
        /* wait for async event */
        while(!event_fired)
        {
            wait(1);
        }
        event_fired = false;

        /* loop until all data has been read from socket */
        do
        {
            if (tls)
            {
                result = mbedtls_ssl_read(tlssocket->get_ssl_context(), (unsigned char*) &data[received_bytes], data_length - received_bytes);
                TEST_ASSERT_MESSAGE((result == MBEDTLS_ERR_SSL_WANT_READ) || (result >= 0), "failed to read ssl");                
            }
            else
            {
                result = tcpsocket->recv(&data[received_bytes], data_length - received_bytes);
                TEST_ASSERT_MESSAGE((result == NSAPI_ERROR_WOULD_BLOCK) || (result >= 0), "failed to read socket");
            }
//            printf("result: %d\r\n", result);

            if (result > 0)
            {
                /* skip HTTP header */
                if (body_index == 0)
                {
                    std::string header(data, result);
                    body_index = header.find("\r\n\r\n");
                    TEST_ASSERT_MESSAGE(body_index != std::string::npos, "failed to find body");

                    /* remove header */
                    memmove(data, &data[body_index + 4], result - body_index - 4);

                    received_bytes += (result - body_index - 4);
                }
                else
                {
                    received_bytes += result;
                }

//                data[result] = '\0';
//                printf("%s", data);
                printf("received_bytes: %u\r\n", received_bytes);
            }
        }
        while ((result > 0) && (received_bytes < expected_bytes));

        if (result == MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            printf("MBEDTLS_ERR_SSL_WANT_WRITE: %d\r\n", MBEDTLS_ERR_SSL_WANT_WRITE);
            break;
        }
    }

    delete[] request;

    if (tls)
    {
        delete tlssocket;
    }
    else
    {
        delete tcpsocket;
    }

    printf("done\r\n");

    return received_bytes;
}
