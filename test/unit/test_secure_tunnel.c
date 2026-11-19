/*
 * Unit tests for secure_tunnel
 *
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Mocksubscriptions.h"
#include "secure-tunnel.h"
#include <gg/backoff.h>
#include <gg/error.h>
#include <unity.h>
#include <stddef.h>
#include <stdint.h>

// Prototypes are required by -Wmissing-prototypes, as in the other unit tests.
void test_run_secure_tunnel_success(void);
void test_retry_backs_off_between_5s_and_60s(void);
void test_retry_recovers_after_transient_timeouts(void);
void test_retry_keeps_going_while_nucleus_is_offline(void);
void test_retry_does_not_retry_config_errors(void);
void test_retry_does_not_retry_invalid_thing_name(void);

// Number of consecutive failures the fake backoff tolerates before giving up,
// so that a bug in the retry loop fails the test instead of hanging it.
#define MAX_FAKE_RETRIES 16

// How many times the nucleus reports no connection before the subscription
// goes through in test_retry_keeps_going_while_nucleus_is_offline.
#define OFFLINE_ATTEMPTS 6

static uint32_t backoff_base_ms;
static uint32_t backoff_max_ms;
static uint32_t backoff_max_attempts;
static size_t backoff_sleep_count;

// Stands in for the SDK's gg_backoff(): same retry contract, but it records
// the parameters and counts the sleeps instead of sleeping. The test binary's
// definition takes precedence over the one in the gg-sdk library.
GgError gg_backoff(
    uint32_t base_ms,
    uint32_t max_ms,
    uint32_t max_attempts,
    GgError (*fn)(void *ctx),
    void *ctx
) {
    backoff_base_ms = base_ms;
    backoff_max_ms = max_ms;
    backoff_max_attempts = max_attempts;

    for (uint32_t attempt = 1;; attempt++) {
        GgError ret = fn(ctx);
        if (ret == GG_ERR_OK) {
            return GG_ERR_OK;
        }
        if ((max_attempts != 0) && (attempt >= max_attempts)) {
            return ret;
        }
        TEST_ASSERT_TRUE_MESSAGE(
            backoff_sleep_count < MAX_FAKE_RETRIES, "too many retries"
        );
        backoff_sleep_count++;
    }
}

static SecureTunnelConfig test_config(void) {
    return (SecureTunnelConfig) { .thing_name = GG_STR("test-thing"),
                                  .region = GG_STR("us-west-2"),
                                  .artifact_path = GG_STR("/opt/localproxy"),
                                  .max_concurrent_tunnels = 5,
                                  .tunnel_timeout_seconds = 3600 };
}

void setUp(void) {
    Mocksubscriptions_Init();
    backoff_base_ms = 0;
    backoff_max_ms = 0;
    backoff_max_attempts = 0;
    backoff_sleep_count = 0;
}

void tearDown(void) {
    Mocksubscriptions_Verify();
    Mocksubscriptions_Destroy();
}

void test_run_secure_tunnel_success(void) {
    SecureTunnelConfig config = test_config();

    subscribe_to_aws_tunnel_tokens_ExpectAndReturn(&config, GG_ERR_OK);

    GgError ret = run_secure_tunnel(&config);

    TEST_ASSERT_EQUAL(GG_ERR_OK, ret);
    TEST_ASSERT_EQUAL_size_t(0, backoff_sleep_count);
}

void test_retry_backs_off_between_5s_and_60s(void) {
    SecureTunnelConfig config = test_config();

    subscribe_to_aws_tunnel_tokens_ExpectAndReturn(&config, GG_ERR_OK);

    (void) run_secure_tunnel(&config);

    TEST_ASSERT_EQUAL_UINT32(5000, backoff_base_ms);
    TEST_ASSERT_EQUAL_UINT32(60000, backoff_max_ms);
    // 0 means gg_backoff() retries indefinitely.
    TEST_ASSERT_EQUAL_UINT32(0, backoff_max_attempts);
}

// The nucleus is not connected to AWS IoT Core yet (e.g. right after boot), so
// the IPC subscribe times out twice before it succeeds.
void test_retry_recovers_after_transient_timeouts(void) {
    SecureTunnelConfig config = test_config();

    subscribe_to_aws_tunnel_tokens_ExpectAndReturn(&config, GG_ERR_TIMEOUT);
    subscribe_to_aws_tunnel_tokens_ExpectAndReturn(&config, GG_ERR_TIMEOUT);
    subscribe_to_aws_tunnel_tokens_ExpectAndReturn(&config, GG_ERR_OK);

    GgError ret = run_secure_tunnel(&config);

    TEST_ASSERT_EQUAL(GG_ERR_OK, ret);
    TEST_ASSERT_EQUAL_size_t(2, backoff_sleep_count);
}

void test_retry_keeps_going_while_nucleus_is_offline(void) {
    SecureTunnelConfig config = test_config();

    for (int i = 0; i < OFFLINE_ATTEMPTS; i++) {
        subscribe_to_aws_tunnel_tokens_ExpectAndReturn(&config, GG_ERR_NOCONN);
    }
    subscribe_to_aws_tunnel_tokens_ExpectAndReturn(&config, GG_ERR_OK);

    GgError ret = run_secure_tunnel(&config);

    TEST_ASSERT_EQUAL(GG_ERR_OK, ret);
    TEST_ASSERT_EQUAL_size_t(OFFLINE_ATTEMPTS, backoff_sleep_count);
}

// Missing SVCUID / socket path: retrying cannot help, so fail fast and let the
// nucleus report the component as errored.
void test_retry_does_not_retry_config_errors(void) {
    SecureTunnelConfig config = test_config();

    subscribe_to_aws_tunnel_tokens_ExpectAndReturn(&config, GG_ERR_CONFIG);

    GgError ret = run_secure_tunnel(&config);

    TEST_ASSERT_EQUAL(GG_ERR_FAILURE, ret);
    TEST_ASSERT_EQUAL_size_t(0, backoff_sleep_count);
}

void test_retry_does_not_retry_invalid_thing_name(void) {
    SecureTunnelConfig config = test_config();

    subscribe_to_aws_tunnel_tokens_ExpectAndReturn(&config, GG_ERR_INVALID);

    GgError ret = run_secure_tunnel(&config);

    TEST_ASSERT_EQUAL(GG_ERR_FAILURE, ret);
    TEST_ASSERT_EQUAL_size_t(0, backoff_sleep_count);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_run_secure_tunnel_success);
    RUN_TEST(test_retry_backs_off_between_5s_and_60s);
    RUN_TEST(test_retry_recovers_after_transient_timeouts);
    RUN_TEST(test_retry_keeps_going_while_nucleus_is_offline);
    RUN_TEST(test_retry_does_not_retry_config_errors);
    RUN_TEST(test_retry_does_not_retry_invalid_thing_name);

    return UNITY_END();
}
