# Building Local Proxy

## Overview

This component requires a prebuilt
[localproxy binary](https://github.com/aws-samples/aws-iot-securetunneling-localproxy/tree/main)
from AWS IoT Secure Tunneling.

Tested with
[commit feb59e2](https://github.com/aws-samples/aws-iot-securetunneling-localproxy/commit/feb59e268c8f4f1c7450f3a510963e84cc397ac7).

## Build Instructions

The following commands assume you're building on the same architecture as your
target device. For cross-compilation, see the
[localproxy README](https://github.com/aws-samples/aws-iot-securetunneling-localproxy/blob/feb59e268c8f4f1c7450f3a510963e84cc397ac7/README.md).

### Install Dependencies

```sh
sudo apt update

# Install boost 1.87
wget https://archives.boost.io/release/1.87.0/source/boost_1_87_0.tar.gz -O /tmp/boost_1_87_0.tar.gz
tar xzvf /tmp/boost_1_87_0.tar.gz
cd boost_1_87_0
./bootstrap.sh
sudo ./b2 install link=static

# Install other dependencies
sudo apt install zlib1g protobuf-compiler libprotobuf-dev libssl-dev
```

### Build

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
make -C build/
```

The resulting binary should be approximately 16MB.

### Deployment

Upload the localproxy binary to S3 so it can be included in your component
deployment.
