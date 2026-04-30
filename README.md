# libWinpos

Client-side window positioning hint library for Qt applications.

On Wayland, compositors do not expose a global coordinate space to clients, so
the standard `QWindow::setPosition()` / `QWidget::move()` calls have no effect.

**libwinpos** implements the
[xx-zones-v1](https://gitlab.freedesktop.org/wayland/wayland-protocols/-/blob/main/experimental/xx-zones/xx-zones-v1.xml)
Wayland protocol to let applications request where their windows should be placed within a compositor-provided zone.
On X11 and other platforms it falls back to Qt's native positioning APIs, so the same code works on every platform,
Windows, macOS, Linux/X11 and Linux/Wayland, as long as the compositor supports the protocol.

Currently, only specialized compositors support the `xx-zones` protocol, but you can add support
for KDE's KWin by installing the [kwin-zones](https://invent.kde.org/automotive/kwin-zones) plugin.

## API

All types live in the `Winpos` namespace.

### High-level API

The high-level `WindowPositioner` helper makes window positioning work on all platforms.
Use it like this:

```cpp
// In QWidget::showEvent() or after the QWindow is visible:
auto pos = new Winpos::WindowPositioner(windowHandle(), this);

// Receive position updates from the compositor.
connect(pos, &Winpos::WindowPositioner::positionChanged, this, [pos] {
    qDebug() << "window at" << pos->position() << "zone size" << pos->zoneSize();
});
connect(pos, &Winpos::WindowPositioner::zoneSizeChanged, this, [pos] {
    qDebug() << "zone resized to" << pos->zoneSize();
});

// Request a position (zone-relative frame coordinates on Wayland,
// screen-available-area-relative on X11).
pos->move(100, 50);

// Save and restore geometry across sessions.
QRect saved = pos->geometry();       // position + window size
pos->setGeometry(saved);             // restores both position and size
```

### Low-level API

For applications that need full protocol access, wrapper classes for the zones
protocol are provided:

| Class              | Wraps                | Purpose                                              |
|--------------------|----------------------|------------------------------------------------------|
| `Winpos::Manager`  | `xx_zone_manager_v1` | Process-wide singleton; creates zones and zone items |
| `Winpos::Zone`     | `xx_zone_v1`         | A display area with its own coordinate space         |
| `Winpos::ZoneItem` | `xx_zone_item_v1`    | Positionable wrapper around a toplevel `QWindow`     |

```cpp
Winpos::Manager *mgr = Winpos::Manager::instance();
if (!mgr->isActive()) { /* compositor does not support xx-zones-v1 */ }

Winpos::Zone *zone = mgr->zoneForScreen(window->screen());  // cached per screen
Winpos::ZoneItem *item = mgr->createItem(window);           // requires window shown

zone->addItem(item);
item->setPosition({100, 50});

// Share a zone with another client process:
QString handle = zone->handle();   // pass to the other process
Winpos::Zone *shared = mgr->zoneFromHandle(handle);
```

### Coordinate conventions

`WindowPositioner::position()` and `ZoneItem::position()` report the window's
**frame** top-left in zone-relative logical pixels on Wayland. Use
`frameExtents()` to convert between frame and content-area coordinates.

On X11 `position()` returns the content-area position relative to
`QScreen::availableGeometry()`.

## Platform support

| Platform                    | Behaviour                                                                       |
|-----------------------------|---------------------------------------------------------------------------------|
| Wayland + xx-zones-v1       | Full protocol; compositor places windows at requested coordinates               |
| Wayland without xx-zones-v1 | `Manager::isActive()` is false; `WindowPositioner::move()` is a no-op           |
| X11                         | Falls back to `QWindow::setPosition()`; zone is the screen's available geometry |

## Build

**Requirements:**
- CMake >= 3.16
- Qt >= 6.8 (`Core`, `Gui`, `GuiPrivate`, `WaylandClient`)
- wayland-protocols

Build with the example application:

```bash
cmake -S . -B build -G Ninja -DBUILD_EXAMPLES=ON
cmake --build build
./build/examples/winpos-zone-demo
```
