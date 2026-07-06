# ☁️ Heaven

**C++ global-menu library for Wayland sessions.**

Heaven lets Wayland clients export their application menus (à la macOS) to a
single *global menu* application, and lets the compositor decide whose menu is
shown. All inter-process communication happens over the **D-Bus session bus**;
the objects and their properties are mirrored on both ends.

The project builds three independent libraries on top of
[`cz-core`](https://github.com/CuarzoSoftware):

| Library | pkg-config | Used by | Role |
|---|---|---|---|
| **cz-heaven-bar** | `cz-heaven-bar` | the global-menu application | Receives and mirrors the menus of every client; delivers clicks back. |
| **cz-heaven-client** | `cz-heaven-client` | ordinary Wayland apps | Publishes the app's menu tree and reacts to clicks. |
| **cz-heaven-compositor** | `cz-heaven-compositor` | the Wayland compositor | Authenticates clients and tells the bar which one is active. |

Everything lives under the `CZ::Bar`, `CZ::Client` and `CZ::Compositor`
namespaces respectively.

---

## Architecture

```
                    ┌──────────────────────┐
                    │   Compositor (comp)   │
                    │  cz-heaven-compositor │
                    └──────────┬───────────┘
          RegisterClient(token)│  ▲          │ SetActiveClient(dbusId)
                               │  │          ▼
        ┌───────────────┐      │  │      ┌───────────────────────┐
        │    Client     │──────┘  └──────│         Bar           │
        │ cz-heaven-    │  RegisterClient │   cz-heaven-bar       │
        │   client      │  CreateObject   │  (global menu app)    │
        │               │  SetObject*     │                       │
        │               │  Commit …       │                       │
        │               │◀────────────────│  ObjectClicked(id)    │
        └───────────────┘                 └───────────────────────┘
```

* The **compositor** hands each client a private token through a Wayland
  protocol (out of scope for this project). The client sends that token back
  over D-Bus so the compositor can map the client's `wl_client` to its D-Bus
  unique name. The compositor then tells the bar which D-Bus id is active
  (or an empty string when none is).
* The **client** describes its menu as a tree of objects and pushes the changes
  to the bar. The bar buffers every change and only applies them — notifying
  its user — when the client calls `commit()`.
* The **bar** renders the active client's menu and calls `HNObject::click()`
  when the user activates an item; the click is delivered back to the owning
  client.

### Active top bar

The compositor decides which *client* is active. The client itself decides
which of *its own* top bars should be displayed (e.g. the one belonging to the
focused window) via `HNClient::setActiveTopbar()`.

---

## Object model

A menu is a tree of objects. Each object has an immutable role:

| Object | Can host children | Properties |
|---|---|---|
| **Topbar**  | yes (menus only) | — |
| **Menu**    | yes | title, icon, shortcut, enabled |
| **Action**  | no  | title, icon, shortcut, enabled |
| **Toggle**  | no  | title, icon, shortcut, enabled, checked |
| **Divider** | no  | title |

Properties are provided through the `HNWith*` mixin interfaces
(`HNWithTitle`, `HNWithIcon`, `HNWithShortcut`, `HNWithEnabled`,
`HNWithParent`, `HNWithChildren`). Only **menus** may be nested inside a
**topbar**; cycles are rejected.

---

## The commit model

On the client side nothing is sent until the first `commit()`. This lets an
application build its whole menu atomically. After the first commit, individual
property changes are streamed to the bar as they happen, and each subsequent
`commit()` tells the bar to apply the batch.

The bar stores every change in a per-client buffer and processes the buffer
(emitting its signals) only when it receives a `Commit`.

### Reconnection

If the bar disappears and later comes back, the client automatically
re-registers and **re-sends its entire state** — every object, all properties
and the full parent/child hierarchy — while keeping the client-side object
memory intact. Object ids are only reused after the bar acknowledges their
destruction.

---

## D-Bus interface

All names are on the session bus.

**`org.cuarzo.HeavenBar`** — `/org/cuarzo/HeavenBar`

| Method | Signature | Caller |
|---|---|---|
| `SetActiveClient` | `s → b` | compositor |
| `RegisterClient` | `→ b` | client |
| `SetClientName` | `s` | client |
| `SetClientTopbar` | `u` | client |
| `CreateObject` | `uu` (id, type) | client |
| `DestroyObject` | `u → u` (id, acked id) | client |
| `SetObjectTitle` / `SetObjectIcon` / `SetObjectShortcut` | `us` | client |
| `SetObjectEnabled` / `SetToggleChecked` | `ub` | client |
| `SetObjectParent` | `uu` (id, parent id; 0 = detach) | client |
| `InsertObjectBefore` | `uu` (id, sibling id; 0 = append) | client |
| `Commit` | — | client |

**`org.cuarzo.HeavenCompositor`** — `/org/cuarzo/HeavenCompositor`

| Method | Signature | Caller |
|---|---|---|
| `RegisterClient` | `s` (token) | client |

**`org.cuarzo.HeavenClient`** — `/org/cuarzo/HeavenClient`

| Method | Signature | Caller |
|---|---|---|
| `ObjectClicked` | `u` (object id) | bar |

Presence of each peer is tracked with `NameOwnerChanged` matches, which is what
drives the reconnection logic.

---

## Building

Requires a C++20 compiler, Meson, and the `cz-core` and `libsystemd`
(or `elogind`/`basu`) development packages.

```sh
meson setup builddir
ninja -C builddir
sudo ninja -C builddir install
```

This produces the three shared libraries, their pkg-config files, and the
example programs under `builddir/examples/`.

---

## Usage

A `CZCore` instance must exist before creating any Heaven object, and the
program must run its event loop (`core->dispatch()`).

### Client

```cpp
auto core   = CZCore::GetOrMake();
auto client = CZ::Client::HNClient::GetOrMake();

auto topbar = CZ::Client::HNTopbar::Make();
auto file   = CZ::Client::HNMenu::Make("File", "", "", true, topbar.get());
auto quit   = CZ::Client::HNAction::Make("Quit", "application-exit", "Ctrl+Q", true, file.get());

quit->onClicked.subscribe(quit.get(), [](auto*){ /* quit the app */ });

client->setName("My App");
client->setActiveTopbar(topbar.get());
client->setPrivateHandle(handleFromCompositor); // received via a Wayland protocol
client->commit();

while (core->dispatch() >= 0) {}
```

Keep the returned `shared_ptr`s alive: destroying the last reference removes the
object (and notifies the bar).

### Bar

```cpp
auto bar = CZ::Bar::HNBar::GetOrMake();

bar->onObjectCreated.subscribe(bar.get(), [](CZ::Bar::HNObject *o){ /* … */ });
bar->onActiveClientChanged.subscribe(bar.get(), [](CZ::Bar::HNBar *b){ /* redraw */ });
// call obj->click() when the user activates an item
```

### Compositor

```cpp
auto comp = CZ::Compositor::HNCompositor::GetOrMake();

comp->onClientRegistered.subscribe(comp.get(),
    [comp = comp.get()](const char *handle, const char *dbusId){
        // map `handle` to the matching wl_client, then:
        comp->setActiveClient(dbusId);
    });
```

Complete, runnable versions of the three programs live in
[`examples/`](examples/). To try them on an isolated bus:

```sh
export LD_LIBRARY_PATH=$PWD/builddir:/usr/local/lib64
dbus-run-session -- sh -c '
  builddir/examples/compositor/cz-heaven-compositor-example &
  builddir/examples/bar/cz-heaven-bar-example &
  builddir/examples/client/cz-heaven-client-example'
```

Set `CZ_HEAVEN_{BAR,CLIENT,COMPOSITOR}_LOG_LEVEL` (0–6) to control logging.

---

## API documentation

Every public class and method is documented with Doxygen. Generate the HTML
reference (output goes to `docs/`) with:

```sh
cd doxygen
doxygen Doxyfile
```

---

## License

See [LICENSE](LICENSE).
