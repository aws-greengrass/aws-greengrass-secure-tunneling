#include "../../src/subscriptions.h"
#include <gg/ipc/client.h>
#include <gg/process_wait.h>
#include <gg/sdk.h>
#include <gg/test.h>
#include <ipc/mock.h>
#include <ipc/packet_sequences.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unity.h>

GG_TEST_DEFINE(subscribe_to_aws_tunnel_tokens_okay) {
    pid_t pid = fork();
    TEST_ASSERT_TRUE_MESSAGE(pid >= 0, "fork failed");

    if (pid == 0) {
        gg_sdk_init();
        SecureTunnelConfig config = { .thing_name = GG_STR("test-thing"),
                                      .region = GG_STR("us-east-1"),
                                      .artifact_path = GG_STR("/tmp"),
                                      .max_concurrent_tunnels = 1,
                                      .tunnel_timeout_seconds = 300 };

        GG_TEST_ASSERT_OK(subscribe_to_aws_tunnel_tokens(&config));
        TEST_PASS();
    }

    GG_TEST_ASSERT_OK(gg_test_accept_client(5));
    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_connect_accepted_sequence(gg_test_get_auth_token()), 5
    ));
    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_mqtt_subscribe_accepted_sequence(
            1,
            GG_STR("$aws/things/test-thing/tunnels/notify"),
            GG_STR("e30="),
            GG_STR("1"),
            0
        ),
        5
    ));
    GG_TEST_ASSERT_OK(gg_test_wait_for_client_disconnect(5));
    GG_TEST_ASSERT_OK(gg_process_wait(pid));
}

GG_TEST_DEFINE(subscribe_to_aws_tunnel_tokens_invalid_thing_name) {
    pid_t pid = fork();
    TEST_ASSERT_TRUE_MESSAGE(pid >= 0, "fork failed");

    if (pid == 0) {
        SecureTunnelConfig config
            = { .thing_name = (GgBuffer) { .data = NULL, .len = 0 },
                .region = GG_STR("us-east-1"),
                .artifact_path = GG_STR("/tmp"),
                .max_concurrent_tunnels = 1,
                .tunnel_timeout_seconds = 300 };

        TEST_ASSERT_EQUAL(
            GG_ERR_INVALID, subscribe_to_aws_tunnel_tokens(&config)
        );
        TEST_PASS();
    }

    GG_TEST_ASSERT_OK(gg_process_wait(pid));
}

GG_TEST_DEFINE(subscribe_to_aws_tunnel_tokens_connection_failure) {
    pid_t pid = fork();
    TEST_ASSERT_TRUE_MESSAGE(pid >= 0, "fork failed");

    if (pid == 0) {
        gg_sdk_init();
        SecureTunnelConfig config = { .thing_name = GG_STR("test-device"),
                                      .region = GG_STR("us-west-2"),
                                      .artifact_path = GG_STR("/tmp"),
                                      .max_concurrent_tunnels = 1,
                                      .tunnel_timeout_seconds = 300 };

        GG_TEST_ASSERT_OK(subscribe_to_aws_tunnel_tokens(&config));
        TEST_PASS();
    }

    GG_TEST_ASSERT_OK(gg_test_accept_client(5));
    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_connect_accepted_sequence(gg_test_get_auth_token()), 5
    ));
    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_mqtt_subscribe_accepted_sequence(
            1,
            GG_STR("$aws/things/test-device/tunnels/notify"),
            GG_STR("e30="),
            GG_STR("1"),
            0
        ),
        5
    ));
    GG_TEST_ASSERT_OK(gg_test_wait_for_client_disconnect(5));
    GG_TEST_ASSERT_OK(gg_process_wait(pid));
}

void tearDown(void) {
    (void) gg_test_disconnect();
}

int main(void) {
    GgError ret = gg_test_setup_ipc("/tmp/test_ipc", 0777, "test_token");
    if (ret != GG_ERR_OK) {
        return 1;
    }

    int result = gg_test_run_suite();

    gg_test_close();
    return result;
}
