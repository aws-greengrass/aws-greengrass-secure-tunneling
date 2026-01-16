#include "tunnel_notification_parser.h"
#include <gg/buffer.h>
#include <gg/flags.h>
#include <gg/list.h>
#include <gg/log.h>
#include <gg/map.h>
#include <string.h>
#include <stdint.h>

static uint16_t get_port_from_service(GgBuffer service) {
    if (gg_buffer_eq(service, GG_STR("SSH"))) {
        return 22;
    }
    if (gg_buffer_eq(service, GG_STR("VNC"))) {
        return 5900;
    }
    return 0; // unknown port
}

GgError parse_and_validate_notification(
    GgMap notification, TunnelCreationContext *request
) {
    GgObject *token_obj = NULL;
    GgObject *region_obj = NULL;
    GgObject *services_obj = NULL;

    GgMapSchema schema = GG_MAP_SCHEMA(
        { GG_STR("clientAccessToken"), GG_REQUIRED, GG_TYPE_BUF, &token_obj },
        { GG_STR("region"), GG_REQUIRED, GG_TYPE_BUF, &region_obj },
        { GG_STR("services"), GG_REQUIRED, GG_TYPE_LIST, &services_obj }
    );

    GgError ret = gg_map_validate(notification, schema);
    if (ret != GG_ERR_OK) {
        GG_LOGE("Tunnel notification validation failed");
        return ret;
    }

    GgBuffer token = gg_obj_into_buf(*token_obj);
    GgBuffer region = gg_obj_into_buf(*region_obj);
    GgList services = gg_obj_into_list(*services_obj);

    ret = gg_list_type_check(services, GG_TYPE_BUF);
    if (ret != GG_ERR_OK) {
        GG_LOGE("Services list validation failed - must contain only strings");
        return ret;
    }

    if (services.len != 1) {
        GG_LOGE(
            "The component only supports one service per tunnel. Received: %d",
            (int) services.len
        );
        return GG_ERR_RANGE;
    }

    GgBuffer service = gg_obj_into_buf(services.items[0]);

    // Copy to null-terminated strings
    size_t token_len = token.len < sizeof(request->access_token) - 1
        ? token.len
        : sizeof(request->access_token) - 1;
    size_t region_len = region.len < sizeof(request->region) - 1
        ? region.len
        : sizeof(request->region) - 1;
    size_t service_len = service.len < sizeof(request->service) - 1
        ? service.len
        : sizeof(request->service) - 1;

    memcpy(request->access_token, token.data, token_len);
    request->access_token[token_len] = '\0';

    memcpy(request->region, region.data, region_len);
    request->region[region_len] = '\0';

    memcpy(request->service, service.data, service_len);
    request->service[service_len] = '\0';

    request->port = get_port_from_service(service);
    if (request->port == 0) {
        GG_LOGE("Unsupported service: %.*s", (int) service.len, service.data);
        return GG_ERR_INVALID;
    }

    return GG_ERR_OK;
}
