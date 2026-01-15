/*
 * Unit test for service name validation
 *
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tunnel.c"
#include <gg/arena.h>
#include <gg/json_decode.h>
#include <string.h>
#include <unistd.h>
#include <unity.h>

static char test_json[256];

static void reset_tunnel_state(void) {
    pthread_mutex_lock(&tunnel_mutex);
    active_tunnels = 0;
    tunnel_slots_mask = 0;
    tunnel_config = NULL;
    pthread_mutex_unlock(&tunnel_mutex);
}

void setUp(void) {
    reset_tunnel_state();
}

void tearDown(void) {
    usleep(10000); // Wait for any worker threads
    reset_tunnel_state();
}

static GgMap create_notification(uint8_t *arena_mem, const char *service) {
    snprintf(
        test_json,
        sizeof(test_json),
        "{\"clientAccessToken\":\"test-token\","
        "\"region\":\"us-west-2\","
        "\"services\":[\"%s\"]}",
        service
    );
    GgArena arena
        = gg_arena_init((GgBuffer) { .data = arena_mem, .len = 1024 });
    GgObject obj;
    (void) gg_json_decode_destructive(
        gg_buffer_from_null_term(test_json), &arena, &obj
    );
    return gg_obj_into_map(obj);
}

static SecureTunnelConfig config = {
    .thing_name = { .data = (uint8_t *) "test", .len = 4 },
    .region = { .data = (uint8_t *) "us-west-2", .len = 9 },
    .artifact_path = { .data = (uint8_t *) "/nonexistent", .len = 12 },
    .max_concurrent_tunnels = 20,
    .tunnel_timeout_seconds = 300,
};

static void test_ssh_service_accepted(void) {
    uint8_t arena_mem[1024];
    GgMap notification = create_notification(arena_mem, "SSH");
    TEST_ASSERT_EQUAL(
        GG_ERR_OK, handle_tunnel_notification(notification, &config)
    );
}

static void test_http_service_accepted(void) {
    uint8_t arena_mem[1024];
    GgMap notification = create_notification(arena_mem, "HTTP");
    TEST_ASSERT_EQUAL(
        GG_ERR_OK, handle_tunnel_notification(notification, &config)
    );
}

static void test_https_service_accepted(void) {
    uint8_t arena_mem[1024];
    GgMap notification = create_notification(arena_mem, "HTTPS");
    TEST_ASSERT_EQUAL(
        GG_ERR_OK, handle_tunnel_notification(notification, &config)
    );
}

static void test_rdp_service_accepted(void) {
    uint8_t arena_mem[1024];
    GgMap notification = create_notification(arena_mem, "RDP");
    TEST_ASSERT_EQUAL(
        GG_ERR_OK, handle_tunnel_notification(notification, &config)
    );
}

static void test_vnc_service_accepted(void) {
    uint8_t arena_mem[1024];
    GgMap notification = create_notification(arena_mem, "VNC");
    TEST_ASSERT_EQUAL(
        GG_ERR_OK, handle_tunnel_notification(notification, &config)
    );
}

static void test_random_service_rejected(void) {
    uint8_t arena_mem[1024];
    GgMap notification = create_notification(arena_mem, "RandomScript");
    TEST_ASSERT_EQUAL(
        GG_ERR_INVALID, handle_tunnel_notification(notification, &config)
    );
}

static void test_empty_service_rejected(void) {
    uint8_t arena_mem[1024];
    GgMap notification = create_notification(arena_mem, "");
    TEST_ASSERT_EQUAL(
        GG_ERR_INVALID, handle_tunnel_notification(notification, &config)
    );
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ssh_service_accepted);
    RUN_TEST(test_http_service_accepted);
    RUN_TEST(test_https_service_accepted);
    RUN_TEST(test_rdp_service_accepted);
    RUN_TEST(test_vnc_service_accepted);
    RUN_TEST(test_random_service_rejected);
    RUN_TEST(test_empty_service_rejected);
    return UNITY_END();
}
