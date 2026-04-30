/*
 * Copyright (C) 2025-2026 Matthias Klumpp <matthias@tenstral.net>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <QMargins>
#include <QObject>
#include <QPoint>

#include "winposqt_export.h"

class QWindow;
struct xx_zone_item_v1;

namespace Winpos
{

class Zone;

/**
 * @brief Represents a positionable wrapper around a toplevel QWindow.
 *
 * A ZoneItem corresponds to an xx_zone_item_v1 protocol object. It is created
 * by Manager::createItem() and can be placed in a Zone via Zone::addItem().
 */
class WINPOSQT_EXPORT ZoneItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QPoint position READ position NOTIFY positionChanged)
    Q_PROPERTY(QMargins frameExtents READ frameExtents NOTIFY frameExtentsChanged)
    Q_PROPERTY(bool closed READ isClosed NOTIFY closed)

public:
    ~ZoneItem() override;

    QWindow *window() const;
    Zone *zone() const;

    QPoint position() const;
    QMargins frameExtents() const;
    bool isClosed() const;

    void setPosition(const QPoint &pos);
    void setPosition(int x, int y);

Q_SIGNALS:
    void positionChanged(const QPoint &pos);
    void positionFailed();
    void frameExtentsChanged(const QMargins &extents);
    void zoneChanged(Winpos::Zone *zone);
    void closed();

private:
    explicit ZoneItem(::xx_zone_item_v1 *raw, QWindow *window, QObject *parent = nullptr);
    friend class Manager;
    friend class Zone;

    // Bridge methods called by Zone's event handlers
    void enterZone(Zone *zone);
    void leaveZone();
    ::xx_zone_item_v1 *nativeItem() const;

    // Recover a ZoneItem* from a raw Wayland handle
    static ZoneItem *fromNativeItem(::xx_zone_item_v1 *raw);

    class Private;
    Private *d;
};

} // namespace Winpos
