#ifndef GG_IPC_PACKET_SEQUENCES_H
#define GG_IPC_PACKET_SEQUENCES_H

#include "mock.h"

GgipcPacketSequence gg_test_connect_accepted_sequence(GgBuffer auth_token);
GgipcPacketSequence gg_test_connect_hangup_sequence(GgBuffer auth_token);
GgipcPacketSequence gg_test_mqtt_publish_accepted_sequence(
    int32_t stream_id, GgBuffer topic, GgBuffer payload_base64, GgBuffer qos
);
GgipcPacketSequence gg_test_mqtt_publish_error_sequence(
    int32_t stream_id, GgBuffer topic, GgBuffer payload_base64, GgBuffer qos
);
GgipcPacketSequence gg_test_mqtt_subscribe_accepted_sequence(
    int32_t stream_id,
    GgBuffer topic,
    GgBuffer payload_base64,
    GgBuffer qos,
    size_t messages
);

#endif
