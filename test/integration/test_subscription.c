/*
 * Integration test for subscribe_to_aws_tunnel_tokens
 *
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "secure-tunnel.h"
#include "subscriptions.h"
#include <gg/ipc/client.h>
#include <gg/ipc/mock.h>
#include <gg/ipc/packet_sequences.h>
#include <gg/process_wait.h>
#include <gg/sdk.h>
#include <gg/test.h>
#include <sys/types.h>
#include <unistd.h>
#include <unity.h>
#include <stdlib.h>

#define GG_TEST_SOCKET_DIR "/tmp/gg-test-subscription"
#define GG_TEST_AUTH_TOKEN "test-auth-token"

void suiteSetUp(void) {
    GgError ret
        = gg_test_setup_ipc(GG_TEST_SOCKET_DIR, 0777, GG_TEST_AUTH_TOKEN);
    if (ret != GG_ERR_OK) {
        _Exit(1);
    }
}

void setUp(void) {
}

void tearDown(void) {
    (void) gg_test_disconnect();
}

int suiteTearDown(int num_failures) {
    gg_test_close();
    return num_failures;
}

GG_TEST_DEFINE(subscribe_to_tunnel_tokens_okay) {
    pid_t pid = fork();
    TEST_ASSERT_TRUE_MESSAGE(pid >= 0, "fork failed");

    if (pid == 0) {
        gg_sdk_init();

        SecureTunnelConfig config = {
            .thing_name = GG_STR("my-thing"),
            .region = GG_STR("us-west-2"),
            .artifact_path = GG_STR("/path/to/localproxy"),
            .max_concurrent_tunnels = 1,
            .tunnel_timeout_seconds = 300,
        };

        GG_TEST_ASSERT_OK(subscribe_to_aws_tunnel_tokens(&config));
#ifdef ENABLE_COVERAGE
        __gcov_flush();
#endif
        _Exit(0);
    }

    GG_TEST_ASSERT_OK(gg_test_accept_client(1));

    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_connect_accepted_sequence(gg_test_get_auth_token()), 5
    ));

    // Expect subscription to $aws/things/my-thing/tunnels/notify
    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_mqtt_subscribe_accepted_sequence(
            1,
            GG_STR("$aws/things/my-thing/tunnels/notify"),
            GG_STR(""), // empty payload for subscribe
            GG_STR("1"), // QoS 1
            0 // no messages to send back
        ),
        5
    ));

    GG_TEST_ASSERT_OK(gg_test_wait_for_client_disconnect(5));
    GG_TEST_ASSERT_OK(gg_process_wait(pid));
}

// Base64 encoded JSON:
// {"clientAccessToken":"test-token","region":"us-west-2","services":["SSH"]}
static const char *TUNNEL_NOTIFICATION_B64
    = "eyJjbGllbnRBY2Nlc3NUb2tlbiI6InRlc3QtdG9rZW4iLCJyZWdpb24iOiJ1cy13ZXN0LTIi"
      "LCJzZXJ2aWNlcyI6WyJTU0giXX0=";

GG_TEST_DEFINE(subscribe_receives_notification) {
    pid_t pid = fork();
    TEST_ASSERT_TRUE_MESSAGE(pid >= 0, "fork failed");

    if (pid == 0) {
        gg_sdk_init();

        SecureTunnelConfig config = {
            .thing_name = GG_STR("my-thing"),
            .region = GG_STR("us-west-2"),
            .artifact_path = GG_STR("/bin/echo"), // use echo as dummy
            .max_concurrent_tunnels = 1,
            .tunnel_timeout_seconds = 300,
        };

        GG_TEST_ASSERT_OK(subscribe_to_aws_tunnel_tokens(&config));

        // Wait for notification to be processed
        sleep(1);
#ifdef ENABLE_COVERAGE
        __gcov_flush();
#endif
        _Exit(0);
    }

    GG_TEST_ASSERT_OK(gg_test_accept_client(1));

    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_connect_accepted_sequence(gg_test_get_auth_token()), 5
    ));

    // Subscribe and send 1 notification message
    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_mqtt_subscribe_accepted_sequence(
            1,
            GG_STR("$aws/things/my-thing/tunnels/notify"),
            gg_buffer_from_null_term((char *) TUNNEL_NOTIFICATION_B64),
            GG_STR("1"),
            1 // send 1 message
        ),
        5
    ));

    GG_TEST_ASSERT_OK(gg_test_wait_for_client_disconnect(5));
    GG_TEST_ASSERT_OK(gg_process_wait(pid));
}

int main(void) {
    return gg_test_run_suite();
}
