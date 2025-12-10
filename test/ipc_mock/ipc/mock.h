#ifndef GG_IPC_MOCK_H
#define GG_IPC_MOCK_H

#include <gg/attr.h>
#include <gg/eventstream/rpc.h>
#include <gg/eventstream/types.h>
#include <gg/object.h>
#include <sys/types.h>
#include <stddef.h>

typedef enum {
    CLIENT_TO_SERVER = 0,
    SERVER_TO_CLIENT = 1
} GgipcPacketDirection;

NULL_TERMINATED_STRING_ARG(1) NULL_TERMINATED_STRING_ARG(3)
GgError gg_test_setup_ipc(
    const char *socket_path_prefix, mode_t mode, const char *auth_token
);

GgBuffer gg_test_get_socket_path(void);
GgBuffer gg_test_get_auth_token(void);

#define GG_TEST_MAX_HEADER_COUNT 10

typedef struct {
    EventStreamHeader headers[GG_TEST_MAX_HEADER_COUNT];
    uint32_t header_count;
    int direction;
    bool has_payload;
    GgObject payload;
} GgipcPacket;

typedef struct {
    GgipcPacket packets[10];
    size_t len;
} GgipcPacketSequence;

#define GG_PACKET_SEQUENCE(...) \
    (GgipcPacketSequence) { \
        .packets = (GgipcPacket[]) { __VA_ARGS__ }, \
        .len = (sizeof((GgipcPacket[]) { __VA_ARGS__ })) \
            / (sizeof(GgipcPacket)) \
    }

#define GG_PACKET_SEQUENCE_FOREACH(name, seq) \
    for (GgipcPacket *name = (seq).packets; name < &(seq).packets[(seq).len]; \
         name = &name[1])

GgError gg_test_accept_client(int client_timeout);
GgError gg_test_expect_packet_sequence(
    GgipcPacketSequence sequence, int client_timeout
);
GgError gg_test_disconnect(void);
GgError gg_test_wait_for_client_disconnect(int client_timeout);
void gg_test_close(void);

#endif
