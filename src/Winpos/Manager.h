/*
 * Copyright (C) 2025-2026 Matthias Klumpp <matthias@tenstral.net>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "winposqt_export.h"
#include <QWaylandClientExtension>

class QScreen;
class QWindow;

namespace Winpos
{

class Zone;
class ZoneItem;

/**
 * @brief Process-wide manager for the xx-zones-v1 Wayland protocol.
 *
 * Obtain the singleton via Manager::instance(). Check isActive() to determine
 * whether the compositor supports the protocol before using any other methods.
 *
 * Manager caches one Zone per QScreen (zoneForScreen()). When zones are no
 * longer needed, the application can discard the Zone objects - the cache will
 * be updated on the next call to zoneForScreen() for that screen.
 *
 * Most callers should use WindowPositioner instead of the Manager/Zone/ZoneItem
 * API directly.
 */
class WINPOSQT_EXPORT Manager : public QWaylandClientExtension
{
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)

public:
    /** Returns the process-wide singleton, creating it if necessary. */
    static Manager *instance();
    ~Manager() override;

    /**
     * Returns a Zone for the given screen.
     * If @p screen is nullptr, the compositor picks the output.
     * Returns nullptr when the protocol is not active.
     */
    Zone *zoneForScreen(QScreen *screen = nullptr);

    /**
     * Joins an existing zone identified by @p handle, or creates a new zone
     * if the handle is unknown. The caller takes ownership of the returned Zone.
     * Returns nullptr when the protocol is not active.
     */
    Zone *zoneFromHandle(const QString &handle);

    /**
     * Creates a ZoneItem representing @p window's xdg_toplevel.
     * The window must have been created (create() / show() called) before this.
     * The caller takes ownership of the returned ZoneItem.
     * Returns nullptr if the protocol is not active or the window has no toplevel.
     */
    ZoneItem *createItem(QWindow *window);

    // QWaylandClientExtension interface
    const wl_interface *extensionInterface() const override;
    void bind(wl_registry *registry, int id, int version) override;

private:
    explicit Manager();
    Q_DISABLE_COPY_MOVE(Manager)

    class Private;
    Private *d;
};

} // namespace Winpos
