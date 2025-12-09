/*
 * Integration test for error handling
 *
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_helpers.h"
#include "tunnel_notification_parser.h"
#include <gg/arena.h>
#include <gg/json_decode.h>
#include <unity.h>

void test_malformed_json(void);
void test_empty_json(void);
void test_missing_region(void);
void test_missing_services(void);
void test_empty_services_array(void);
void test_wrong_type_services(void);
void test_null_json(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_malformed_json(void) {
    char json[] = "{\"clientAccessToken\":123,"
                  "\"region\":\"us-west-2\","
                  "\"services\":[\"SSH\"]}";

    uint8_t arena_mem[1024];
    GgArena arena = gg_arena_init(GG_BUF(arena_mem));
    GgObject obj;

    GgError ret = gg_json_decode_destructive(
        gg_buffer_from_null_term(json), &arena, &obj
    );
    TEST_ASSERT_EQUAL(GG_ERR_OK, ret);

    TunnelCreationContext ctx = { 0 };
    ret = parse_and_validate_notification(gg_obj_into_map(obj), &ctx);
    TEST_ASSERT_NOT_EQUAL(GG_ERR_OK, ret);
}

void test_empty_json(void) {
    char json[] = "{}";

    uint8_t arena_mem[1024];
    GgArena arena = gg_arena_init(GG_BUF(arena_mem));
    GgObject obj;

    GgError ret = gg_json_decode_destructive(
        gg_buffer_from_null_term(json), &arena, &obj
    );
    TEST_ASSERT_EQUAL(GG_ERR_OK, ret);

    TunnelCreationContext ctx = { 0 };
    ret = parse_and_validate_notification(gg_obj_into_map(obj), &ctx);
    TEST_ASSERT_NOT_EQUAL(GG_ERR_OK, ret);
}

void test_missing_region(void) {
    char json[] = "{\"clientAccessToken\":\"token\","
                  "\"services\":[\"SSH\"]}";

    uint8_t arena_mem[1024];
    GgArena arena = gg_arena_init(GG_BUF(arena_mem));
    GgObject obj;

    GgError ret = gg_json_decode_destructive(
        gg_buffer_from_null_term(json), &arena, &obj
    );
    TEST_ASSERT_EQUAL(GG_ERR_OK, ret);

    TunnelCreationContext ctx = { 0 };
    ret = parse_and_validate_notification(gg_obj_into_map(obj), &ctx);
    TEST_ASSERT_NOT_EQUAL(GG_ERR_OK, ret);
}

void test_missing_services(void) {
    char json[] = "{\"clientAccessToken\":\"token\","
                  "\"region\":\"us-west-2\"}";

    uint8_t arena_mem[1024];
    GgArena arena = gg_arena_init(GG_BUF(arena_mem));
    GgObject obj;

    GgError ret = gg_json_decode_destructive(
        gg_buffer_from_null_term(json), &arena, &obj
    );
    TEST_ASSERT_EQUAL(GG_ERR_OK, ret);

    TunnelCreationContext ctx = { 0 };
    ret = parse_and_validate_notification(gg_obj_into_map(obj), &ctx);
    TEST_ASSERT_NOT_EQUAL(GG_ERR_OK, ret);
}

void test_empty_services_array(void) {
    char json[] = "{\"clientAccessToken\":\"token\","
                  "\"region\":\"us-west-2\","
                  "\"services\":[]}";

    uint8_t arena_mem[1024];
    GgArena arena = gg_arena_init(GG_BUF(arena_mem));
    GgObject obj;

    GgError ret = gg_json_decode_destructive(
        gg_buffer_from_null_term(json), &arena, &obj
    );
    TEST_ASSERT_EQUAL(GG_ERR_OK, ret);

    TunnelCreationContext ctx = { 0 };
    ret = parse_and_validate_notification(gg_obj_into_map(obj), &ctx);
    TEST_ASSERT_NOT_EQUAL(GG_ERR_OK, ret);
}

void test_wrong_type_services(void) {
    char json[] = "{\"clientAccessToken\":\"token\","
                  "\"region\":\"us-west-2\","
                  "\"services\":\"SSH\"}";

    uint8_t arena_mem[1024];
    GgArena arena = gg_arena_init(GG_BUF(arena_mem));
    GgObject obj;

    GgError ret = gg_json_decode_destructive(
        gg_buffer_from_null_term(json), &arena, &obj
    );
    TEST_ASSERT_EQUAL(GG_ERR_OK, ret);

    TunnelCreationContext ctx = { 0 };
    ret = parse_and_validate_notification(gg_obj_into_map(obj), &ctx);
    TEST_ASSERT_NOT_EQUAL(GG_ERR_OK, ret);
}

void test_null_json(void) {
    char json[] = "null";

    uint8_t arena_mem[1024];
    GgArena arena = gg_arena_init(GG_BUF(arena_mem));
    GgObject obj;

    GgError ret = gg_json_decode_destructive(
        gg_buffer_from_null_term(json), &arena, &obj
    );
    TEST_ASSERT_EQUAL(GG_ERR_OK, ret);
    TEST_ASSERT_NOT_EQUAL(GG_TYPE_MAP, gg_obj_type(obj));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_malformed_json);
    RUN_TEST(test_empty_json);
    RUN_TEST(test_missing_region);
    RUN_TEST(test_missing_services);
    RUN_TEST(test_empty_services_array);
    RUN_TEST(test_wrong_type_services);
    RUN_TEST(test_null_json);

    return UNITY_END();
}
