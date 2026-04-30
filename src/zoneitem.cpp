/*
 * Copyright (C) 2025-2026 Matthias Klumpp <matthias@tenstral.net>
 *
 * SPDX-License-Identifier: MIT
 */

#include "Winpos/ZoneItem.h"

#include <QPointer>
#include <QWindow>

#include "Winpos/Zone.h"
#include "qwayland-xx-zones-v1.h"

namespace Winpos
{

class ZoneItem::Private : public QtWayland::xx_zone_item_v1
{
public:
    explicit Private(ZoneItem *zItem, ::xx_zone_item_v1 *raw, QWindow *window)
        : QtWayland::xx_zone_item_v1(raw)
        , self(zItem)
        , window(window)
    {
        // NOTE: Constructing xx_zone_iten_v1 calls add_listener() with this
        // ZoneItem::Private instance as data.
    }

    ZoneItem *self;
    QWindow *window = nullptr;
    QPoint position;
    QMargins frameExtents;
    QPointer<Zone> zone;
    bool closed = false;

protected:
    void xx_zone_item_v1_frame_extents(int32_t top, int32_t bottom, int32_t left, int32_t right) override
    {
        frameExtents = QMargins(left, top, right, bottom);
        Q_EMIT self->frameExtentsChanged(frameExtents);
    }

    void xx_zone_item_v1_position(int32_t x, int32_t y) override
    {
        position = QPoint(x, y);
        Q_EMIT self->positionChanged(position);
    }

    void xx_zone_item_v1_position_failed() override
    {
        Q_EMIT self->positionFailed();
    }

    void xx_zone_item_v1_closed() override
    {
        self->d->closed = true;
        if (self->d->zone) {
            self->d->zone = nullptr;
            Q_EMIT self->zoneChanged(nullptr);
        }
        Q_EMIT self->closed();
    }
};

ZoneItem::ZoneItem(::xx_zone_item_v1 *raw, QWindow *window, QObject *parent)
    : QObject(parent)
    , d(new Private(this, raw, window))
{
}

ZoneItem::~ZoneItem()
{
    if (d->isInitialized() && !d->closed) {
        d->destroy();
    }
    delete d;
}

QWindow *ZoneItem::window() const
{
    return d->window;
}

Zone *ZoneItem::zone() const
{
    return d->zone.data();
}

QPoint ZoneItem::position() const
{
    return d->position;
}

QMargins ZoneItem::frameExtents() const
{
    return d->frameExtents;
}

bool ZoneItem::isClosed() const
{
    return d->closed;
}

void ZoneItem::setPosition(const QPoint &pos)
{
    if (!d->isInitialized() || d->closed) {
        return;
    }
    d->set_position(pos.x(), pos.y());
}

void ZoneItem::setPosition(int x, int y)
{
    setPosition(QPoint(x, y));
}

void ZoneItem::enterZone(Zone *zone)
{
    d->zone = zone;
    Q_EMIT zoneChanged(zone);
}

void ZoneItem::leaveZone()
{
    d->zone = nullptr;
    Q_EMIT zoneChanged(nullptr);
}

::xx_zone_item_v1 *ZoneItem::nativeItem() const
{
    return d->object();
}

ZoneItem *ZoneItem::fromNativeItem(::xx_zone_item_v1 *raw)
{
    // fromObject() returns the Private* stored as listener data by add_listener.
    auto priv = static_cast<Private *>(QtWayland::xx_zone_item_v1::fromObject(raw));
    return priv ? priv->self : nullptr;
}

} // namespace Winpos
