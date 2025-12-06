/*
 * aws.greengrass.SecureTunneling - AWS Greengrass component for secure
 * tunneling to IoT devices using AWS IoT Device Management Secure Tunneling
 * service.
 *
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ST_TUNNEL_H
#define ST_TUNNEL_H

#include "secure-tunnel.h"
#include <gg/error.h>
#include <gg/object.h>
#include <stdint.h>

typedef struct {
    char access_token[1024];
    char region[64];
    char service[64];
    uint16_t port;
} TunnelCreationContext;

GgError handle_tunnel_notification(
    GgMap notification, const SecureTunnelConfig *config
);

#endif // ST_TUNNEL_H
