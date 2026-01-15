/*
 * Unit tests for secure_tunnel
 *
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Mocksubscriptions.h"
#include "secure-tunnel.h"
#include <unity.h>

void test_run_secure_tunnel_success(void);
void test_run_secure_tunnel_subscription_failure(void);

void setUp(void) {
    Mocksubscriptions_Init();
}

void tearDown(void) {
    Mocksubscriptions_Verify();
    Mocksubscriptions_Destroy();
}

void test_run_secure_tunnel_success(void) {
    SecureTunnelConfig config = { .thing_name = GG_STR("test-thing"),
                                  .region = GG_STR("us-west-2"),
                                  .artifact_path = GG_STR("/opt/localproxy"),
                                  .max_concurrent_tunnels = 5,
                                  .tunnel_timeout_seconds = 3600 };

    subscribe_to_aws_tunnel_tokens_ExpectAndReturn(&config, GG_ERR_OK);

    GgError ret = run_secure_tunnel(&config);

    TEST_ASSERT_EQUAL(GG_ERR_OK, ret);
}

void test_run_secure_tunnel_subscription_failure(void) {
    SecureTunnelConfig config = { .thing_name = GG_STR("test-thing"),
                                  .region = GG_STR("us-west-2"),
                                  .artifact_path = GG_STR("/opt/localproxy"),
                                  .max_concurrent_tunnels = 5,
                                  .tunnel_timeout_seconds = 3600 };

    subscribe_to_aws_tunnel_tokens_ExpectAndReturn(&config, GG_ERR_FAILURE);

    GgError ret = run_secure_tunnel(&config);

    TEST_ASSERT_EQUAL(GG_ERR_FAILURE, ret);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_run_secure_tunnel_success);
    RUN_TEST(test_run_secure_tunnel_subscription_failure);

    return UNITY_END();
}
