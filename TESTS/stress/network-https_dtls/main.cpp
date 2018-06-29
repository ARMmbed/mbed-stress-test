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
#include "dtls_socket.h"

using namespace utest::v1;

#include MBED_CONF_APP_PROTAGONIST_DOWNLOAD
#include "sertti_der.h"

#include "mbedtls/timing.h"

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
    // Create a queue that can hold a maximum of 32 events
    EventQueue queue(32 * EVENTS_EVENT_SIZE);
    // Create a thread that'll run the event queue's dispatch function
    Thread t;
    
    // Start the event queue's dispatch thread
    t.start(callback(&queue, &EventQueue::dispatch_forever));    
    
    int result = -1;

    /* setup TLS socket */
    DTLSSocket* dtlssocket = new DTLSSocket(interface, 
                                            /*"lootbox.s3.dualstack.us-west-2.amazonaws.com"*/"192.168.100.5", 
                                            /*443*/4430, 
                                            (const char*)SSL_CA_PEM,
                                            sizeof(SSL_CA_PEM));
    
    TEST_ASSERT_NOT_NULL_MESSAGE(dtlssocket, "failed to instantiate dtlssocket");

    dtlssocket->set_debug(true);
    printf("debug mode set\r\n");

    timing_context_t timing_context    = {0};
    timing_context.event_q             = &queue;
    timing_context.is_handshake_success = false;
    for (int tries = 0; tries < MAX_RETRIES; tries++) {
        result = dtlssocket->connect(&timing_context);
        if ((result == 0) || (result == MBEDTLS_ERR_SSL_WANT_READ) || (result == MBEDTLS_ERR_SSL_WANT_WRITE)) {
            break;
        }
        printf("connection failed. retry %d of %d\r\n", tries, MAX_RETRIES);
    }
    
//    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "failed to connect");

    if ((result == MBEDTLS_ERR_SSL_WANT_READ) || (result == MBEDTLS_ERR_SSL_WANT_WRITE)) {
        printf("Wait for successfull handshake prior proceeding, reason %d.\r\n", result);
        while (!timing_context.is_handshake_success) {            
        };
    }

    UDPSocket* udpsocket = dtlssocket->get_udp_socket();
    TEST_ASSERT_NOT_NULL_MESSAGE(dtlssocket, "failed to get underlying UDPSocket");

    udpsocket->set_blocking(false);
    printf("non-blocking mode set\r\n");

    udpsocket->sigio(socket_event);
    printf("registered callback function\r\n");


    /* setup request */
    /* -1 to remove h from .h in header file name */
    size_t request_size = strlen(part1) + strlen(filename) - 1 + strlen(part2)/* + 1*/;
    char *request = new char[request_size]();

    /* construct request */
    memcpy(&request[0], part1, strlen(part1));
    memcpy(&request[strlen(part1)], filename, strlen(filename) - 1);
    memcpy(&request[strlen(part1) + strlen(filename) - 1], part2, strlen(part2));

    printf("request: %s[end]\r\n", request);


    /* send request to server */
    result = mbedtls_ssl_write(dtlssocket->get_ssl_context(), (const unsigned char*) request, request_size);
    TEST_ASSERT_MESSAGE(result != MBEDTLS_ERR_SSL_WANT_READ, "failed to write ssl");
    TEST_ASSERT_MESSAGE(result != MBEDTLS_ERR_SSL_WANT_WRITE, "failed to write ssl");
    TEST_ASSERT_EQUAL_INT_MESSAGE(request_size, result, "failed to write ssl");


    /* read response */
    char* receive_buffer = new char[size];
    TEST_ASSERT_NOT_NULL_MESSAGE(receive_buffer, "failed to allocate receive buffer");

    size_t expected_bytes = sizeof(story);
    
    printf("expected_bytes: %d\r\n", expected_bytes);
    
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
//            printf("mbedtls_ssl_read: \r\n");
            
            result = mbedtls_ssl_read(dtlssocket->get_ssl_context(), (unsigned char*) receive_buffer, size/* - 1*/);
            TEST_ASSERT_MESSAGE((result == MBEDTLS_ERR_SSL_WANT_READ) || (result >= 0), "failed to read ssl");

