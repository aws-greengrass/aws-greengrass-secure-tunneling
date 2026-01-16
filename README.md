# aws.greengrass.SecureTunneling

A Greengrass component that enables secure tunneling to IoT devices using AWS
IoT Device Management Secure Tunneling service.

This component listens for tunnel notifications and automatically establishes
secure tunnels using the localproxy client, supporting services like SSH and
VNC.

## Quick Start

1. [Build localproxy](docs/localproxy.md)
2. [Build the component](docs/BUILD.md)
3. [Set up AWS permissions](docs/deployment.md#prerequisites)
4. Deploy to your device:
   - [Local deployment](docs/deployment.md#local-deployment)
   - [GDK deployment](docs/gdk.md) (recommended)
5. [Create and use tunnels](docs/usage.md)

## Configuration

### Component Parameters

The component supports the following configuration parameters:

#### maxConcurrentTunnels

Maximum number of concurrent tunnels allowed.

- Type: Integer
- Default: `20`

#### tunnelTimeoutSeconds

Tunnel timeout duration in seconds.

- Type: Integer
- Default: `43200` (12 hours)

## Supported Services

| Service | Port |
| ------- | ---- |
| SSH     | 22   |
| VNC     | 5900 |

## Resource Usage

| Component                    | Binary Size | Memory  |
| ---------------------------- | ----------- | ------- |
| aws-greengrass-secure-tunnel | 69 KB       | ~140 KB |
| localproxy                   | 3.9 MB      | ~2.2 MB |
| **Total**                    | ~4.0 MB     | ~2.3 MB |

_Note: Measurements are for MinSizeRel builds. Memory values represent
unique/private memory (RssAnon). Shared libraries and file-backed memory are not
included in these measurements._

## Runtime Dependencies

| Library   | Minimum Version | Required By |
| --------- | --------------- | ----------- |
| glibc     | 2.35            | Both        |
| libstdc++ | 3.4.29          | localproxy  |
| libgcc_s  | 3.0             | localproxy  |
| OpenSSL   | 3.0.0           | localproxy  |

Install on Ubuntu:

```bash
sudo apt install libc6 libstdc++6 libgcc-s1 libssl3
```

## License

See [LICENSE](LICENSE) file for details.
