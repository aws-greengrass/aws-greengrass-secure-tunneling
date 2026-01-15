#!/bin/bash
set -e

# Get version
VERSION=$(cat version | tr -d '\n')

# Check for localproxy
if [ ! -f run/localproxy ]; then
    echo "Error: run/localproxy not found. Please build localproxy first."
    exit 1
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

# Generate recipe
sed -e "s/{COMPONENT_NAME}/aws.greengrass.SecureTunneling/g" \
    -e "s/{COMPONENT_VERSION}/$VERSION/g" \
    -e "s|BUCKET_NAME|$(jq -r '.component."aws.greengrass.SecureTunneling".publish.bucket' gdk-config.json)|g" \
    -e "s|COMPONENT_NAME|aws.greengrass.SecureTunneling|g" \
    -e "s|COMPONENT_VERSION|$VERSION|g" \
    recipe.yaml > greengrass-build/recipes/recipe.yaml

echo "Build complete: greengrass-build/artifacts/aws.greengrass.SecureTunneling/NEXT_PATCH/aws.greengrass.SecureTunneling.zip"
