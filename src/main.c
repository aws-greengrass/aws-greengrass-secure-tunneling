/*
 * aws.greengrass.SecureTunneling - AWS Greengrass component for secure
 * tunneling to IoT devices using AWS IoT Device Management Secure Tunneling
 * service.
 *
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "secure-tunnel.h"
#include <argp.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/log.h>
#include <gg/sdk.h>
#include <gg/utils.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_MAX_CONCURRENT_TUNNELS 20
#define DEFAULT_TUNNEL_TIMEOUT_SECONDS 43200 // 12 hours

static char doc[]
    = "secure-tunnel -- AWS Greengrass Secure Tunneling component";

static struct argp_option opts[] = {
    { "thing-name", 't', "name", 0, "Thing name", 0 },
    { "region", 'r', "region", 0, "AWS region", 0 },
    { "max-tunnels", 'm', "count", 0, "Maximum concurrent tunnels", 0 },
    { "timeout", 'T', "seconds", 0, "Tunnel timeout in seconds", 0 },
    { "artifact-path", 'a', "path", 0, "Path to aws-local-proxy binary", 0 },
    { "version", 'v', 0, 0, "Show version information", 0 },
    { 0 }
};

static inline void set_config_defaults(SecureTunnelConfig *config) {
    if (config->max_concurrent_tunnels == 0) {
        config->max_concurrent_tunnels = DEFAULT_MAX_CONCURRENT_TUNNELS;
    }
    if (config->tunnel_timeout_seconds == 0) {
        config->tunnel_timeout_seconds = DEFAULT_TUNNEL_TIMEOUT_SECONDS;
    }
}

static inline void validate_required_fields(
    SecureTunnelConfig *config, struct argp_state *state
) {
    if (config->region.len == 0) {
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        char *env_region = getenv("AWS_REGION");
        if (env_region != NULL) {
            config->region = gg_buffer_from_null_term(env_region);
        } else {
            GG_LOGE("Error: region is required");
            // NOLINTNEXTLINE(concurrency-mt-unsafe)
            argp_usage(state);
            // NOLINTNEXTLINE(concurrency-mt-unsafe)
            exit(1);
        }
    }

    if (config->thing_name.len == 0 || config->region.len == 0
        || config->artifact_path.len == 0) {
        GG_LOGE("Error: thingName and local proxy paths are required");
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        argp_usage(state);
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        exit(1);
    }
}

static error_t arg_parser(int key, char *arg, struct argp_state *state) {
    SecureTunnelConfig *args = state->input;
    switch (key) {
    case 't':
        args->thing_name = gg_buffer_from_null_term(arg);
        break;
    case 'r':
        args->region = gg_buffer_from_null_term(arg);
        break;
    case 'm': {
        int val = atoi(arg);
        if (val <= 0) {
            GG_LOGE("Error: max-tunnels must be a positive integer");
            return ARGP_ERR_UNKNOWN;
        }
        args->max_concurrent_tunnels = val;
        break;
    }
    case 'T': {
        int val = atoi(arg);
        if (val <= 0) {
            GG_LOGE("Error: timeout must be a positive integer");
            return ARGP_ERR_UNKNOWN;
        }
        args->tunnel_timeout_seconds = val;
        break;
    }
    case 'a':
        args->artifact_path = gg_buffer_from_null_term(arg);
        break;
    case 'v':
        printf(SEC_TUN_VERSION "\n");

        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        exit(0);
        break;
    case ARGP_KEY_END:
        validate_required_fields(args, state);
        set_config_defaults(args);

        if (args->max_concurrent_tunnels > 20) {
            GG_LOGE(
                "Error: maxConcurrentTunnels cannot exceed 20 (provided: %d)",
                args->max_concurrent_tunnels
            );
            // NOLINTNEXTLINE(concurrency-mt-unsafe)
            exit(1);
        }

        if (args->tunnel_timeout_seconds > 43200) {
            GG_LOGE(
                "Error: tunnelTimeoutSeconds cannot exceed 43200 (provided: "
                "%d)",
                args->tunnel_timeout_seconds
            );
            // NOLINTNEXTLINE(concurrency-mt-unsafe)
            exit(1);
        }

        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { opts, arg_parser, 0, doc, 0, 0, 0 };

int main(int argc, char *argv[]) {
    static SecureTunnelConfig args = { 0 };

    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    argp_parse(&argp, argc, argv, 0, 0, &args);

    gg_sdk_init();

    GG_LOGI("Starting Secure Tunnel component");
    GG_LOGI(
        "Thing name: %.*s", (int) args.thing_name.len, args.thing_name.data
    );
    GG_LOGI("Max concurrent tunnels: %d", args.max_concurrent_tunnels);
    GG_LOGI("Tunnel timeout: %d seconds", args.tunnel_timeout_seconds);

    if (run_secure_tunnel(&args) != GG_ERR_OK) {
        GG_LOGE("Failed to run secure tunnel");
        return 1;
    }

    // Keep subscription alive
    GG_LOGI("Secure tunnel running, waiting for notifications...");
    while (true) {
        (void) gg_sleep(60);
    }

    return 0;
}
