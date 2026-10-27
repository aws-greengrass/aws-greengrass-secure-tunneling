/*
 * Unit tests for secure_tunnel
 *
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Mocksubscriptions.h"
#include "secure-tunnel.h"
#include <stddef.h>
#include <stdint.h>
#include <unity.h>

void test_run_secure_tunnel_success(void);
void test_retry_succeeds_on_first_attempt_without_sleeping(void);
void test_retry_recovers_after_transient_timeouts(void);
void test_retry_backoff_is_capped(void);
void test_retry_gives_up_after_max_attempts(void);
void test_retry_does_not_retry_config_errors(void);
void test_retry_does_not_retry_invalid_thing_name(void);

#define MAX_RECORDED_SLEEPS 16

static int64_t recorded_sleeps[MAX_RECORDED_SLEEPS];
static size_t recorded_sleep_count;

// Backoff starts at 5 s, doubles, and is capped at 60 s.
static const int EXPECTED_DELAYS_S[] = { 5, 10, 20, 40, 60, 60, 60 };

static void assert_recorded_delays(size_t count) {
    TEST_ASSERT_EQUAL_size_t(count, recorded_sleep_count);
    for (size_t i = 0; i < count; i++) {
        TEST_ASSERT_EQUAL_INT(EXPECTED_DELAYS_S[i], (int) recorded_sleeps[i]);
    }
}

static GgError fake_sleep(int64_t seconds) {
    TEST_ASSERT_TRUE_MESSAGE(
        recorded_sleep_count < MAX_RECORDED_SLEEPS, "too many sleeps"
    );
    recorded_sleeps[recorded_sleep_count++] = seconds;
    return GG_ERR_OK;
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
    recorded_sleep_count = 0;
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
}

void test_retry_succeeds_on_first_attempt_without_sleeping(void) {
    SecureTunnelConfig config = test_config();

    subscribe_to_aws_tunnel_tokens_ExpectAndReturn(&config, GG_ERR_OK);

    GgError ret = run_secure_tunnel_with_retry(&config, 0, fake_sleep);

    TEST_ASSERT_EQUAL(GG_ERR_OK, ret);
    TEST_ASSERT_EQUAL_size_t(0, recorded_sleep_count);
}

// The nucleus is not connected to AWS IoT Core yet (e.g. right after boot), so
// the IPC subscribe times out twice before it succeeds.
void test_retry_recovers_after_transient_timeouts(void) {
    SecureTunnelConfig config = test_config();

    subscribe_to_aws_tunnel_tokens_ExpectAndReturn(&config, GG_ERR_TIMEOUT);
    subscribe_to_aws_tunnel_tokens_ExpectAndReturn(&config, GG_ERR_TIMEOUT);
    subscribe_to_aws_tunnel_tokens_ExpectAndReturn(&config, GG_ERR_OK);

    GgError ret = run_secure_tunnel_with_retry(&config, 0, fake_sleep);

    TEST_ASSERT_EQUAL(GG_ERR_OK, ret);
    assert_recorded_delays(2);
}

void test_retry_backoff_is_capped(void) {
    SecureTunnelConfig config = test_config();

    for (int i = 0; i < 7; i++) {
        subscribe_to_aws_tunnel_tokens_ExpectAndReturn(
            &config, GG_ERR_NOCONN
        );
    }
    subscribe_to_aws_tunnel_tokens_ExpectAndReturn(&config, GG_ERR_OK);

    GgError ret = run_secure_tunnel_with_retry(&config, 0, fake_sleep);

    TEST_ASSERT_EQUAL(GG_ERR_OK, ret);
    assert_recorded_delays(7);
}

void test_retry_gives_up_after_max_attempts(void) {
    SecureTunnelConfig config = test_config();

    subscribe_to_aws_tunnel_tokens_ExpectAndReturn(&config, GG_ERR_TIMEOUT);
    subscribe_to_aws_tunnel_tokens_ExpectAndReturn(&config, GG_ERR_TIMEOUT);
    subscribe_to_aws_tunnel_tokens_ExpectAndReturn(&config, GG_ERR_TIMEOUT);

    GgError ret = run_secure_tunnel_with_retry(&config, 3, fake_sleep);

    TEST_ASSERT_EQUAL(GG_ERR_FAILURE, ret);
    assert_recorded_delays(2);
}

// Missing SVCUID / socket path: retrying cannot help, so fail fast and let the
// nucleus report the component as errored.
void test_retry_does_not_retry_config_errors(void) {
    SecureTunnelConfig config = test_config();

    subscribe_to_aws_tunnel_tokens_ExpectAndReturn(&config, GG_ERR_CONFIG);

    GgError ret = run_secure_tunnel_with_retry(&config, 0, fake_sleep);

    TEST_ASSERT_EQUAL(GG_ERR_FAILURE, ret);
    TEST_ASSERT_EQUAL_size_t(0, recorded_sleep_count);
}

void test_retry_does_not_retry_invalid_thing_name(void) {
    SecureTunnelConfig config = test_config();

    subscribe_to_aws_tunnel_tokens_ExpectAndReturn(&config, GG_ERR_INVALID);

    GgError ret = run_secure_tunnel_with_retry(&config, 0, fake_sleep);

    TEST_ASSERT_EQUAL(GG_ERR_FAILURE, ret);
    TEST_ASSERT_EQUAL_size_t(0, recorded_sleep_count);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_run_secure_tunnel_success);
    RUN_TEST(test_retry_succeeds_on_first_attempt_without_sleeping);
    RUN_TEST(test_retry_recovers_after_transient_timeouts);
    RUN_TEST(test_retry_backoff_is_capped);
    RUN_TEST(test_retry_gives_up_after_max_attempts);
    RUN_TEST(test_retry_does_not_retry_config_errors);
    RUN_TEST(test_retry_does_not_retry_invalid_thing_name);

    return UNITY_END();
}
