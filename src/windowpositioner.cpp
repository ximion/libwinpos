/*
 * Copyright (C) 2025-2026 Matthias Klumpp <matthias@tenstral.net>
 *
 * SPDX-License-Identifier: MIT
 */

#include "Winpos/WindowPositioner.h"

#include <QByteArray>
#include <QDataStream>
#include <QEvent>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QPlatformSurfaceEvent>
#include <QPointer>
#include <QScreen>
#include <QWindow>
#include <QtGui/qpa/qplatformwindow_p.h>

#include "Winpos/Manager.h"
#include "Winpos/Zone.h"
#include "Winpos/ZoneItem.h"

Q_DECLARE_LOGGING_CATEGORY(logWinpos)

namespace Winpos
{

class WindowPositioner::Private
{
public:
    QWindow *window = nullptr;
    bool isOnWayland = false;

    // Wayland-only
    ZoneItem *item = nullptr;
    QPointer<Zone> zone;
    bool autoZone = true;
    QMetaObject::Connection zoneSizeConn;
    QPoint pendingPosition;
    bool hasPendingPosition = false;

    // Position/size state (all environments)
    QPoint position;
    QSize zoneSize;
    QMargins frameExtents;

    // X11-only
    QMetaObject::Connection screenSizeConn;
};

static QPoint screenZoneOrigin(const QWindow *window)
{
    const auto screen = window->screen();
    return screen ? screen->availableGeometry().topLeft() : QPoint();
}

WindowPositioner::WindowPositioner(QWindow *window, QObject *parent)
    : QObject(parent)
    , d(new Private)
{
    Q_ASSERT(window);
    d->window = window;
    d->isOnWayland = (QGuiApplication::platformName() == QLatin1String("wayland"));

    if (d->isOnWayland) {
        d->window->installEventFilter(this);

        auto ww = d->window->nativeInterface<QNativeInterface::Private::QWaylandWindow>();
        if (ww) {
            connect(ww, &QNativeInterface::Private::QWaylandWindow::surfaceRoleCreated, this, &WindowPositioner::onSurfaceRoleCreated);
            connect(ww, &QNativeInterface::Private::QWaylandWindow::surfaceRoleDestroyed, this, &WindowPositioner::onSurfaceRoleDestroyed);
            QMetaObject::invokeMethod(this, &WindowPositioner::onSurfaceRoleCreated, Qt::QueuedConnection);
        }

        connect(Manager::instance(), &Manager::activeChanged, this, &WindowPositioner::onManagerActiveChanged);
        connect(d->window, &QWindow::screenChanged, this, &WindowPositioner::onWaylandScreenChanged);
    } else {
        connect(d->window, &QWindow::xChanged, this, &WindowPositioner::onX11PositionChanged);
        connect(d->window, &QWindow::yChanged, this, &WindowPositioner::onX11PositionChanged);
        connect(d->window, &QWindow::screenChanged, this, [this](QScreen *screen) {
            setupX11(screen);
        });
        setupX11(d->window->screen());
    }
}

WindowPositioner::~WindowPositioner()
{
    if (d->isOnWayland) {
        cleanupWayland();
    }
    delete d;
}

QWindow *WindowPositioner::window() const
{
    return d->window;
}
Zone *WindowPositioner::zone() const
{
    return d->zone.data();
}

void WindowPositioner::setZone(Zone *zone)
{
    d->autoZone = false;
    if (d->zone == zone) {
        return;
    }

    if (d->item && d->zone) {
        d->zone->removeItem(d->item);
    }

    disconnect(d->zoneSizeConn);
    d->zone = zone;
    Q_EMIT zoneChanged(zone);

    if (zone) {
        d->zoneSize = zone->size();
        d->zoneSizeConn = connect(zone, &Zone::sizeChanged, this, [this](const QSize &zs) {
            d->zoneSize = zs;
            Q_EMIT zoneSizeChanged(zs);
        });
    }

    if (d->item && zone) {
        zone->addItem(d->item);
    }
}

QPoint WindowPositioner::position() const
{
    if (d->isOnWayland) {
        return d->position;
    }
    if (!d->window) {
        return {};
    }

    return d->window->position() - screenZoneOrigin(d->window);
}

QSize WindowPositioner::zoneSize() const
{
    if (d->isOnWayland) {
        return d->zoneSize;
    }
    if (!d->window || !d->window->screen()) {
        return {};
    }

    return d->window->screen()->availableGeometry().size();
}

QMargins WindowPositioner::frameExtents() const
{
    if (d->isOnWayland) {
        return d->frameExtents;
    }
    if (!d->window) {
        return {};
    }

    return d->window->frameMargins();
}

QRect WindowPositioner::geometry() const
{
    if (!d->window) {
        return {};
    }

    return {position(), d->window->size()};
}

QRect WindowPositioner::frameGeometry() const
{
    if (!d->window) {
        return {};
    }
    const QMargins ext = frameExtents();
    const QPoint fp = position() - QPoint(ext.left(), ext.top());
    const QSize fs = d->window->size() + QSize(ext.left() + ext.right(), ext.top() + ext.bottom());

    return {fp, fs};
}

bool WindowPositioner::isActive() const
{
    if (d->isOnWayland) {
        return d->item && d->zone;
    }

    return d->window != nullptr;
}

void WindowPositioner::move(const QPoint &pos)
{
    if (d->isOnWayland) {
        if (!d->item || !d->zone) {
            d->pendingPosition = pos;
            d->hasPendingPosition = true;
            return;
        }
        d->item->setPosition(pos);
    } else {
        if (!d->window) {
            return;
        }
        d->window->setPosition(screenZoneOrigin(d->window) + pos);
        if (d->position != pos) {
            d->position = pos;
            Q_EMIT positionChanged(pos);
        }
    }
}

void WindowPositioner::move(int x, int y)
{
    move(QPoint(x, y));
}

void WindowPositioner::setGeometry(const QRect &rect)
{
    move(rect.topLeft());
    if (d->window) {
        d->window->resize(rect.size());
    }
}

bool WindowPositioner::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == d->window && event->type() == QEvent::PlatformSurface) {
        auto se = dynamic_cast<QPlatformSurfaceEvent *>(event);
        if (se->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated) {
            auto ww = d->window->nativeInterface<QNativeInterface::Private::QWaylandWindow>();
            if (ww) {
                connect(ww,
                        &QNativeInterface::Private::QWaylandWindow::surfaceRoleCreated,
                        this,
                        &WindowPositioner::onSurfaceRoleCreated,
                        Qt::UniqueConnection);
                connect(ww,
                        &QNativeInterface::Private::QWaylandWindow::surfaceRoleDestroyed,
                        this,
                        &WindowPositioner::onSurfaceRoleDestroyed,
                        Qt::UniqueConnection);
            }
        } else if (se->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
            cleanupWayland();
        }
    }

    return QObject::eventFilter(watched, event);
}

