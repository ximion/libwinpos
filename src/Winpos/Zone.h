/*
 * Copyright (C) 2025-2026 Matthias Klumpp <matthias@tenstral.net>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <QObject>
#include <QSize>
#include <QString>

#include "winposqt_export.h"

struct xx_zone_v1;

namespace Winpos
{

class ZoneItem;

/**
 * @brief Represents a display area provided by the compositor.
 *
 * A Zone corresponds to an xx_zone_v1 protocol object. Items (toplevel windows)
 * can be added to a zone and positioned within its coordinate space via
 * ZoneItem::setPosition().
 *
 * A zone is identified by a string handle that can be shared with other clients
 * to join the same coordinate space (see Manager::zoneFromHandle()).
 *
 * An invalid zone (isValid() == false) has a negative size and must not be used.
 * Check isReady() before assuming the zone's dimensions are final.
 */
class WINPOSQT_EXPORT Zone : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QSize size READ size NOTIFY sizeChanged)
    Q_PROPERTY(QString handle READ handle NOTIFY handleChanged)
    Q_PROPERTY(bool valid READ isValid CONSTANT)
    Q_PROPERTY(bool ready READ isReady NOTIFY ready)

public:
    ~Zone() override;

    QSize size() const;
    QString handle() const;

    /** Returns false if the compositor denied the zone request. */
    bool isValid() const;

    /** Returns true when the zone is ready to be used. */
    bool isReady() const;

    /** Returns true if width or height is infinite. */
    bool isInfiniteHorizontal() const;
    bool isInfiniteVertical() const;

    /** Add/remove items */
    void addItem(ZoneItem *item);
    void removeItem(ZoneItem *item);

Q_SIGNALS:
    void sizeChanged(const QSize &size);
    void handleChanged(const QString &handle);
    void ready();
    void itemEntered(Winpos::ZoneItem *item);
    void itemLeft(Winpos::ZoneItem *item);
    void itemBlocked(Winpos::ZoneItem *item);

private:
    explicit Zone(struct ::xx_zone_v1 *raw, QObject *parent = nullptr);
    friend class Manager;

    class Private;
    Private *d;
};

} // namespace Winpos
