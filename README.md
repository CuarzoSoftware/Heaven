# ☁️ Heaven

Simple C library for exposing global menu bars through Unix Domain Sockets in Wayland and X11 sessions.

## 🧩 APIs

#### Heaven-Server

API for the global menu bar (process in charge of rendering the bar). Accepts menus from clients and choose which one to display with help from the Wayland or X11 compositor.

#### Heaven-Client

API for client applications which want to display their menus in the global menu bar. Allow applications to send their menus to the global menu bar, update them and listen to their actions events (e.g. when an user clicks on a menu item or types a shortcut).

#### Heaven-Compositor

API for Wayland or X11 compositors. Allows the compositor to notify the global menu bar which process is currently active using its own policy (e.g. when the user clicks on a window).

## 🔨 Build

Heaven only depends on standard Linux libraries (socket, poll, etc) found in almost every distribution. Can be built with meson or qmake:

```
$ cd Heaven
$ meson setup build
$ cd build
$ meson compile
$ sudo meson install
```

## 🕹️ Examples

Check the **server**, **client** and **compositor** examples found in `src/examples` to get familiar with the API.

## 📖 Documentation
##### [Link to the APIs documentation](https://ehopperdietzel.github.io/Heaven/ "Link to the API documentation")

> The documentation is still incomplete.

## ⚽ Objects

**hv_object**
* **hv_top_bar**: Represents an orderer group of menus (e.g. File, Edit, View, etc). The client can create multiple top bars (e.g. for different windows) but only one can be active at a time.
* **hv_menu**: A menu which can be assigned a label and contain multiple child items. Can be a child of a top bar or an action.
* **hv_item**
	* **hv_action**: A selectable item which can be assigned an icon, label and shortcuts. Child of a menu and can also contain a menu.
	* **hv_separator**: A horizontal line which can contain a label and be added to a menu to separate groups of actions.

All objects except for **hv_top_bar** can be child of other objects and only one object at the time.

## ⭐ Features

* Multiple top bars per client.
* Actions with icon (8-bit alpha only), label and shortcuts.
* Separators with label.
* Nested menus.
* Thread safe.

## Use case

1. The Wayland or X11 compositor starts.
2. The global menu process starts, gets displayed by the compositor and creates the global menu service using the **Heaven-Server** library.
3. The compositor connects to the global menu service using the **Heaven-Compositor** library (only one compositor can be connected at a time), identifies the global menu process by its pid and adds it to a keyboard events white list.
4. A client app starts and the compositor identifies it by its pid.
5. The client app connects to the global menu service and sends its menus using the **Heaven-Client**  library.
6. The global menu process identifies the client by its pid.
7. The compositor sends the pid of the current active client using its own policy (e.g. when the user clicks on a window). 
8. The global menu service shows the menu of the active client.


