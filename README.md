# aws.greengrass.SecureTunneling

A Greengrass component that enables secure tunneling to IoT devices using AWS
IoT Device Management Secure Tunneling service.

This component listens for tunnel notifications and automatically establishes
secure tunnels using the localproxy client, supporting services like SSH, HTTP,
HTTPS, RDP, and VNC.

## Quick Start

1. [Build localproxy](docs/localproxy.md)
2. [Build the component](docs/building.md)
3. [Set up AWS permissions](docs/deployment.md#prerequisites)
4. [Deploy to your device](docs/deployment.md#local-deployment)
5. [Create and use tunnels](docs/usage.md)

For configuration options, see [Configuration](docs/configuration.md).

## Supported Services

| Service | Port |
| ------- | ---- |
| SSH     | 22   |
| HTTP    | 80   |
| HTTPS   | 443  |
| RDP     | 3389 |
| VNC     | 5900 |

## Resource Usage

| Component                    | Binary Size | Memory    |
| ---------------------------- | ----------- | --------- |
| aws-greengrass-secure-tunnel | 267 KB      | ~140 KB   |
| localproxy                   | 12 MB       | ~2.1 MB   |
| **Total**                    | **~12 MB**  | **~3 MB** |

_Note: Memory values represent unique/private memory (RssAnon). Shared libraries
and file-backed memory are not included in these measurements._

## License

See [LICENSE](LICENSE) file for details.
