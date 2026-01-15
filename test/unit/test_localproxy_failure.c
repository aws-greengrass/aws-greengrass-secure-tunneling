/*
 * Unit test for localproxy startup failure cleanup
 *
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_helpers.h"
// Include tunnel.c directly to access static variables
#include "tunnel.c"
#include <fcntl.h>
#include <gg/arena.h>
#include <gg/json_decode.h>
#include <string.h>
#include <sys/stat.h>
#include <unity.h>
#include <stdio.h>

#define TEST_DIR "/tmp/gg-test-localproxy"
#define MAX_TUNNEL_SLOTS 20

void test_nonexistent_binary_cleanup(void);
void test_nonexecutable_binary_cleanup(void);
void test_crashing_binary_cleanup(void);
void test_max_tunnel_slots_enforced(void);

static int initial_fd_count;

static int count_open_fds(void) {
    int count = 0;
    for (int fd = 0; fd < 1024; fd++) {
        if (fcntl(fd, F_GETFD) != -1) {
            count++;
        }
    }
    return count;
}

static void reset_tunnel_state(void) {
    pthread_mutex_lock(&tunnel_mutex);
    active_tunnels = 0;
    tunnel_slots_mask = 0;
    tunnel_config = NULL;
    pthread_mutex_unlock(&tunnel_mutex);
}

void setUp(void) {
    reset_tunnel_state();
    initial_fd_count = count_open_fds();
}

void tearDown(void) {
    test_remove_directory(TEST_DIR);
}

static SecureTunnelConfig test_config;

static SecureTunnelConfig *make_config_with_max(
    const char *artifact_path, int max_tunnels
) {
    test_config = (SecureTunnelConfig) {
        .thing_name = GG_STR("test-thing"),
        .region = GG_STR("us-west-2"),
        .artifact_path = gg_buffer_from_null_term((char *) artifact_path),
        .max_concurrent_tunnels = max_tunnels,
        .tunnel_timeout_seconds = 300,
    };
    return &test_config;
}

static SecureTunnelConfig *make_config(const char *artifact_path) {
    return make_config_with_max(artifact_path, 1);
}

static char test_json[256];

static GgMap mock_create_tunnel_notification(
    uint8_t *arena_mem, size_t arena_size
) {
    strcpy(
        test_json,
        "{\"clientAccessToken\":\"test-token\","
        "\"region\":\"us-west-2\","
        "\"services\":[\"SSH\"]}"
    );

    GgArena arena
        = gg_arena_init((GgBuffer) { .data = arena_mem, .len = arena_size });
    GgObject obj;
    (void) gg_json_decode_destructive(
        gg_buffer_from_null_term(test_json), &arena, &obj
    );
    return gg_obj_into_map(obj);
}

// Test non-existent localproxy binary
void test_nonexistent_binary_cleanup(void) {
    SecureTunnelConfig *config = make_config("/nonexistent/path");
    uint8_t arena_mem[1024];
    GgMap notification
        = mock_create_tunnel_notification(arena_mem, sizeof(arena_mem));

    GgError ret = handle_tunnel_notification(notification, config);
    TEST_ASSERT_EQUAL(GG_ERR_OK, ret);

    // Wait for worker thread to complete
    usleep(100000); // 100ms

    TEST_ASSERT_EQUAL_INT(0, active_tunnels);
    TEST_ASSERT_EQUAL_UINT32(0, tunnel_slots_mask);
    TEST_ASSERT_EQUAL_INT(initial_fd_count, count_open_fds());
}

// Test non-executable localproxy binary
void test_nonexecutable_binary_cleanup(void) {
    // Create temp directory with non-executable localproxy
    mkdir(TEST_DIR, 0755);
    FILE *f = fopen(TEST_DIR "/localproxy", "w");
    TEST_ASSERT_NOT_NULL(f);
    fclose(f);
    chmod(TEST_DIR "/localproxy", 0644); // No execute permission

    SecureTunnelConfig *config = make_config(TEST_DIR);
    uint8_t arena_mem[1024];
    GgMap notification
        = mock_create_tunnel_notification(arena_mem, sizeof(arena_mem));

    GgError ret = handle_tunnel_notification(notification, config);
    TEST_ASSERT_EQUAL(GG_ERR_OK, ret);

    usleep(100000);

    TEST_ASSERT_EQUAL_INT(0, active_tunnels);
    TEST_ASSERT_EQUAL_UINT32(0, tunnel_slots_mask);
    TEST_ASSERT_EQUAL_INT(initial_fd_count, count_open_fds());
}

// Test crashing localproxy binary
void test_crashing_binary_cleanup(void) {
    // Create temp directory with crashing localproxy script
    mkdir(TEST_DIR, 0755);
    FILE *f = fopen(TEST_DIR "/localproxy", "w");
    TEST_ASSERT_NOT_NULL(f);
    fprintf(f, "#!/bin/sh\nkill -SEGV $$\n");
    fclose(f);
    chmod(TEST_DIR "/localproxy", 0755);

    SecureTunnelConfig *config = make_config(TEST_DIR);
    uint8_t arena_mem[1024];
    GgMap notification
        = mock_create_tunnel_notification(arena_mem, sizeof(arena_mem));

    GgError ret = handle_tunnel_notification(notification, config);
    TEST_ASSERT_EQUAL(GG_ERR_OK, ret);

    // Wait for worker thread to complete by polling active_tunnels
    for (int i = 0; i < 50 && active_tunnels > 0; i++) {
        usleep(100000); // 100ms
    }

    TEST_ASSERT_EQUAL_INT(0, active_tunnels);
    TEST_ASSERT_EQUAL_UINT32(0, tunnel_slots_mask);
    TEST_ASSERT_EQUAL_INT(initial_fd_count, count_open_fds());
}

// Test that 21st tunnel is rejected when 20 slots are occupied
void test_max_tunnel_slots_enforced(void) {
    SecureTunnelConfig *config
        = make_config_with_max("/nonexistent", MAX_TUNNEL_SLOTS + 1);
    uint8_t arena_mem[1024];

    // Manually occupy all 20 slots
    pthread_mutex_lock(&tunnel_mutex);
    active_tunnels = MAX_TUNNEL_SLOTS;
    tunnel_slots_mask = 0xFFFFF; // All 20 bits set
    pthread_mutex_unlock(&tunnel_mutex);

    GgMap notification
        = mock_create_tunnel_notification(arena_mem, sizeof(arena_mem));
    GgError ret = handle_tunnel_notification(notification, config);

    TEST_ASSERT_EQUAL(GG_ERR_NOMEM, ret);
    TEST_ASSERT_EQUAL_INT(MAX_TUNNEL_SLOTS, active_tunnels);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nonexistent_binary_cleanup);
    RUN_TEST(test_nonexecutable_binary_cleanup);
    RUN_TEST(test_crashing_binary_cleanup);
    RUN_TEST(test_max_tunnel_slots_enforced);
    return UNITY_END();
}
