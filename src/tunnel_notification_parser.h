/*
 * aws.greengrass.SecureTunneling - AWS Greengrass component for secure
 * tunneling to IoT devices using AWS IoT Device Management Secure Tunneling
 * service.
 *
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ST_TUNNEL_NOTIFICATION_PARSER_H
#define ST_TUNNEL_NOTIFICATION_PARSER_H

#include "tunnel.h"
#include <gg/error.h>
#include <gg/object.h>

GgError parse_and_validate_notification(
    GgMap notification, TunnelCreationContext *request
);

#endif
