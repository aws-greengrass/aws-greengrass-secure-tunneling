/*
 * aws.greengrass.SecureTunneling - AWS Greengrass component for secure
 * tunneling to IoT devices using AWS IoT Device Management Secure Tunneling
 * service.
 *
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ST_SUBSCRIPTIONS_H
#define ST_SUBSCRIPTIONS_H

#include "secure-tunnel.h"
#include <gg/error.h>

GgError subscribe_to_aws_tunnel_tokens(const SecureTunnelConfig *config);

#endif // ST_SUBSCRIPTIONS_H
