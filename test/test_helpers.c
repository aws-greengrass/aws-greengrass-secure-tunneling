/*
 * Test helpers for aws-greengrass-secure-tunnel
 *
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_helpers.h"
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Global variables required by CMock
int GlobalExpectCount;
int GlobalVerifyOrder;

void assert_gg_buffer_equal(GgBuffer expected, GgBuffer actual) {
    TEST_ASSERT_EQUAL_size_t(expected.len, actual.len);
    TEST_ASSERT_EQUAL_MEMORY(expected.data, actual.data, expected.len);
}

void test_remove_directory(const char *path) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0
            || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (entry->d_type == DT_DIR) {
            test_remove_directory(full_path);
        } else {
            unlink(full_path);
        }
    }
    closedir(dir);
    rmdir(path);
}
