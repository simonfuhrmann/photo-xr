# Photo XR

Photo XR is an web application for hosting local VR photos.

## OpenXR requires a secure context

A secure context is required for OpenXR to function. This means that the web
browsers will _generally_ refuse to start any WebXR application over a HTTP
connection (unless you're connecting to `localhost`). A HTTPS connection is
_required_. This can be queried in JavaScript via `window.isSecureContext`.

This is a privacy and security restriction aiming to prevent leaking of
sensitive data (sensor data about your real environment) over an unsecured
channel. There are two options to work with this restriction:

1. The web server itself supports TLS (HTTPS) and issues a self-signed
   certificate. This is currently not implemented.
2. The use of a reverse proxy, a web server that is capable of serving HTTPS,
   which proxies the request to the PhotoXR web server. This is recommended.

Both options use a self-signed certificate, and your browser will show a
security warning. Click 'Advanced' -> 'Proceed' to get past this warning.

In absence of the first option, `caddy` is recommended, and a `Caddyfile` is
provided as part of the repository. You can install `caddy` and disable the
`caddy` system service, if you choose to. Then start `caddy` adjacent to
`Caddyfile`.

```bash
sudo caddy run
``` 

By default, Caddy uses port :443. This is a privileged port and requires `sudo`.
If you prefer to run without root, you can change the port in `Caddyfile`. For
example, choose port :8443, and access the site at https://<IP>:8443.