//            printf("+result: %d\r\n", result);

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
//                printf("received_bytes: %u\r\n", received_bytes);
            }
        }
        while ((result > 0) && (received_bytes < expected_bytes));

        if (result == MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            printf("MBEDTLS_ERR_SSL_WANT_WRITE: %d\r\n", MBEDTLS_ERR_SSL_WANT_WRITE);
            break;
        }
    }

    delete request;
    delete dtlssocket;
    delete[] receive_buffer;

    printf("done\r\n");
}

static control_t setup_network(const size_t call_count)
{
    interface = easy_connect(true);
    TEST_ASSERT_NOT_NULL_MESSAGE(interface, "failed to initialize network");

    return CaseNext;
}

static control_t download_4k(const size_t call_count)
{
    download(4*1024);

    return CaseNext;
}

utest::v1::status_t greentea_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(30*60, "default_auto");
    return greentea_test_setup_handler(number_of_cases);
}

Case cases[] = {
    Case("Setup network", setup_network),
    Case("Download  4k", download_4k),
};

Specification specification(greentea_setup, cases);

int main()
{
    return !Harness::run(specification);
}

int ssl_handshake_continue(timing_context_t *cntx)
{    
    int ret = ~MBEDTLS_ERR_SSL_WANT_READ;

    while (ret != MBEDTLS_ERR_SSL_WANT_READ) {
        printf("mbedtls_ssl_handshake_step\r\n");                
        
        ret = mbedtls_ssl_handshake_step(cntx->ssl);
        printf("SSL state: %d\r\n", cntx->ssl->state);
        if (MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED == ret) {
            mbedtls_ssl_session_reset(cntx->ssl);
#if 0            
#if defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED)
            if (mbedtls_ssl_set_hs_ecjpake_password(cntx->ssl, sec->_pw, sec->_pw_len) != 0) {
                return -1;
            }
#endif
#endif // 0
            return 1;
        } else if (ret && (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)) {
            return ret;
        }

        if (cntx->ssl->state == MBEDTLS_SSL_HANDSHAKE_OVER) {
            return 0;
        }
    }

    if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
        return 1;
    }

    return -1;
}
    
void on_timeout(timing_context_t *cntx)
{
    printf("!!on_timeout\r\n");
   
    if (cntx->fin_ms > cntx->int_ms) {
        /* Intermediate expiry. */
        printf("!!Intermediate expiry\r\n");
        
        cntx->is_intermediate_expire = true;
        cntx->fin_ms                -= cntx->int_ms;
        
        cntx->timer_id = cntx->event_q->call_in(cntx->int_ms, on_timeout, cntx);
        MBED_ASSERT(cntx->timer_id != 0);
    } else {    
        /* Final expiry. */        
        printf("!!Final expiry\r\n");        
        
        cntx->is_final_expire = true;        
        const int err         = ssl_handshake_continue(cntx);
        if (MBEDTLS_ERR_SSL_TIMEOUT == err) {
            printf("MBEDTLS_ERR_SSL_TIMEOUT\r\n");        
            
            mbedtls_ssl_session_reset(cntx->ssl);
        }
    }
}

