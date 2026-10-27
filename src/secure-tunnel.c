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
#include <gg/utils.h>
#include <stdbool.h>
#include <stdint.h>

#define SUBSCRIBE_RETRY_INITIAL_DELAY_S 5
#define SUBSCRIBE_RETRY_MAX_DELAY_S 60

// Errors that retrying cannot fix: missing IPC environment or an invalid thing
// name. Everything else (IPC timeout, nucleus not yet connected to AWS IoT
// Core, IPC socket not ready) is treated as transient.
static bool is_permanent_error(GgError err) {
    return (err == GG_ERR_CONFIG) || (err == GG_ERR_INVALID);
}

GgError run_secure_tunnel_with_retry(
    const SecureTunnelConfig *config,
    uint32_t max_attempts,
    SecureTunnelSleepFn sleep_fn
) {
    int64_t delay_s = SUBSCRIBE_RETRY_INITIAL_DELAY_S;

    for (uint32_t attempt = 1;; attempt++) {
        GgError ret = subscribe_to_aws_tunnel_tokens(config);
        if (ret == GG_ERR_OK) {
            if (attempt > 1) {
                GG_LOGI(
                    "Subscribed to tunnel notifications after %u attempts",
                    attempt
                );
            }
            return GG_ERR_OK;
        }

        if (is_permanent_error(ret)) {
            GG_LOGE(
                "Failed to subscribe to aws for tunnel tokens: %d (not "
                "retrying)",
                ret
            );
            return GG_ERR_FAILURE;
        }

        if ((max_attempts != 0) && (attempt >= max_attempts)) {
            GG_LOGE(
                "Failed to subscribe to aws for tunnel tokens after %u "
                "attempts: %d",
                attempt,
                ret
            );
            return GG_ERR_FAILURE;
        }

        GG_LOGW(
            "Failed to subscribe to aws for tunnel tokens (attempt %u, error "
            "%d); the nucleus may not be connected to AWS IoT Core yet. "
            "Retrying in %lld s.",
            attempt,
            ret,
            (long long) delay_s
        );
        (void) sleep_fn(delay_s);

        delay_s *= 2;
        if (delay_s > SUBSCRIBE_RETRY_MAX_DELAY_S) {
            delay_s = SUBSCRIBE_RETRY_MAX_DELAY_S;
        }
    }
}

GgError run_secure_tunnel(const SecureTunnelConfig *config) {
    return run_secure_tunnel_with_retry(config, 0, gg_sleep);
}
