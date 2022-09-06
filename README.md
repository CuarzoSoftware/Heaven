# Heaven
C library for exposing global menus in Wayland and X11 sessions using Unix Domain Sockets.

> :warning: **Under development !**

## Consist of 3 APIs / Libs

#### Heaven-Server

API for the global menu process. Accepts menus from clients and choose which one to display with help from the Wayland or X11 compositor.

#### Heaven-Client

API for client applications. Allow clients to expose their menu bars and associate them with a specific window (wl_surface id or X11 window id).

#### Heaven-Compositor

API for Wayland or X11 compositors. Allows the compositor to notify which window/process is currently active.

## Features

* Menu bars for specific windows.
* Menu bars shared with multiple windows.
* Menus reordering.
* Menu title.
* Menu items with icon, text and shortcuts.
* Menu separators with or without title.
* Nested menus.
