# Configuration

## Runtime Requirements

- AWS IoT Greengrass v2 (nucleus/nucleus lite)
- localproxy binary (included in component artifacts)
- Network connectivity to AWS IoT Core
- Appropriate IAM permissions (see [deployment.md](deployment.md#prerequisites))

## Component Parameters

The component supports the following configuration parameters:

### maxConcurrentTunnels

Maximum number of concurrent tunnels allowed.

- Type: Integer
- Default: `20`

### tunnelTimeoutSeconds

Tunnel timeout duration in seconds.

- Type: Integer
- Default: `43200` (12 hours)

## Supported Services

The component automatically maps services to local ports:

| Service | Port | Protocol |
| ------- | ---- | -------- |
| SSH     | 22   | TCP      |
| HTTP    | 80   | TCP      |
| HTTPS   | 443  | TCP      |
| RDP     | 3389 | TCP      |
| VNC     | 5900 | TCP      |
