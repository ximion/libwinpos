/*
 * Copyright (C) 2025-2026 Matthias Klumpp <matthias@tenstral.net>
 *
 * SPDX-License-Identifier: MIT
 */

#include "Winpos/Manager.h"

#include <QGuiApplication>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QPointer>
#include <QScreen>
#include <QWindow>
#include <QtGui/qpa/qplatformnativeinterface.h>
#include <QtGui/qscreen_platform.h>

#include "Winpos/Zone.h"
#include "Winpos/ZoneItem.h"
#include "qwayland-xx-zones-v1.h"

Q_LOGGING_CATEGORY(logWinpos, "winpos.zones", QtWarningMsg)

namespace Winpos
{

class Manager::Private : public QtWayland::xx_zone_manager_v1
{
public:
    QHash<QScreen *, QPointer<Zone>> screenZones;
};

static Manager *s_manager = nullptr;

Manager *Manager::instance()
{
    if (!s_manager) {
        s_manager = new Manager();
        if (qApp) {
            s_manager->setParent(qApp);
            QObject::connect(qApp, &QObject::destroyed, [] {
                s_manager = nullptr;
            });
        }
        s_manager->initialize();
    }
    return s_manager;
}

Manager::Manager()
    : QWaylandClientExtension(1)
    , d(new Private())
{
    connect(this, &QWaylandClientExtension::activeChanged, this, [this] {
        if (!isActive()) {
            qCDebug(logWinpos) << "xx-zones-v1 protocol became inactive";
            d->screenZones.clear();
        } else {
            qCDebug(logWinpos) << "xx-zones-v1 protocol active (version" << version() << ")";
        }
    });
    if (qApp) {
        connect(qApp, &QGuiApplication::screenRemoved, this, [this](QScreen *screen) {
            d->screenZones.remove(screen);
        });
    }
}

Manager::~Manager()
{
    if (d->isInitialized()) {
        d->destroy();
    }
    delete d;
    if (s_manager == this) {
        s_manager = nullptr;
    }
}

const wl_interface *Manager::extensionInterface() const
{
    return QtWayland::xx_zone_manager_v1::interface();
}

void Manager::bind(wl_registry *registry, int id, int version)
{
    const int v = qMin(version, qMin(static_cast<int>(QtWayland::xx_zone_manager_v1::interface()->version), this->version()));
    setVersion(v);
    d->init(registry, id, v);
}

Zone *Manager::zoneForScreen(QScreen *screen)
{
    if (!isActive()) {
        qCWarning(logWinpos) << "zoneForScreen: xx-zones-v1 is not active";
        return nullptr;
    }
    if (!screen) {
        screen = qGuiApp->primaryScreen();
    }
    if (!screen) {
        return nullptr;
    }

    auto it = d->screenZones.find(screen);
    if (it != d->screenZones.end() && !it.value().isNull()) {
        return it.value().data();
    }

    wl_output *output = nullptr;
    if (auto ws = screen->nativeInterface<QNativeInterface::QWaylandScreen>()) {
        output = ws->output();
    }

    auto raw = d->get_zone(output);
    if (!raw) {
        qCWarning(logWinpos) << "get_zone returned null for screen" << screen->name();
        return nullptr;
    }

    auto zone = new Zone(raw, this);
    d->screenZones[screen] = zone;
    connect(zone, &QObject::destroyed, this, [this, screen] {
        d->screenZones.remove(screen);
    });
    return zone;
}

Zone *Manager::zoneFromHandle(const QString &handle)
{
    if (!isActive()) {
        qCWarning(logWinpos) << "zoneFromHandle: xx-zones-v1 is not active";
        return nullptr;
    }
    auto raw = d->get_zone_from_handle(handle);
    return raw ? new Zone(raw) : nullptr;
}

ZoneItem *Manager::createItem(QWindow *window)
{
    if (!isActive()) {
        qCWarning(logWinpos) << "createItem: xx-zones-v1 is not active";
        return nullptr;
    }
    if (!window) {
        return nullptr;
    }

    auto pni = qGuiApp->platformNativeInterface();
    if (!pni) {
        return nullptr;
    }

    auto toplevel = static_cast<struct ::xdg_toplevel *>(pni->nativeResourceForWindow("xdg_toplevel", window));
    if (!toplevel) {
        qCWarning(logWinpos) << "createItem: no xdg_toplevel for window — is it shown?";
        return nullptr;
    }

    auto raw = d->get_zone_item(toplevel);
    return raw ? new ZoneItem(raw, window) : nullptr;
}

} // namespace Winpos
