# GDK Deployment

This guide covers deploying the component using the Greengrass Development Kit
(GDK).

## Prerequisites

- [GDK CLI](https://github.com/aws-greengrass/aws-greengrass-gdk-cli) installed
- AWS credentials configured
- S3 bucket for component artifacts
- Built binaries (see [BUILD.md](BUILD.md))

Install GDK:

```sh
pip3 install git+https://github.com/aws-greengrass/aws-greengrass-gdk-cli.git@v1.6.0
```

## Configuration

Edit `gdk-config.json`:

```json
{
  "component": {
    "aws.greengrass.SecureTunneling": {
      "publish": {
        "bucket": "your-bucket-name",
        "region": "us-east-1"
      }
    }
  }
}
```

## Build

Build the component and create the artifact zip:

```sh
gdk component build
```

## Publish

Upload to S3 and create the component version:

```sh
gdk component publish
```

## Deploy

Create a deployment:

```sh
gdk component list  # Verify component is published
```

Then deploy via AWS Console or CLI:

```sh
aws greengrassv2 create-deployment \
  --target-arn "arn:aws:iot:REGION:ACCOUNT:thing/THING_NAME" \
  --components '{
    "aws.greengrass.SecureTunneling": {
      "componentVersion": "2.0.0"
    }
  }'
```
