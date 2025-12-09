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
#include "tunnel.h"
#include <gg/arena.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/ipc/client.h>
#include <gg/json_decode.h>
#include <gg/log.h>
#include <gg/object.h>
#include <gg/utils.h>
#include <gg/vector.h>
#include <stdbool.h>
#include <stdint.h>

static void on_tunnel_notification(
    void *ctx, GgBuffer topic, GgBuffer payload, GgIpcSubscriptionHandle handle
) {
    const SecureTunnelConfig *config = (const SecureTunnelConfig *) ctx;
    (void) handle;
    GG_LOGI(
        "Received tunnel aws tunnel token on topic: %.*s",
        (int) topic.len,
        topic.data
    );

    static uint8_t arena_mem[4096];
    GgArena arena = gg_arena_init(GG_BUF(arena_mem));
    GgObject notification;

    GgError ret = gg_json_decode_destructive(payload, &arena, &notification);
    if (ret != GG_ERR_OK) {
        GG_LOGE("Failed to parse tunnel notification JSON: %d", ret);
        return;
    }

    if (gg_obj_type(notification) != GG_TYPE_MAP) {
        GG_LOGE("Invalid notification format");
        return;
    }

    GG_LOGI("Successfully parsed tunnel notification JSON");
    if (handle_tunnel_notification(gg_obj_into_map(notification), config)
        != GG_ERR_OK) {
        GG_LOGE("Failed to handle aws tunnel token notification");
    }
}

GgError subscribe_to_aws_tunnel_tokens(const SecureTunnelConfig *config) {
    uint8_t topic_memory[256];
    GgByteVec tunnel_token_sub_topic = GG_BYTE_VEC(topic_memory);

    GgError ret
        = gg_byte_vec_append(&tunnel_token_sub_topic, GG_STR("$aws/things/"));
    gg_byte_vec_chain_append(&ret, &tunnel_token_sub_topic, config->thing_name);
    gg_byte_vec_chain_append(
        &ret, &tunnel_token_sub_topic, GG_STR("/tunnels/notify")
    );
    if (ret != GG_ERR_OK) {
        return ret;
    }

    GG_LOGI("Connecting to Greengrass IPC");
    ret = ggipc_connect();
    if (ret != GG_ERR_OK) {
        GG_LOGE("Failed to connect to Greengrass IPC: %d", ret);
        return ret;
    }

    GG_LOGI(
        "Subscribing to IoT Core topic: %.*s",
        (int) tunnel_token_sub_topic.buf.len,
        tunnel_token_sub_topic.buf.data
    );
    GgIpcSubscriptionHandle sub_handle;
    ret = ggipc_subscribe_to_iot_core(
        tunnel_token_sub_topic.buf,
        1, // QoS 1
        on_tunnel_notification,
        (void *) config,
        &sub_handle
    );
    if (ret != GG_ERR_OK) {
        GG_LOGE("Failed to subscribe to tunnel notifications: %d", ret);
        return ret;
    }

    GG_LOGI("Successfully subscribed to tunnel notifications");
    return GG_ERR_OK;
}
