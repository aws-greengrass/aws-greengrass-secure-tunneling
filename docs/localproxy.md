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
[localproxy README](https://github.com/aws-samples/aws-iot-securetunneling-localproxy/blob/feb59e268c8f4f1c7450f3a510963e84cc397ac7/README.md).

### Install Dependencies

```sh
sudo apt-get update && sudo apt-get install -y build-essential cmake wget git libssl-dev zlib1g-dev

# Install Boost 1.87.0
wget -q https://archives.boost.io/release/1.87.0/source/boost_1_87_0.tar.gz -O boost.tar.gz
tar xzf boost.tar.gz && cd boost_1_87_0
./bootstrap.sh --prefix=/usr/local
sudo ./b2 install link=static -j$(nproc)
cd ..

# Install Protobuf 3.17.3
wget -q https://github.com/protocolbuffers/protobuf/releases/download/v3.17.3/protobuf-all-3.17.3.tar.gz -O protobuf.tar.gz
tar xzf protobuf.tar.gz && cd protobuf-3.17.3
mkdir -p build && cd build
cmake ../cmake -DCMAKE_INSTALL_PREFIX=/usr/local -Dprotobuf_BUILD_TESTS=OFF
make -j$(nproc) && sudo make install
cd ../..
```

### Build

```sh
mkdir -p build && cd build
cmake .. -DBUILD_TESTS=OFF -DLINK_STATIC_OPENSSL=OFF
make -j$(nproc)
strip bin/localproxy
```

The resulting binary should be approximately 4MB.

### Deployment

Upload the localproxy binary to S3 so it can be included in your component
deployment.
