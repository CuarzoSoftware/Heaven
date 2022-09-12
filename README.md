# Heaven

C library for allowing global menu bars in Wayland and X11 sessions.

> :warning: **Under development !**

## APIs

#### Heaven-Server

API for the global menu (process in charge of rendering the global menu bar). Accepts menus from clients and choose which one to display with help of the Wayland or X11 compositor.

#### Heaven-Client

API for client applications which want to display their menus in the global menu bar. Allow applications to send their menus to the global menu process, update them and listen to their actions events (e.g. when an user clicks on a menu item or types a shortcut).

#### Heaven-Compositor

API for Wayland or X11 compositors. Allows the compositor to notify the global menu which process is currently active using its own policy  (e.g. when the user clicks on a window).

## Objects

**hv_object**
* **hv_top_bar**: Represents an orderer group of menus (e.g. File, Edit, View, etc). The client can create multiple topbars (e.g. for different windows) but only one can be active at a time.
* **hv_menu**: A menu which can be assigned a label and contain multiple child items. Can be a child of a top bar or an action.
* **hv_item**
	* **hv_action**: A selectable item which can be assigned an icon, label and shortcuts. Child of a menu and can also contain a menu.
	* **hv_separator**: A horizontal line which can contain a label and be added to a menu to separate groups of actions.

All objects except **hv_top_bar** can be child of other objects and only one object at the time.

## Features

* Multiple topbars per client.
* Actions with icons, label and shortcuts.
* Separators with label.
* Nested menus,

## Use case

1. The Wayland or X11 compositor starts.
2. The global menu process starts, gets displayed by the compositor and creates the global menu service using the **Heaven-Server** library.
3. The compositor connects to the global menu service using the **Heaven-Compositor** library (only one compositor can be connected at a time), identifies the global menu process by its pid and adds it to a keyboard events whitelist.
4. A client app starts and the compositor identifies it by its pid.
5. The client app connects to the global menu service and sends its menus using the **Heaven-Client**  library.
6. The global menu process identifies the client by its pid.
7. The compositor sends the pid of the current active client using its own policy (e.g. when the user clicks on a window). 
8. The global menu service shows the menu of the active client.

