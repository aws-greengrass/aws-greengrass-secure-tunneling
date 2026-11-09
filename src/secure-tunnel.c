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
#include <gg/backoff.h>
#include <gg/error.h>
#include <gg/log.h>
#include <stdbool.h>
#include <stdint.h>

// gg_backoff() sleeps a random time up to the current window (full jitter);
// the window starts at 5 s, doubles on every failure and is capped at 60 s.
#define SUBSCRIBE_RETRY_BASE_MS 5000
#define SUBSCRIBE_RETRY_MAX_MS 60000

typedef struct {
    const SecureTunnelConfig *config;
    GgError permanent_error;
    uint32_t attempt;
} SubscribeState;

// Errors that retrying cannot fix: missing IPC environment or an invalid thing
// name. Everything else (IPC timeout, nucleus not yet connected to AWS IoT
// Core, IPC socket not ready) is treated as transient.
static bool is_permanent_error(GgError err) {
    return (err == GG_ERR_CONFIG) || (err == GG_ERR_INVALID);
}

// gg_backoff() keeps calling this until it returns GG_ERR_OK. A permanent
// error therefore also returns GG_ERR_OK, with the real error kept in the
// state, so that the backoff loop stops instead of retrying forever.
static GgError try_subscribe(void *ctx) {
    SubscribeState *state = ctx;
    state->attempt++;

    GgError ret = subscribe_to_aws_tunnel_tokens(state->config);
    if (ret == GG_ERR_OK) {
        if (state->attempt > 1) {
            GG_LOGI(
                "Subscribed to tunnel notifications after %u attempts",
                state->attempt
            );
        }
        return GG_ERR_OK;
    }

    if (is_permanent_error(ret)) {
        GG_LOGE(
            "Failed to subscribe to aws for tunnel tokens: %d (not retrying)",
            ret
        );
        state->permanent_error = ret;
        return GG_ERR_OK;
    }

    GG_LOGW(
        "Failed to subscribe to aws for tunnel tokens (attempt %u, error %d); "
        "the nucleus may not be connected to AWS IoT Core yet. Retrying.",
        state->attempt,
        ret
    );
    return ret;
}

GgError run_secure_tunnel(const SecureTunnelConfig *config) {
    SubscribeState state
        = { .config = config, .permanent_error = GG_ERR_OK, .attempt = 0 };

    GgError ret = gg_backoff(
        SUBSCRIBE_RETRY_BASE_MS,
        SUBSCRIBE_RETRY_MAX_MS,
        0,
        try_subscribe,
        &state
    );
    if ((ret != GG_ERR_OK) || (state.permanent_error != GG_ERR_OK)) {
        return GG_ERR_FAILURE;
    }
    return GG_ERR_OK;
}
