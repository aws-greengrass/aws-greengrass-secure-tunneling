/*
 * Test helpers for aws-greengrass-secure-tunnel
 *
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <gg/buffer.h>
#include <unity.h>

void assert_gg_buffer_equal(GgBuffer expected, GgBuffer actual);
void test_remove_directory(const char *path);

#endif // TEST_HELPERS_H
