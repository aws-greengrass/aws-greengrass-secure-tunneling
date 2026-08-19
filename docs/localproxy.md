# Building Local Proxy

## Overview

This component requires a prebuilt
[localproxy binary version >=v3.2.0](https://github.com/aws-samples/aws-iot-securetunneling-localproxy/tree/main)
from AWS IoT Secure Tunneling.

If you want arm64, arm7l or x86 linux build these are available as pre-built
binaries with the repo's
[latest release](https://github.com/aws-samples/aws-iot-securetunneling-localproxy/releases/).

## Build Instructions

The following commands assume you're building on the same architecture as your
target device. For cross-compilation, see the
[localproxy README](https://github.com/aws-greengrass/aws-greengrass-secure-tunneling).

### Install Dependencies

```sh
sudo apt-get update && sudo apt-get install -y build-essential cmake wget git libssl-dev zlib1g-dev
```

### Build

```sh
mkdir -p build && cd build
cmake .. -DBUILD_TESTS=OFF -DLINK_STATIC_OPENSSL=OFF -DLOCALPROXY_DEP_MODE=fetch
make -j$(nproc)
strip bin/localproxy
```

The resulting binary should be approximately 4MB.

### Deployment

Upload the localproxy binary to S3 so it can be included in your component
deployment.
