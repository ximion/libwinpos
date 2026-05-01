/*
 * Copyright (C) 2025-2026 Matthias Klumpp <matthias@tenstral.net>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <QWindow>

#include "Zone.h"
#include "winposqt_export.h"

class QScreen;

namespace Winpos
{

/**
 * @brief Cross-platform window positioning helper.
 *
 * WindowPositioner gives applications a single API that works on both Wayland
 * (via the xx-zones-v1 protocol) and X11 / other platforms (via Qt's native
 * QWindow positioning).
 *
 * On **Wayland** with a supporting compositor, the positioner registers the
 * window's xdg_toplevel with a zone and sends zone-relative position requests
 * through the protocol. All coordinates are relative to the zone's (0, 0)
 * origin. The positioner initializes asynchronously once the Wayland surface
 * role is ready.
 *
 * On **X11** (and other non-Wayland platforms) the positioner delegates
 * directly to Qt's `QWindow::setPosition()` / `QWindow::position()`, treating
 * the screen's available geometry as the zone.  Coordinates are relative to
 * the top-left of `QScreen::availableGeometry()` so that (0, 0) means
 * "top-left of the usable screen area", somewhat consistent with the Wayland
 * zone model.
 *
 * All position coordinates refer to the **content area** (window without
 * frame decorations), matching Qt's own `QWindow::position()` convention.
 * Use `frameGeometry()` when you need the full frame-inclusive rectangle.
 *
 * Usage example:
 * @code
 * // In QWidget::showEvent or after QWindow is visible:
 * auto pos = new Winpos::WindowPositioner(windowHandle(), this);
 * connect(pos, &Winpos::WindowPositioner::positionChanged, label, [label, pos] {
 *     label->setText(QString("Position: %1, %2").arg(pos->position().x())
 *                                               .arg(pos->position().y()));
 * });
 * pos->move(100, 200);
 * @endcode
 */
class WINPOSQT_EXPORT WindowPositioner : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QWindow *window READ window CONSTANT)
    Q_PROPERTY(Winpos::Zone *zone READ zone WRITE setZone NOTIFY zoneChanged)
    Q_PROPERTY(QPoint position READ position NOTIFY positionChanged)
    Q_PROPERTY(QSize zoneSize READ zoneSize NOTIFY zoneSizeChanged)
    Q_PROPERTY(QMargins frameExtents READ frameExtents NOTIFY frameExtentsChanged)
    Q_PROPERTY(QRect geometry READ geometry NOTIFY positionChanged)
    Q_PROPERTY(QRect frameGeometry READ frameGeometry NOTIFY positionChanged)

public:
    explicit WindowPositioner(QWindow *window, QObject *parent = nullptr);
    ~WindowPositioner() override;

    QWindow *window() const;

    /**
     * The zone this window is positioned in (Wayland only; nullptr on X11).
     * If not set explicitly, defaults to Manager::zoneForScreen(window->screen()).
     * Setting an explicit zone disables automatic screen-following.
     */
    Zone *zone() const;
    void setZone(Zone *zone);

    /**
     * Current window position relative to the zone/screen origin.
     *
     * On Wayland this is the **frame** top-left (same coordinate as the
     * protocol's @c position event and @c set_position request), so it
     * includes window decorations. Use @c frameExtents() to convert
     * between frame and content-area coordinates when needed.
     *
     * On X11 it is the content-area position (@c QWindow::position()
     * minus the available-geometry origin).
     */
    QPoint position() const;

    /**
     * Size of the positioning zone
     * (zone protocol size on Wayland, available screen geometry size on X11).
     */
    QSize zoneSize() const;

    /**
     * Frame decoration extents around the content area.
     */
    QMargins frameExtents() const;

    /** Content geometry (position + window size) in zone/screen coordinates. */
    QRect geometry() const;

    /** Full frame geometry including decorations in zone/screen coordinates. */
    QRect frameGeometry() const;

    /** Returns true when the positioner is fully valid and can send positions. */
    bool isActive() const;

public Q_SLOTS:
    /** Suggest a zone/screen-relative content position for this window. */
    void move(const QPoint &pos);
    void move(int x, int y);

    /**
     * Set both the position and size of the window.
     * Equivalent to move(rect.topLeft()) + window()->resize(rect.size()).
     */
    void setGeometry(const QRect &rect);

    /**
     * Serialize the current zone-relative geometry (position + size) and
     * window state into an opaque byte array suitable for storing in
     * application settings. The format mirrors Qt's QWidget::saveGeometry().
     *
     * @return Serialized geometry, or an empty QByteArray if the positioner
     *         has no valid window.
     */
    QByteArray saveGeometry() const;

    /**
     * Restore the zone-relative geometry and window state from data previously
     * produced by saveGeometry(). The format mirrors Qt's
     * QWidget::restoreGeometry().
     *
     * If the stored zone size differs from the current one, the position is
     * clamped so the window stays inside the zone.
     *
     * @param geometry  Byte array returned by a prior saveGeometry() call.
     * @return @c true if the data was valid and the geometry was applied,
     *         @c false if the data was unrecognized or malformed.
     */
    bool restoreGeometry(const QByteArray &geometry);

Q_SIGNALS:
    void zoneChanged(Winpos::Zone *zone);
    void positionChanged(const QPoint &pos);
    void positionFailed();
    void frameExtentsChanged(const QMargins &extents);
    void zoneSizeChanged(const QSize &size);

public:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Q_SLOT void onSurfaceRoleCreated();
    Q_SLOT void onSurfaceRoleDestroyed();
    Q_SLOT void onManagerActiveChanged();
    Q_SLOT void onWaylandScreenChanged(QScreen *screen);
    Q_SLOT void onX11PositionChanged();

    void initializeWayland();
    void cleanupWayland();
    void setupX11(QScreen *screen);

    class Private;
    Private *d;
};

} // namespace Winpos
