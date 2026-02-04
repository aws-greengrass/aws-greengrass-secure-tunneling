# Usage Guide

## Creating a Secure Tunnel

Once deployed, create tunnels via the AWS Console:

1. Navigate to
   [Secure Tunneling](https://us-east-1.console.aws.amazon.com/iot/home?region=us-east-1#/tunnelhub)
   in IoT Core (Must be pre-logged into AWS console to use link)
2. Click **Create tunnel**
3. Select **Manual Setup**
4. Click **Next**
5. Click **Add new service**
6. Enter service name (e.g., `SSH`)
7. Select your device's Thing Name
8. Click **Next**
9. Click **Confirm and create**
10. Click **Close** (no download needed)

## Connecting to the Tunnel

1. On the tunnel page, click **Connect** (do not refresh or leave the page)
2. Provide username and authentication method
3. Click **Connect**
4. Wait for connection to establish
5. Access your device through the web UI

## AWS CLI Alternative

Create a tunnel:

```bash
aws iotsecuretunneling open-tunnel \
    --destination-config thingName=<YOUR_THING_NAME>,services=SSH
```

List tunnels:

```bash
aws iotsecuretunneling list-tunnels
```

Describe a tunnel:

```bash
aws iotsecuretunneling describe-tunnel --tunnel-id <TUNNEL_ID>
```

Close a tunnel:

```bash
aws iotsecuretunneling close-tunnel --tunnel-id <TUNNEL_ID>
```

Connect using localproxy with the source access token:

```bash
./localproxy -r <REGION> -s <SOURCE_ACCESS_TOKEN> -d localhost:22
```

## Additional Resources

- [AWS Secure Tunneling Tutorial](https://docs.aws.amazon.com/iot/latest/developerguide/tunneling-tutorial-quick-setup.html)
