Spotify tray
============

Adds a tray icon to the Spotify Linux client application. Only compatible with Xorg, does not work
in Wayland..

Features:
* Basic playback control through right-click menu
* Hiding the main client window ("minimize to tray")

XWayland
------------
In order to run Spotify tray under Wayland you will need to force X11 backend of GDK like so:
```sh
GDK_BACKEND=x11 spotify-tray
```

Installation
------------

* Clone this repository
* `./autogen.sh`
* `./configure`
* `make`
* Optionally `make install` will put the resulting binary to `/usr/local/bin`

Disclaimer
----------

This software is in no way official and has nothing to do with the Spotify Technology S.A. company.
Its only purpose is to add functionality missing in the official client.

Spotify is a registered trademark of Spotify AB.
