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
#include "subscriptions.h"
#include <gg/error.h>
#include <gg/log.h>

GgError run_secure_tunnel(const SecureTunnelConfig *config) {
    GgError ret = subscribe_to_aws_tunnel_tokens(config);
    if (ret != GG_ERR_OK) {
        GG_LOGE("Failed to subscribe to aws for tunnel tokens");
        return GG_ERR_FAILURE;
    }

    return GG_ERR_OK;
}
