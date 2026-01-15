# Deployment Guide

## Prerequisites

### AWS Permissions

The component requires IoT Core access for tunnel notifications. Add one of
these policies to your Greengrass device's role alias.

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

#### Production Policy (Least Privilege)

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

Replace `<REGION>` and `<ACCOUNT-ID>` with your values.

## Local Deployment

### Greengrass Nucleus

Deploy using greengrass-cli:

```sh
sudo /greengrass/v2/bin/greengrass-cli deployment create \
  --recipeDir components/recipes \
  --artifactDir components/artifacts \
  --merge "aws.greengrass.SecureTunneling=1.0.0"
```

Verify deployment:

```sh
tail -f /greengrass/v2/logs/greengrass.log
```

### Greengrass Nucleus Lite

Deploy using ggl-cli:

```sh
/usr/local/bin/ggl-cli deploy \
  --recipe-dir components/recipes \
  --artifacts-dir components/artifacts \
  --add-component aws.greengrass.SecureTunneling=1.0.0
```

Verify deployment:

```sh
journalctl -afu 'ggl.*'
```

## Cloud Deployment

1. Use `recipe-prod.yaml` (gdk-cli to create and publish component) to create a
   private component
2. Create a new deployment in AWS IoT Greengrass
3. Deploy to target devices

See
[AWS documentation](https://docs.aws.amazon.com/greengrass/v2/developerguide/manage-deployments.html)
for detailed guidance.

## Monitoring

### Greengrass Nucleus

View logs in the Greengrass root directory:

```sh
tail -f /greengrass/v2/logs/aws.greengrass.SecureTunneling.log
```

### Greengrass Nucleus Lite

Follow nucleus logs:

```sh
journalctl -afu 'ggl.*'
```

View component logs:

```sh
journalctl -f -u ggl.aws.greengrass.SecureTunneling.service
```
