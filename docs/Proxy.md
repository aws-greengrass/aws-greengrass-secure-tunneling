# Running Behind a Web Proxy

The `aws.greengrass.SecureTunneling` component does not take any proxy
configuration of its own. It inherits the network proxy settings that the
Greengrass nucleus is configured with — the component environment is formatted
automatically by Greengrass, so no recipe changes are required.

This document explains how to configure Greengrass so that the Secure Tunneling
component connects to the AWS IoT Secure Tunneling service through your
corporate web proxy.

> **⚠️ Known limitation — SSH login from the IoT Core Console**
>
> When the device is behind a web proxy, the browser-based SSH client in the AWS
> IoT Core Console only works with password-based login.
>
> Key-based SSH login from the console is not supported in this setup. If you
> need key-based auth, run
> [`aws-iot-securetunneling-localproxy`](https://github.com/aws-samples/aws-iot-securetunneling-localproxy)
> in source mode on your workstation and point your SSH client at the local port
> it exposes. See [usage.md](usage.md#aws-cli-alternative) for the commands.

## How It Works

1. You configure the `networkProxy` object on the `aws.greengrass.Nucleus`
   component (at install time or via a later deployment).
2. The nucleus exports `ALL_PROXY`, `HTTP_PROXY`, `HTTPS_PROXY`, and `NO_PROXY`
   to every component process it spawns. See the [AWS IoT Greengrass V2
   developer guide][gg-proxy] for details.
3. The Secure Tunneling component picks up those variables automatically and
   tunnels its WebSocket connection through the web proxy using `HTTP CONNECT`.

```
┌────────────────────────────────────────────────────────────────────┐
│                       Greengrass Core Device                       │
│                                                                    │
│  aws.greengrass.Nucleus                                            │
│   └─ networkProxy.proxy.url  ──►  HTTPS_PROXY / HTTP_PROXY / ...   │
│                                    (exported to component env)     │
│                                             │                      │
│  aws.greengrass.SecureTunneling  ◄──────────┘                      │
│                           │                                        │
└───────────────────────────┼────────────────────────────────────────┘
                            │ HTTP CONNECT
                            ▼
                 ┌────────────────────┐          ┌───────────────────┐
                 │   Corporate Web    │  WSS     │  AWS IoT Secure   │
                 │       Proxy        │ ───────► │  Tunneling (:443) │
                 └────────────────────┘          └───────────────────┘
```

## Prerequisites

- Greengrass nucleus v2.5.0 or later, or Greengrass nucleus lite v2.1.0 or
  later.
- A web proxy that allows the HTTP `CONNECT` method to the AWS IoT Secure
  Tunneling device connection endpoints
  (`data.tunneling.iot.<region>.amazonaws.com:443`).
- If using an HTTPS proxy with a private CA, the proxy's CA certificate must be
  appended to the core device's Amazon root CA file so the nucleus trusts it.

## Configure the Network Proxy on the Nucleus

Deploy the following configuration update to the `aws.greengrass.Nucleus`
component. The nucleus restarts when `networkProxy` is updated.

### HTTP proxy, no authentication

```json
{
  "networkProxy": {
    "noProxyAddresses": "localhost,127.0.0.1",
    "proxy": {
      "url": "http://proxy.example.com:3128"
    }
  }
}
```

### HTTP or HTTPS proxy with basic authentication

Use `http://` or `https://` in the `url` depending on the proxy's scheme. HTTPS
proxies require nucleus v2.5.0+ or nucleus lite v2.1.0+, and the proxy server CA
must be trusted by the device (see next section).

Credentials can be provided either inside the URL or as separate fields. If both
are supplied, the URL's `userinfo` wins.

```json
{
  "networkProxy": {
    "noProxyAddresses": "localhost,127.0.0.1",
    "proxy": {
      "url": "https://proxy.example.com:3128",
      "username": "gg_user",
      "password": "gg_password"
    }
  }
}
```

Example deployment payload (serialized `merge` update as required by
Greengrass):

```json
{
  "components": {
    "aws.greengrass.Nucleus": {
      "configurationUpdate": {
        "merge": "{\"networkProxy\":{\"noProxyAddresses\":\"localhost,127.0.0.1\",\"proxy\":{\"url\":\"https://proxy.example.com:3128\",\"username\":\"gg_user\",\"password\":\"gg_password\"}}}"
      }
    }
  }
}
```

You can also supply the same configuration at install time with the
`--init-config` installer argument.

### Trust the HTTPS proxy CA (HTTPS proxies only)

Append the proxy server's CA certificate chain to the Amazon root CA file that
the nucleus uses. The path is defined by `system.rootCaPath` in
`/greengrass/v2/config/effectiveConfig.yaml` (typically `rootCA.pem` under the
Greengrass root).

```bash
sudo cat proxy-ca.pem >> /greengrass/v2/rootCA.pem
```

Without this step, TLS verification fails when connecting through the HTTPS
proxy.

## Required Permissions and Device Role

### Greengrass nucleus

When the classic nucleus connects through a proxy, MQTT client authentication
can no longer rely on the device certificate alone on older nuclei. If you run
nucleus < v2.4.0, add the following actions to your token exchange role alias
policy:

- `iot:Connect`
- `iot:Publish`
- `iot:Receive`
- `iot:Subscribe`

For nucleus v2.4.0+, the standard device role is sufficient.

### Greengrass nucleus lite

Nucleus lite keeps certificate-based MQTT authentication even when a proxy is in
use (the device certificate is presented directly to AWS IoT through the proxy
tunnel), so no extra `iot:*` actions need to be added to the token exchange role
alias. The existing role alias from your install is enough.

### Both runtimes

Tunnel notifications still require the subscribe permission documented in
[deployment.md](deployment.md#prerequisites).

## Firewall and Endpoint Allow-Listing

Your web proxy must permit outbound TLS/`CONNECT` to:

| Purpose               | Endpoint                                                | Port |
| --------------------- | ------------------------------------------------------- | ---- |
| Secure tunneling data | `data.tunneling.iot.<region>.amazonaws.com`             | 443  |
| IoT Core (MQTT/HTTPS) | `<prefix>-ats.iot.<region>.amazonaws.com`               | 443  |
| Greengrass data plane | `greengrass-ats.iot.<region>.amazonaws.com` (if in use) | 443  |

Replace `<region>` with your deployment region. The component issues an HTTP
`CONNECT` to `data.tunneling.iot.<region>.amazonaws.com:443`; the proxy must be
configured to allow this host.

## Environment Variables Recognized

The component consumes only the variables that the nucleus sets for it. No
per-component overrides are defined.

| Variable      | Source                                     |
| ------------- | ------------------------------------------ |
| `HTTPS_PROXY` | Nucleus, when `networkProxy` is configured |
| `HTTP_PROXY`  | Nucleus, when `networkProxy` is configured |
| `ALL_PROXY`   | Nucleus, when `networkProxy` is configured |
| `NO_PROXY`    | Nucleus, when `networkProxy` is configured |

Notes:

- Greengrass exports both uppercase and lowercase forms, so no additional casing
  handling is required.
- If `networkProxy` is not set on the nucleus, no proxy variables are exported
  and the component connects directly.

## Verification

1. Deploy or restart `aws.greengrass.SecureTunneling` after configuring the
   nucleus proxy.
2. Open a tunnel for the device (see [usage.md](usage.md)).
3. Inspect the component log and confirm the tunnel establishes:
   - Greengrass Nucleus:
     ```
     tail -f /greengrass/v2/logs/aws.greengrass.SecureTunneling.log
     ```
   - Greengrass Nucleus Lite:
     ```
     journalctl -f -u ggl.aws.greengrass.SecureTunneling.service
     ```

   A successful run logs messages such as
   `Started tunnel worker for service: SSH` followed by
   `Tunnel completed successfully` when the session closes.

## Troubleshooting

| Symptom                                           | Likely Cause                                                             | Fix                                                                                                                             |
| ------------------------------------------------- | ------------------------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------- |
| `Could not connect to proxy` / connection refused | Wrong proxy URL, port, or proxy unreachable from the device              | Verify from the core device: `curl -I https://aws.amazon.com -x $HTTPS_PROXY`                                                   |
| `407 Proxy Authentication Required`               | Missing or incorrect credentials                                         | Set `username`/`password` in `networkProxy.proxy` or embed in the URL                                                           |
| `Method not allowed` on `CONNECT`                 | Proxy blocks the `CONNECT` verb or disallows 443 to the tunnel endpoint  | Allow `CONNECT` to `data.tunneling.iot.<region>.amazonaws.com:443`                                                              |
| TLS handshake fails to the HTTPS proxy            | Proxy CA not trusted by the device                                       | Append proxy CA to the nucleus `rootCaPath` file (see [Trust the HTTPS proxy CA](#trust-the-https-proxy-ca-https-proxies-only)) |
| Nucleus connects fine, tunnels still fail         | `NO_PROXY` contains the tunneling endpoint, bypassing the required proxy | Remove the tunneling endpoint from `noProxyAddresses`                                                                           |
| Works from shell but not from the component       | Proxy vars exported only for the login shell, not to Greengrass          | Configure `networkProxy` on the nucleus instead of exporting in `~/.bashrc` or similar                                          |

## References

- [Configure the AWS IoT Greengrass Core software — network proxy][gg-proxy]
- [Configure local proxy for devices that use web proxy][iot-local-proxy-web]

[gg-proxy]:
  https://docs.aws.amazon.com/greengrass/v2/developerguide/configure-greengrass-core-v2.html#configure-alpn-network-proxy
[iot-local-proxy-web]:
  https://docs.aws.amazon.com/iot/latest/developerguide/configure-local-proxy-web-proxy.html