void WindowPositioner::onSurfaceRoleCreated()
{
    if (!d->item && Manager::instance()->isActive()) {
        initializeWayland();
    }
}

void WindowPositioner::onSurfaceRoleDestroyed()
{
    cleanupWayland();
}

void WindowPositioner::onManagerActiveChanged()
{
    if (Manager::instance()->isActive()) {
        if (!d->item) {
            initializeWayland();
        }
    } else {
        cleanupWayland();
    }
}

void WindowPositioner::onWaylandScreenChanged(QScreen *screen)
{
    if (!d->autoZone || !d->item) {
        return;
    }

    auto newZone = Manager::instance()->zoneForScreen(screen);
    if (!newZone || newZone == d->zone) {
        return;
    }

    if (d->zone) {
        d->zone->removeItem(d->item);
    }

    disconnect(d->zoneSizeConn);
    d->zone = newZone;
    d->zoneSize = newZone->size();
    Q_EMIT zoneChanged(newZone);
    Q_EMIT zoneSizeChanged(d->zoneSize);

    d->zoneSizeConn = connect(newZone, &Zone::sizeChanged, this, [this](const QSize &sz) {
        d->zoneSize = sz;
        Q_EMIT zoneSizeChanged(sz);
    });
    newZone->addItem(d->item);
}

void WindowPositioner::onX11PositionChanged()
{
    if (!d->window) {
        return;
    }
    const QPoint pos = d->window->position() - screenZoneOrigin(d->window);
    if (pos != d->position) {
        d->position = pos;
        Q_EMIT positionChanged(pos);
    }
}

