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

Deploy using ggl-cli:

```sh
/usr/local/bin/ggl-cli deploy \
  --recipe-dir components/recipes \
  --artifacts-dir components/artifacts \
  --add-component aws.greengrass.SecureTunneling=1.0.0
```

Verify deployment:

```sh
# Check nucleus logs for SUCCEEDED status
```

## Cloud Deployment

1. Use `recipe-all.yaml` (or `recipe.yaml` for architecture-specific builds) to
   create a private component
2. Create a new deployment in AWS IoT Greengrass
3. Deploy to target devices

See
[AWS documentation](https://docs.aws.amazon.com/greengrass/v2/developerguide/manage-deployments.html)
for detailed guidance.

## Monitoring

View component logs:

```sh
journalctl -f -u ggl.aws.greengrass.SecureTunneling.service
```
