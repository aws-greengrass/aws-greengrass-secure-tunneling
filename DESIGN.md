# AWS Greengrass Secure Tunneling - Design Document

## Overview

Enables secure remote access to IoT devices behind firewalls using AWS IoT
Secure Tunneling. The component listens for tunnel notifications and launches
localproxy processes to establish secure connections.

## Problem Statement

When customers onboard their devices to AWS, they expect that cloud-connected
devices can be remotely accessed when needed. While services like SSH, HTTP, and
VNC enable device access, there is no off-the-shelf solution that can maintain
and track access for an entire fleet without requiring inbound firewall rules.

AWS IoT Secure Tunneling service addresses this by providing a managed solution
that:

- Securely tracks and maintains remote access across device fleets
- Eliminates the need for inbound firewall rule configuration
- Works with devices behind NAT and firewalls using only outbound connections

To use AWS IoT Secure Tunneling, devices need:

1. A secure method for receiving access tokens used for authentication
2. The ability to establish WebSocket connections between the cloud and local
   device services (SSH, VNC, etc.)

This component solves both requirements by subscribing to MQTT tunnel
notifications via Greengrass IPC and launching a websocket using
[localproxy code base](https://github.com/aws-samples/aws-iot-securetunneling-localproxy)
with the received access tokens.

This open source secure tunneling component is a drop in replacement for fleets
using the previously offered Greengrass Secure Tunneling component.

## Architecture

### High-Level Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   AWS Console   │    │   AWS IoT Core   │    │  Greengrass     │
│                 │    │                  │    │  Device         │
│ Create Tunnel   ├────┤ Secure Tunneling ├────┤                 │
│ Manage Sessions │    │ Service          │    │ SecureTunneling │
└─────────────────┘    └──────────────────┘    │ Component       │
                                               └─────────────────┘
```

### Component Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                      SecureTunneling Component                          │
├─────────────────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────────┐  ┌────────────────────────┐     │
│  │    Main      │  │  Subscription    │  │       Tunnel           │     │
│  │   Process    │  │    Manager       │  │       Manager          │     │
│  │              │  │                  │  │                        │     │
│  │ • CLI Args   │  │ • MQTT Subscribe │  │ • Parse AWS Access     │     │
│  │ • Config     │  │ • Event Loop     │  │   Token                │     │
│  │ • Lifecycle  │  │ • Error Handling │  │ • Launch Proxy         │     │
│  │              │  │                  │  │ • Process Management   │     │
│  └──────────────┘  └──────────────────┘  └────────────────────────┘     │
├─────────────────────────────────────────────────────────────────────────┤
│                           Greengrass SDK                                │
├─────────────────────────────────────────────────────────────────────────┤
│                            localproxy                                   │
│                      (AWS IoT Device SDK)                               │
└─────────────────────────────────────────────────────────────────────────┘
```

### Data Flow

1. User creates tunnel via AWS Console
2. AWS IoT Core publishes notification to device
3. Component receives MQTT notification
4. Parse notification for access token and service info
5. Launch localproxy with extracted parameters
6. localproxy establishes secure connection through websockets
7. Service becomes accessible through tunnel

### Message Flow

```
AWS Console → IoT Core → MQTT → Component → localproxy → Local Service
     ↑                                           ↓
     └─────────── Secure Tunnel ←────────────────┘
```

### Core Components

- **Main Process**: CLI parsing, configuration, lifecycle management
- **Subscription Manager**: MQTT subscription to
  `$aws/things/{thingName}/tunnels/notify`
- **Tunnel Manager**: Parse notifications, launch/manage localproxy processes
- **Notification Parser**: Extract access tokens and service configurations

## Key Design Decisions

### Process Model

- **One process per tunnel**: Each tunnel spawns a separate localproxy process
  that creates and maintains the websocket connection
- **Lifecycle management**: Component manages process lifecycle and cleanup
  - Resources are automatically freed when a tunnel closes or times out
  - Component tracks active localproxy processes and enforces limits
- **Concurrency limit**: Maximum 20 concurrent tunnels (consistent with legacy
  secure tunnel component)

### Security

- Uses AWS IoT device certificates for authentication
- Short-lived, single-use access tokens
- TLS 1.2+ encryption for all communication
- No inbound firewall rules required

### Resource Management

- Binary size: <5.0 MB
- Memory usage: ~2 MB(Per Tunnel)
- Automatic cleanup on tunnel timeout (default: 12 hours)

### Future Scope

To achieve even smaller footprint and memory usage the component can implement
the localproxy's
[V1 websocket protocol](https://github.com/aws-samples/aws-iot-securetunneling-localproxy/blob/main/V1WebSocketProtocolGuide.md)
in C. Furthermore, we can add support for additional protocol in phases to
expand support for multiplex tunnels.

## Configuration

See [README.md](README.md#configuration) for detailed parameters.

Key settings:

- `maxConcurrentTunnels`: Max simultaneous tunnels (default: 20)
- `tunnelTimeoutSeconds`: Tunnel lifetime (default: 43200)

## Supported Services

See README.md for complete service list. Common services:

- SSH (22), HTTP (80), HTTPS (443), RDP (3389), VNC (5900)

## Build & Test

- CMake-based build system with multi-platform support
- Unity/CMock testing framework
- See [BUILD.md](docs/BUILD.md) and [RunningTests.md](docs/RunningTests.md)

## Deployment

Packaged as Greengrass v2 component with S3 artifact distribution. See
[deployment.md](docs/deployment.md) for setup instructions.
