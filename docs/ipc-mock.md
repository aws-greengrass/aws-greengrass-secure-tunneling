# IPC Mock Testing Framework

The IPC mock simulates AWS IoT Greengrass IPC communication for integration
testing without requiring a real Greengrass nucleus.

## How It Works

The mock creates a Unix domain socket that mimics the Greengrass IPC server.
Tests fork into two processes:

- **Child process**: Runs the code under test, which connects to the mock socket
  as an IPC client
- **Parent process**: Acts as the mock IPC server, validating requests and
  sending responses

## Setup

```c
// Create mock socket and set environment variables
gg_test_setup_ipc("/tmp/test_ipc", 0777, "test_token");

// Run tests
int result = gg_test_run_suite();

// Cleanup
gg_test_close();
```

This creates a temporary socket at `/tmp/test_ipc_XXXXXX/socket.ipc` and sets:

- `AWS_GG_NUCLEUS_DOMAIN_SOCKET_FILEPATH_FOR_COMPONENT` - socket path
- `SVCUID` - authentication token

## Test Pattern

```c
GG_TEST_DEFINE(my_test) {
    pid_t pid = fork();

    if (pid == 0) {
        // Child: Run code under test
        gg_sdk_init();
        GG_TEST_ASSERT_OK(my_function());
        TEST_PASS();
    }

    // Parent: Validate IPC communication
    GG_TEST_ASSERT_OK(gg_test_accept_client(5));
    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_connect_accepted_sequence(gg_test_get_auth_token()), 5
    ));
    GG_TEST_ASSERT_OK(gg_test_wait_for_client_disconnect(5));
    GG_TEST_ASSERT_OK(gg_process_wait(pid));
}
```

## Packet Sequences

Packet sequences define expected IPC message exchanges:

```c
// Connection handshake
gg_test_connect_accepted_sequence(auth_token)

// MQTT subscribe with messages
gg_test_mqtt_subscribe_accepted_sequence(
    stream_id,           // Stream identifier
    topic,              // MQTT topic
    payload_base64,     // Message payload (base64)
    qos,                // QoS level
    message_count       // Number of messages to send
)

// MQTT publish
gg_test_mqtt_publish_accepted_sequence(
    stream_id, topic, payload_base64, qos
)
```

Each sequence contains packets with:

- **Direction**: `CLIENT_TO_SERVER` (received) or `SERVER_TO_CLIENT` (sent)
- **Headers**: EventStream protocol headers (message type, stream ID, etc.)
- **Payload**: Optional JSON payload as a GgObject

## API Functions

| Function                                       | Purpose                    |
| ---------------------------------------------- | -------------------------- |
| `gg_test_accept_client(timeout)`               | Wait for client to connect |
| `gg_test_expect_packet_sequence(seq, timeout)` | Validate message exchange  |
| `gg_test_disconnect()`                         | Close client connection    |
| `gg_test_wait_for_client_disconnect(timeout)`  | Wait for clean disconnect  |

Timeouts are in seconds. The mock validates:

- Header count and values match expected
- Payload structure matches expected (deep comparison)
- Packet order and direction

## Example

```c
// Test subscribing to tunnel notifications
GG_TEST_DEFINE(subscribe_to_aws_tunnel_tokens_okay) {
    pid_t pid = fork();

    if (pid == 0) {
        gg_sdk_init();
        SecureTunnelConfig config = {
            .thing_name = GG_STR("test-thing"),
            .region = GG_STR("us-east-1"),
            // ...
        };
        GG_TEST_ASSERT_OK(subscribe_to_aws_tunnel_tokens(&config));
        TEST_PASS();
    }

    // Expect connection handshake
    GG_TEST_ASSERT_OK(gg_test_accept_client(5));
    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_connect_accepted_sequence(gg_test_get_auth_token()), 5
    ));

    // Expect MQTT subscription to tunnel topic
    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_mqtt_subscribe_accepted_sequence(
            1,
            GG_STR("$aws/things/test-thing/tunnels/notify"),
            GG_STR("e30="),  // Empty JSON payload
            GG_STR("1"),     // QoS 1
            0                // No messages
        ), 5
    ));

    GG_TEST_ASSERT_OK(gg_test_wait_for_client_disconnect(5));
    GG_TEST_ASSERT_OK(gg_process_wait(pid));
}
```
