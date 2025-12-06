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

typedef struct {
    GgBuffer thing_name;
    GgBuffer region;
    GgBuffer artifact_path;
    int max_concurrent_tunnels;
    int tunnel_timeout_seconds;
} SecureTunnelConfig;

// Function declarations
GgError run_secure_tunnel(const SecureTunnelConfig *config);

#endif // ST_SECURE_TUNNEL_H
