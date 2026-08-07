#!/bin/bash
set -e

# localproxy revision to build. Defaults to main because the fetch dependency
# mode used below (-DLOCALPROXY_DEP_MODE=fetch) exists only on main -- no
# released tag, including the latest v3.3.0, defines that option, so pinning a
# tag would silently ignore the flag and require Boost 1.87 preinstalled on the
# host. Override with LOCALPROXY_REF=<tag-or-sha> to build a specific revision.
LOCALPROXY_REF="${LOCALPROXY_REF:-main}"
LOCALPROXY_REPO="${LOCALPROXY_REPO:-https://github.com/aws-samples/aws-iot-securetunneling-localproxy.git}"

# ---------------------------------------------------------------------------
# Preflight: verify required tools are available
# ---------------------------------------------------------------------------
MISSING_TOOLS=()
for tool in git cmake make zip jq; do
    command -v "$tool" >/dev/null 2>&1 || MISSING_TOOLS+=("$tool")
done
# Check for a C++ compiler (g++ or clang++)
if ! command -v g++ >/dev/null 2>&1 && ! command -v clang++ >/dev/null 2>&1; then
    MISSING_TOOLS+=("c++ compiler (g++ or clang++)")
fi
if [ ${#MISSING_TOOLS[@]} -ne 0 ]; then
    echo "Error: missing required tools: ${MISSING_TOOLS[*]}"
    echo "Install with: sudo apt-get install -y build-essential cmake git curl zip jq libssl-dev zlib1g-dev"
    exit 1
fi

# Get version
VERSION=$(cat version | tr -d '\n')

# ---------------------------------------------------------------------------
# Guard: component binary must already exist.
# It is built by the cmake+make step in gdk-config.json's custom_build_command,
# NOT by this script. If missing, the caller likely ran ./gdk-build.sh directly
# instead of `gdk component build`.
# ---------------------------------------------------------------------------
if [ ! -f build/bin/aws-greengrass-secure-tunnel ]; then
    echo "Error: build/bin/aws-greengrass-secure-tunnel not found."
    echo "This binary is produced by gdk-config.json's custom_build_command (cmake+make)."
    echo "Run 'gdk component build' instead of invoking this script directly."
    exit 1
fi

# ---------------------------------------------------------------------------
# Build or reuse localproxy
# ---------------------------------------------------------------------------
# Localproxy is cloned into a temp directory OUTSIDE this repo so that
# CMakeLists.txt's file(GLOB_RECURSE src/*.c) never picks up localproxy
# sources. An existing run/localproxy is reused to avoid a costly (~20 min)
# rebuild; set LOCALPROXY_FORCE_REBUILD=1 to rebuild anyway.
# ---------------------------------------------------------------------------
if [ -f run/localproxy ] && [ "${LOCALPROXY_FORCE_REBUILD:-0}" != "1" ]; then
    echo "Reusing existing run/localproxy (set LOCALPROXY_FORCE_REBUILD=1 to rebuild)"
else
    echo "Building localproxy ${LOCALPROXY_REF} from source..."

    LOCALPROXY_TMPDIR="$(mktemp -d)"
    # Clean up the temp clone on exit regardless of success/failure.
    # Because set -e is active, a build failure still propagates as non-zero
    # exit — the trap runs cleanup but does not mask the error code.
    trap 'rm -rf "$LOCALPROXY_TMPDIR"' EXIT

    # Cloned with full history rather than --depth 1: localproxy derives its
    # reported version from `git describe --tags`, so a shallow clone carries no
    # tags and the binary reports v0.0.0-unknown. Full history also lets
    # LOCALPROXY_REF be a branch, tag or commit SHA (--branch accepts only the
    # first two).
    git clone -q "$LOCALPROXY_REPO" "$LOCALPROXY_TMPDIR/localproxy"
    git -C "$LOCALPROXY_TMPDIR/localproxy" checkout -q "$LOCALPROXY_REF"
    LOCALPROXY_RESOLVED="$(git -C "$LOCALPROXY_TMPDIR/localproxy" describe --tags 2>/dev/null \
        || git -C "$LOCALPROXY_TMPDIR/localproxy" rev-parse --short HEAD)"
    echo "localproxy ref $LOCALPROXY_REF resolved to $LOCALPROXY_RESOLVED"

    mkdir -p "$LOCALPROXY_TMPDIR/localproxy/build"
    # LOCALPROXY_DEP_MODE=fetch tells localproxy's CMake to use FetchContent for
    # Boost and Protobuf, so only OpenSSL and zlib are required from the system.
    cmake -S "$LOCALPROXY_TMPDIR/localproxy" -B "$LOCALPROXY_TMPDIR/localproxy/build" \
        -DLINK_STATIC_OPENSSL=OFF \
        -DLOCALPROXY_DEP_MODE=fetch
    make -C "$LOCALPROXY_TMPDIR/localproxy/build" -j"$(nproc)"

    mkdir -p run
    strip "$LOCALPROXY_TMPDIR/localproxy/build/bin/localproxy"
    cp "$LOCALPROXY_TMPDIR/localproxy/build/bin/localproxy" run/localproxy
    chmod +x run/localproxy

    echo "localproxy ${LOCALPROXY_REF} installed to run/localproxy"
fi

# Create artifact directory
mkdir -p greengrass-build/artifacts/aws.greengrass.SecureTunneling/NEXT_PATCH

# Copy binaries
cp build/bin/aws-greengrass-secure-tunnel greengrass-build/artifacts/aws.greengrass.SecureTunneling/NEXT_PATCH/
cp run/localproxy greengrass-build/artifacts/aws.greengrass.SecureTunneling/NEXT_PATCH/

# Create zip
cd greengrass-build/artifacts/aws.greengrass.SecureTunneling/NEXT_PATCH
zip aws.greengrass.SecureTunneling.zip aws-greengrass-secure-tunnel localproxy
rm aws-greengrass-secure-tunnel localproxy
cd ../../../..

# Log bundle contents
echo "localproxy ref: ${LOCALPROXY_REF}${LOCALPROXY_RESOLVED:+ (${LOCALPROXY_RESOLVED})}"
unzip -l greengrass-build/artifacts/aws.greengrass.SecureTunneling/NEXT_PATCH/aws.greengrass.SecureTunneling.zip

# Generate recipe
sed -e "s/{COMPONENT_NAME}/aws.greengrass.SecureTunneling/g" \
    -e "s/{COMPONENT_VERSION}/$VERSION/g" \
    -e "s|BUCKET_NAME|$(jq -r '.component."aws.greengrass.SecureTunneling".publish.bucket' gdk-config.json)|g" \
    -e "s|COMPONENT_NAME|aws.greengrass.SecureTunneling|g" \
    -e "s|COMPONENT_VERSION|$VERSION|g" \
    recipe.yaml > greengrass-build/recipes/recipe.yaml

echo "Build complete: greengrass-build/artifacts/aws.greengrass.SecureTunneling/NEXT_PATCH/aws.greengrass.SecureTunneling.zip"
