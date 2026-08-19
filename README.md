# aws.greengrass.SecureTunneling

A Greengrass component that enables secure tunneling to IoT devices using AWS
IoT Device Management Secure Tunneling service.

This component listens for tunnel notifications and automatically establishes
secure tunnels using the localproxy client, supporting services like SSH and
VNC.

## Quick Start

Please check the release build under
[GitHub releases](https://github.com/aws-greengrass/aws-greengrass-component-sdk/releases).
If your target OS and architecture are already available, proceed directly to
step 4 below.

1. [Build localproxy](docs/localproxy.md)
2. [Build the component](docs/BUILD.md)
3. [Set up AWS permissions](docs/deployment.md#prerequisites)
4. Deploy to your device:
   - [Local deployment](docs/deployment.md#local-deployment)
   - [Cloud deployment](docs/deployment.md#cloud-deployment)
   - [GDK deployment](docs/gdk.md) (recommended)
5. [Create and use tunnels](docs/usage.md)
6. [Running behind a web proxy](docs/Proxy.md) (optional)

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

## Automated Builds with CI/CD

To automate the build process for the secure tunneling binaries, you can
reference the following sample CI/CD workflow files:

- **Secure tunnel component**:
  [`.github/workflows/release.yml`](https://github.com/aws-greengrass/aws-greengrass-secure-tunneling/blob/main/.github/workflows/release.yml)
  in the `aws-greengrass-secure-tunneling` repository.

**NOTE: This workflow only supports localproxy release version later than
3.3.0.**

The release workflow should always support the oldest currently supported LTS
release to ensure maximum device compatibility while meeting security standards.

If you need to build the localproxy binary for a different target architecture,
see the
[cross-compilation guide](https://github.com/aws-samples/aws-iot-securetunneling-localproxy#cross-compilation)
in the local proxy repository.

## License

See [LICENSE](LICENSE) file for details.
