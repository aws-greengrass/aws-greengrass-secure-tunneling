# aws.greengrass.SecureTunneling

A Greengrass component that enables secure tunneling to IoT devices using AWS
IoT Device Management Secure Tunneling service.

This component listens for tunnel notifications and automatically establishes
secure tunnels using the localproxy client, supporting services like SSH, HTTP,
HTTPS, RDP, and VNC.

### Build Prerequisites

This repo depends on a prebuilt binary of local proxy that can be downloaded
from
[here](https://github.com/aws-samples/aws-iot-securetunneling-localproxy/tree/main).

Currently it's only been tested with
[feb59e2 main commit](https://github.com/aws-samples/aws-iot-securetunneling-localproxy/commit/feb59e268c8f4f1c7450f3a510963e84cc397ac7).

It's recommended to build the binary statically with no tests. You can follow
the build guide from the repo provided
[README.md](https://github.com/aws-samples/aws-iot-securetunneling-localproxy/blob/feb59e268c8f4f1c7450f3a510963e84cc397ac7/README.md).
There is a good amount of information with options to cross-compile and more.

However, below are the short getting started commands that you can run to get
started. The following commands assume that you are building on the same system
architecture as the target device.

```
sudo apt update

# Install boost 1.87
wget https://archives.boost.io/release/1.87.0/source/boost_1_87_0.tar.gz -O /tmp/boost_1_87_0.tar.gz
tar xzvf /tmp/boost_1_87_0.tar.gz
cd boost_1_87_0
./bootstrap.sh
sudo ./b2 install link=static

# Install zlib1g
sudo apt install zlib1g

# Install protobuf
sudo apt install protobuf-compiler libprotobuf-dev

# Install openssl
sudo apt install libssl-dev

# Install local proxy with release build
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
make -C build/
```

The resulting binary should be roughly about 16MB. Remember to move it to your
S3 bucket so that it can be passed on through deployment.

### Build

To build the project, you will need the following build dependencies:

- GCC or Clang
- CMake ( >= version 3.22)
- Make or Ninja
- pkg-config
- git
- libssl-dev
- libpthread
- Pre-built
  [local-proxy (main)](https://github.com/aws-samples/aws-iot-securetunneling-localproxy/tree/main)
  binary.

On Ubuntu, these can be installed with:

```sh
sudo apt update && sudo apt install build-essential pkg-config cmake git libssl-dev
```

To make a release build configured for minimal size, run:

```sh
cmake -B build -D CMAKE_BUILD_TYPE=MinSizeRel
```

The following configuration flags may be set with cmake (with `-D`):

- `CMAKE_BUILD_TYPE`: This can be set to `MinSizeRel`, `Debug`, `Release` or
  `RelWithDebInfo` for different optimizations

- `ENABLE_WERROR`: Set to `ON` to treat warnings as errors

To build, then run `make`:

```sh
make -C build -j$(nproc)
```

Alternatively, use the provided build script:

```sh
./build.sh
```

### Component Creation

To deploy this component to Greengrass, you need to create a directory hierarchy
as below:

```
components
├── artifacts
│   └── aws.greengrass.SecureTunneling
│       └── 1.0.0
│           ├── secure-tunnel
│           └── localproxy
└── recipes
    └── aws.greengrass.SecureTunneling-1.0.0.yaml
```

The component's recipe is at the root of the directory, while the binary
`secure-tunnel` is in the `./build/bin` folder after successfully building the
project. You'll also need the `localproxy` binary from AWS IoT Secure Tunneling.

You may also build and run this binary outside of Greengrass independently:

```sh
# To get usage help
./build/bin/secure-tunnel --help
```

### Cloud Deployment Prerequisites

Before deploying this component, you should set up the cloud infrastructure and
permissions.

The component requires access to IoT Core for receiving tunnel notifications and
must have the localproxy binary available. You need to provide the following
additional policy to your Greengrass device's role alias at a minimum for the
component to work.

#### Development/Testing Policy (Permissive)

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": ["iot:Subscribe", "iot:Receive"],
      "Resource": "*"
    }
  ]
}
```

#### Least Privilege Policy

For production deployments, use this least privilege policy:

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": ["iot:Subscribe", "iot:Receive"],
      "Resource": [
        "arn:aws:iot:<REGION>:<ACCOUNT-ID>:topicfilter/$aws/things/*/tunnels/notify"
      ]
    }
  ]
}
```

Replace `<REGION>` with your AWS region and `<ACCOUNT-ID>` with your AWS account
ID.

### Configuration

The component supports the following configuration parameters:

- `maxConcurrentTunnels`: Maximum number of concurrent tunnels (default: 20)
- `tunnelTimeoutSeconds`: Tunnel timeout in seconds (default: 43200 - 12 hours)

### Supported Services

The component automatically maps services to ports:

- SSH: port 22
- HTTP: port 80
- HTTPS: port 443

### Local Deployment

Run from your install directory:

```sh
/usr/local/bin/ggl-cli deploy --recipe-dir components/recipes --artifacts-dir components/artifacts --add-component aws.greengrass.SecureTunneling=1.0.0
```

Check the nucleus logs to verify that the deployment has SUCCEEDED.

### Cloud Deployment

Use the `recipe-all.yaml` provided at the root to create your private component.
If you have different architectures, you can use `recipe.yaml` to fine-tune the
binaries as needed. Create a new deployment and deploy it to devices. More
guidance can be found
[here](https://docs.aws.amazon.com/greengrass/v2/developerguide/manage-deployments.html).

### Check the component logs

After the deployment completes, read the logs from the component:

```sh
journalctl -f -u ggl.aws.greengrass.SecureTunneling.service
```

## Usage

Once the component is successfully deployed, navigate to Secure tunneling (IoT
Core feature) from the AWS console. You can also follow the
[us-east-1 link here](https://us-east-1.console.aws.amazon.com/iot/home?region=us-east-1#/tunnelhub).
Then follow the following steps:

- Click on `Create tunnel`
- Select `Manual Setup`
- Click on `Next`
- Click on `Add new service`
- Type `SSH`
- Select your device's Thing Name
- Click `Next`
- Click `Confirm and create`
- You do not need to download anything, click `Close`
- Once you are redirected to the new page, click `Connect` [MUST NOT REFRESH or
  leave the page]
- Provide the username and auth method
- Click `Connect`
- Wait for a bit and you should be able to access your device from the web UI.