void mbedtls_timing_set_delay( void *data, uint32_t int_ms, uint32_t fin_ms)
{
#if 0    
    printf("!!mbedtls_timing_set_delay\r\n");
    printf("int_ms: %u\r\n", (unsigned int)int_ms);
    printf("fin_ms: %u\r\n", (unsigned int)fin_ms);
#endif     
    timing_context_t *cntx = (timing_context_t *)data;
    MBED_ASSERT(cntx != NULL);
//    MBED_ASSERT(int_ms != 0);
//    MBED_ASSERT(fin_ms != 0);    
    
    if ((int_ms > 0) && (fin_ms > 0)) {
        cntx->int_ms = int_ms;
        cntx->fin_ms = fin_ms;
        
        MBED_ASSERT(cntx->timer_id == 0);
        
        printf("!!start timer\r\n");
        cntx->timer_id = cntx->event_q->call_in(cntx->int_ms, on_timeout, cntx);
        MBED_ASSERT(cntx->timer_id != 0);
    } else if (fin_ms == 0) {
//        printf("!!clear timer\r\n");
        cntx->int_ms   = 0;
        cntx->fin_ms   = 0;
        
        if (cntx->timer_id != 0) {      
            cntx->event_q->cancel(cntx->timer_id);
            cntx->timer_id = 0;
        }
    } else {
        /* No implementation required. */
    }

    
    
    
#if 0    
    if (cntx->timer_id != 0) {
        cntx->event_q->cancel(cntx->timer_id);
    }
#endif    
        
#if 0
sec->timer.int_ms = int_ms;
            sec->timer.fin_ms = fin_ms;
            sec->timer.state = TIMER_STATE_NO_EXPIRY;
            if(sec->timer.timer){
                eventOS_timeout_cancel(sec->timer.timer);
            }
            sec->timer.timer = eventOS_timeout_ms(timer_cb, int_ms, sec);
        } else if (fin_ms == 0) {
            /* fin_ms == 0 means cancel the timer */
            sec->timer.state = TIMER_STATE_CANCELLED;
            eventOS_timeout_cancel(sec->timer.timer);
            sec->timer.fin_ms = 0;
            sec->timer.int_ms = 0;
            sec->timer.timer = NULL;
#endif             
#if 0 // original implementation from timing.c     
    printf("!!mbedtls_timing_set_delay \r\n");
    
    mbedtls_timing_delay_context *ctx = (mbedtls_timing_delay_context *) data;

    ctx->int_ms = int_ms;
    ctx->fin_ms = fin_ms;

    if( fin_ms != 0 )
        (void) mbedtls_timing_get_timer( &ctx->timer, 1 );    
#endif // 0    
}

/**
 * \brief          Get the status of delays
 *                 (Memory helper: number of delays passed.)
 *
 * \param data     Pointer to timing data
 *                 Must point to a valid \c mbedtls_timing_delay_context struct.
 *
 * \return          -1 if cancelled (fin_ms = 0)
 *                  0 if none of the delays are passed,
 *                  1 if only the intermediate delay is passed,
 *                  2 if the final delay is passed.
 */
int mbedtls_timing_get_delay(void *data)
{
    timing_context_t *cntx = (timing_context_t *)data;
    
    if (cntx->is_final_expire) {
        /* Final delay is passed. */
//        printf("!!mbedtls_timing_get_delay: Final delay is passed\r\n");
        
        return 2;
    }
    if (cntx->timer_id == 0){
        /* Cancelled. */
//        printf("!!mbedtls_timing_get_delay: Cancelled\r\n");        
    
        return -1;
    }
    if (cntx->is_intermediate_expire) {
        /* Only the intermediate delay is passed. */
//        printf("!!mbedtls_timing_get_delay: Only the intermediate delay is passed\r\n");                
        
        return 1;
    }
    
    printf("!!mbedtls_timing_get_delay: none of the delays are passed\r\n");                    
    return 0;
    
    
    
//    MBED_ASSERT(false);
    
    return 0;    
#if 0 // original implementation from timing.c         
    printf("!!mbedtls_timing_get_delay \r\n");
    
    mbedtls_timing_delay_context *ctx = (mbedtls_timing_delay_context *) data;
    unsigned long elapsed_ms;

    if( ctx->fin_ms == 0 )
        return( -1 );

    elapsed_ms = mbedtls_timing_get_timer( &ctx->timer, 0 );

    if( elapsed_ms >= ctx->fin_ms )
        return( 2 );

    if( elapsed_ms >= ctx->int_ms )
        return( 1 );

    return( 0 );
#endif // 0    
}
