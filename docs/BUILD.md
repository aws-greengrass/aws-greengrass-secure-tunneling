# Building the Component

## Prerequisites

### System Dependencies

- GCC or Clang
- CMake (>= version 3.22)
- Make or Ninja
- pkg-config
- git
- libssl-dev
- libpthread

On Ubuntu:

```sh
sudo apt update && sudo apt install build-essential pkg-config cmake git libssl-dev
```

### Local Proxy Binary

You need a prebuilt localproxy binary. See [localproxy.md](localproxy.md) for
build instructions.

## Build Steps

### Configure

```sh
cmake -B build -D CMAKE_BUILD_TYPE=MinSizeRel
```

Configuration options:

- `CMAKE_BUILD_TYPE`: `MinSizeRel`, `Debug`, `Release`, or `RelWithDebInfo`
- `ENABLE_WERROR`: Set to `ON` to treat warnings as errors

### Compile

```sh
make -C build -j$(nproc)
```

Or use the build script:

```sh
./build.sh
```

### Test Standalone

```sh
./build/bin/secure-tunnel --help
```

## Component Structure

### For local deployment

For Greengrass Local deployment, create this directory hierarchy:

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

### For cloud deployment

For Greengrass Cloud deployment, create a zip file with this structure:

```
aws.greengrass.SecureTunneling-1.0.0.zip
├── secure-tunnel
└── localproxy
```

- `secure-tunnel`: Built binary from `./build/bin`
- `localproxy`: Binary from [localproxy.md](localproxy.md)
- Recipe: Component configuration file
