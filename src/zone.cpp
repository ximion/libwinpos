/*
 * Copyright (C) 2025-2026 Matthias Klumpp <matthias@tenstral.net>
 *
 * SPDX-License-Identifier: MIT
 */

#include "Winpos/Zone.h"

#include "Winpos/ZoneItem.h"
#include "qwayland-xx-zones-v1.h"

namespace Winpos
{

class Zone::Private : public QtWayland::xx_zone_v1
{
public:
    explicit Private(Zone *zone, ::xx_zone_v1 *raw)
        : xx_zone_v1(raw)
        , self(zone)
    {
    }

    Zone *self;
    QSize size;
    QString handle;
    bool valid = true;
    bool ready = false;

protected:
    void xx_zone_v1_size(int32_t width, int32_t height) override
    {
        if (!ready && (width < 0 || height < 0)) {
            valid = false;
            size = QSize(width, height);
            return;
        }
        size = QSize(width, height);
        Q_EMIT self->sizeChanged(size);
    }

    void xx_zone_v1_handle(const QString &pHandle) override
    {
        handle = pHandle;
        Q_EMIT self->handleChanged(handle);
    }

    void xx_zone_v1_done() override
    {
        ready = true;
        Q_EMIT self->ready();
    }

    void xx_zone_v1_item_blocked(::xx_zone_item_v1 *raw) override
    {
        if (auto *item = ZoneItem::fromNativeItem(raw)) {
            Q_EMIT self->itemBlocked(item);
        }
    }

    void xx_zone_v1_item_entered(::xx_zone_item_v1 *raw) override
    {
        if (auto *item = ZoneItem::fromNativeItem(raw)) {
            item->enterZone(self);
            Q_EMIT self->itemEntered(item);
        }
    }

    void xx_zone_v1_item_left(struct ::xx_zone_item_v1 *raw) override
    {
        if (auto *item = ZoneItem::fromNativeItem(raw)) {
            item->leaveZone();
            Q_EMIT self->itemLeft(item);
        }
    }
};

Zone::Zone(::xx_zone_v1 *raw, QObject *parent)
    : QObject(parent)
    , d(new Private(this, raw))
{
}

Zone::~Zone()
{
    if (d->isInitialized()) {
        d->destroy();
    }
    delete d;
}

QSize Zone::size() const
{
    return d->size;
}

QString Zone::handle() const
{
    return d->handle;
}

bool Zone::isValid() const
{
    return d->valid;
}

bool Zone::isReady() const
{
    return d->ready;
}

bool Zone::isInfiniteHorizontal() const
{
    return d->size.width() == 0;
}

bool Zone::isInfiniteVertical() const
{
    return d->size.height() == 0;
}

void Zone::addItem(ZoneItem *item)
{
    if (!item || item->isClosed() || !d->isInitialized() || !d->valid) {
        return;
    }
    d->add_item(item->nativeItem());
}

void Zone::removeItem(ZoneItem *item)
{
    if (!item || !d->isInitialized()) {
        return;
    }
    d->remove_item(item->nativeItem());
}

} // namespace Winpos
