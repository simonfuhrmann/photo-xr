# PhotoXR

**View your local VR photos and videos in your headset, served directly from
your own computer over your local network.**

PhotoXR is a lightweight web application that lets you browse and view locally
stored media in a VR headset through the headset's browser. It is designed for
simple, local-first usage:

- start the web server on your local computer and point it to a directory with
  VR media (both photos and videos are supported),
- open the browser on your VR headset and navigate to the web application
  provided by the web server.

## Overview

PhotoXR is designed to work entirely on your local network:

- A web server for serving locally hosted media files (photos, videos)
- A browser-based VR interface (WebXR) for immersive viewing in a headset

**Local media gallery:** Serve any file format supported by your headset's
browser from your computer serving a user-defined media root directory.

**Photo support:** The WebXR viewer expects all photos to be in side-by-side 180
degree equirectangular format. In addition, it supports Google's VR180 JPEG file
format where the right eye is stored in the JPEG's XMP metadata.

**Video support:** The WebXR viewer expects all videos to be in side-by-side 180
degree equirectangular format. The web server supports HTTP range requests for
efficient video streaming. (Currently, there is no support for fisheye. It seems
straightforward to support via fisheye texture mapping.) 

**UI interaction:** Interaction is limited: Browse the media directory structure
in the flat web application. Once immersive, navigate forward/backward using the
controller stick, and toggle video pause/play and mute/unmute via controller
buttons. (Caveat: Immersive UI, e.g., for video seeking, is currently not
implemented.)

## Architecture

PhotoXR follows a simple client-server model.

```
    VR Browser       <--- HTTPS --->     PhotoXR Server
   (WebXR + JS)                         (C++ web server)
                                               |
                                               v
                                      Local media directory
```

**Server:** The server is a simple, lightweight C++ application with minimal
dependencies that serves all files from a provided local media directory. It
also serves a simple web application (.html and .mjs files) to the client. Most
media files are served as-is, with one exception: JPEGs in Google's VR180 JPEG
file format are transparently re-encoded to side-by-side equirectangular images.

**Client:** The client is a web application loaded in the VR headset's browser.
It utilizes both a flat view to browse the media gallery, and immersive view
using WebXR to view photos and videos. Limited interaction in the immersive view
is provided.

**Communication:** Communication between client and server happens over HTTPS in
the browser. See the section below for details on secure context requirements in
WebXR. HTTP without TLS cannot be supported.

## Requirements

Client runtime
- A WebXR-capable browser (e.g., on Meta Quest devices)
- Local network connectivity between server and headset

Server runtime
- Linux (tested)
- Windows (untested, will need work, but written with portability in mind)
- MacOS (untested, will need work)

## Building and running

PhotoXR uses [Bazel](https://bazel.build/) as the build system. Bazel can
usually be installed via your Linux distribution. `libjpeg` is required to read
and interpret JPEG files. Ubuntu provides this dependency via the `libjpeg8-dev`
package.

```bash
sudo apt install bazel libjpeg8-dev
```

With the dependencies installed, the binary can be built as follows:

```bash
bazel build -c opt src:main
./bazel-bin/src/main /path/to/media
```

Or built and run in the same step:

```bash
bazel run -c opt src:main -- /path/to/media
```

By default, the server listens on port 8080. The web server is currently lacking
HTTPS support. For now, unfortunately, you need to run a HTTPS reverse proxy
(such as Caddy, read about secure context requirements below):

```bash
# Run from the photo-xr/ directory, next to Caddyfile.
sudo caddy run  
```

Then open the application from your VR headset browser.  Click 'Advanced' ->
'Proceed' to get past the self-signed certificate warning.

```
https://<your-machine-ip>
```

## Development

Client development requires to bundle the web application when the client code
changes. See `package.json` for details. This can be achieved with:

```bash
cd ./src/client
npm run bundle  # or "npm run watch" for continuous builds
```

The `Immersive Web Emulator` Chrome extension allows you to enter immersive view
on a regular Chrome browser, which usually does not support WebXR. This is
highly recommended for local development. It avoids constant switching to the
VR headset.

## OpenXR requires a secure context

A secure context is required for OpenXR to function. This means that the web
browsers will _generally_ refuse to start any WebXR application over a HTTP
connection (unless you're connecting to `localhost`). A HTTPS connection is
_required_. This can be queried in JavaScript via `window.isSecureContext`.

This is a privacy and security restriction aiming to prevent leaking of
sensitive data (sensor data about your real environment) over an unsecured
channel. There are two options to work with this restriction:

1. The web server itself supports TLS encryption (HTTPS) and issues a
   self-signed certificate. This is currently not implemented.
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
example, choose port :8443, and access the web app at https://<IP>:8443.

## Limitations

- No built-in HTTPS support yet (reverse proxy required)
- Controller input mapping is basic and may vary across devices
- No immersive UI yet (no video seeking)
- No thumbnails in the gallery view
- Tested only on Meta Quest 3S
- No Windows or MacOS support yet (only tested on Linux)

## License

This is a BSD 3-clause license. See `LICENSE` file.
