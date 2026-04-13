# GDK Deployment

This guide covers deploying the component using the Greengrass Development Kit
(GDK).

## Prerequisites

- [GDK CLI](https://github.com/aws-greengrass/aws-greengrass-gdk-cli)
  - Follow Install GDK step below if not installed already.
- AWS credentials configured
  - Easiest is to export a permissive role's token
- S3 bucket for component artifacts
- Built localproxy binary (see [localproxy.md](localproxy.md))

Install GDK:

```sh
pip3 install git+https://github.com/aws-greengrass/aws-greengrass-gdk-cli.git@v1.6.0
```

Exporting AWS Credentials

```shell
export AWS_ACCESS_KEY_ID=<AWS_ACCESS_KEY_ID>
export AWS_SECRET_ACCESS_KEY=<AWS_SECRET_ACCESS_KEY>
export AWS_SESSION_TOKEN=<AWS_SESSION_TOKEN>
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

To use a custom S3 bucket, specify it with the `--bucket` option:

```sh
gdk component publish --bucket your-bucket-name
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
