/*
 * aws.greengrass.SecureTunneling - AWS Greengrass component for secure
 * tunneling to IoT devices using AWS IoT Device Management Secure Tunneling
 * service.
 *
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ST_SECURE_TUNNEL_H
#define ST_SECURE_TUNNEL_H

#include <gg/buffer.h>
#include <gg/error.h>
#include <stdint.h>

typedef struct {
    GgBuffer thing_name;
    GgBuffer region;
    GgBuffer artifact_path;
    int max_concurrent_tunnels;
    int tunnel_timeout_seconds;
} SecureTunnelConfig;

// Sleep function used between subscription attempts (gg_sleep in production).
typedef GgError (*SecureTunnelSleepFn)(int64_t seconds);

// Subscribes to tunnel notifications, retrying until it succeeds.
GgError run_secure_tunnel(const SecureTunnelConfig *config);

// Subscribes to tunnel notifications. Transient failures, such as the nucleus
// not being connected to AWS IoT Core yet (the IPC subscribe then times out),
// are retried with exponential backoff. Configuration errors are not retried.
// max_attempts == 0 retries forever.
GgError run_secure_tunnel_with_retry(
    const SecureTunnelConfig *config,
    uint32_t max_attempts,
    SecureTunnelSleepFn sleep_fn
);

#endif // ST_SECURE_TUNNEL_H