void WindowPositioner::initializeWayland()
{
    if (d->item) {
        return;
    }

    d->item = Manager::instance()->createItem(d->window);
    if (!d->item) {
        return;
    }

    connect(d->item, &ZoneItem::positionChanged, this, [this](const QPoint &pos) {
        d->position = pos;
        Q_EMIT positionChanged(pos);
    });
    connect(d->item, &ZoneItem::positionFailed, this, &WindowPositioner::positionFailed);
    connect(d->item, &ZoneItem::frameExtentsChanged, this, [this](const QMargins &ext) {
        d->frameExtents = ext;
        Q_EMIT frameExtentsChanged(ext);
    });
    connect(d->item, &ZoneItem::closed, this, [this] {
        d->item->deleteLater();
        d->item = nullptr;
        d->zone = nullptr;
        disconnect(d->zoneSizeConn);
    });

    if (!d->zone && d->autoZone) {
        d->zone = Manager::instance()->zoneForScreen(d->window->screen());
    }

    if (d->zone) {
        d->zoneSize = d->zone->size();
        disconnect(d->zoneSizeConn);
        d->zoneSizeConn = connect(d->zone, &Zone::sizeChanged, this, [this](const QSize &sz) {
            d->zoneSize = sz;
            Q_EMIT zoneSizeChanged(sz);
        });
        d->zone->addItem(d->item);
    }

    if (d->hasPendingPosition) {
        d->hasPendingPosition = false;
        move(d->pendingPosition);
    }
}

void WindowPositioner::cleanupWayland()
{
    if (!d->item) {
        return;
    }

    disconnect(d->zoneSizeConn);

    if (d->zone && !d->item->isClosed()) {
        d->zone->removeItem(d->item);
    }

    delete d->item;
    d->item = nullptr;
    d->zone = nullptr;
}

void WindowPositioner::setupX11(QScreen *screen)
{
    disconnect(d->screenSizeConn);
    if (!screen) {
        return;
    }

    d->zoneSize = screen->availableGeometry().size();
    Q_EMIT zoneSizeChanged(d->zoneSize);

    // emit initial position so listeners don't have to wait for a move event
    onX11PositionChanged();

    d->screenSizeConn = connect(screen, &QScreen::availableGeometryChanged, this, [this](const QRect &geom) {
        d->zoneSize = geom.size();
        Q_EMIT zoneSizeChanged(d->zoneSize);
        onX11PositionChanged();
    });
}

QByteArray WindowPositioner::saveGeometry() const
{
    if (!d->window)
        return {};

    QByteArray array;
    QDataStream stream(&array, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_8);

    const quint32 magicNumber = 0x5747454F; // 'WGEO'
    const quint16 majorVersion = 1;
    const quint16 minorVersion = 0;
    stream << magicNumber << majorVersion << minorVersion << frameGeometry() << geometry() << zoneSize()
           << quint8(d->window->windowStates() & Qt::WindowMaximized) << quint8(d->window->windowStates() & Qt::WindowFullScreen);

    return array;
}

bool WindowPositioner::restoreGeometry(const QByteArray &geometry)
{
    if (geometry.size() < 4)
        return false;

    QDataStream stream(geometry);
    stream.setVersion(QDataStream::Qt_6_8);

    const quint32 magicNumber = 0x5747454F; // 'WGEO'
    quint32 storedMagicNumber;
    stream >> storedMagicNumber;
    if (storedMagicNumber != magicNumber)
        return false;

    const quint16 currentMajorVersion = 1;
    quint16 majorVersion = 0;
    quint16 minorVersion = 0;
    stream >> majorVersion >> minorVersion;

    if (majorVersion > currentMajorVersion)
        return false;

    QRect restoredFrameGeometry;
    QRect restoredGeometry;
    QSize restoredZoneSize;
    quint8 maximized = 0;
    quint8 fullScreen = 0;

    stream >> restoredFrameGeometry >> restoredGeometry >> restoredZoneSize >> maximized >> fullScreen;

    if (stream.status() != QDataStream::Ok)
        return false;

    // Clamp position if the zone size has changed since the geometry was saved.
    const QSize currentZoneSize = zoneSize();
    if (!currentZoneSize.isEmpty() && !restoredZoneSize.isEmpty() && currentZoneSize != restoredZoneSize) {
        QPoint pos = restoredGeometry.topLeft();
        pos.setX(qBound(0, pos.x(), currentZoneSize.width() - restoredGeometry.width()));
        pos.setY(qBound(0, pos.y(), currentZoneSize.height() - restoredGeometry.height()));
        restoredGeometry.moveTopLeft(pos);
    }

    if (!maximized && !fullScreen)
        setGeometry(restoredGeometry);

    if (d->window) {
        Qt::WindowStates states = d->window->windowStates();
        states.setFlag(Qt::WindowMaximized, maximized != 0);
        states.setFlag(Qt::WindowFullScreen, fullScreen != 0);
        if (maximized || fullScreen)
            d->window->setWindowStates(states);
    }

    return true;
}

} // namespace Winpos
