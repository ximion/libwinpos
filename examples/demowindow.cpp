/*
 * Copyright (C) 2025-2026 Matthias Klumpp <matthias@tenstral.net>
 *
 * SPDX-License-Identifier: MIT
 */

#include "demowindow.h"

#include <Winpos/Manager.h>
#include <Winpos/WindowPositioner.h>

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

static constexpr int kStep = 128; // pixels per directional move

DemoWindow::DemoWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Winpos Zone Demo"));
    setMinimumSize(400, 320);
    setupUi();
}

DemoWindow::~DemoWindow() = default;

void DemoWindow::setupUi()
{
    auto central = new QWidget(this);
    setCentralWidget(central);

    auto vbox = new QVBoxLayout(central);

    // Info group
    auto infoGroup = new QGroupBox(QStringLiteral("Zone info"), central);
    auto infoLayout = new QVBoxLayout(infoGroup);

    m_statusLabel = new QLabel(QStringLiteral("Protocol: checking…"), infoGroup);
    m_zoneSizeLabel = new QLabel(QStringLiteral("Zone size: unknown"), infoGroup);
    m_positionLabel = new QLabel(QStringLiteral("Position: unknown"), infoGroup);

    infoLayout->addWidget(m_statusLabel);
    infoLayout->addWidget(m_zoneSizeLabel);
    infoLayout->addWidget(m_positionLabel);
    vbox->addWidget(infoGroup);

    // Movement group
    auto moveGroup = new QGroupBox(QStringLiteral("Move window (zone-relative)"), central);
    auto grid = new QGridLayout(moveGroup);

    auto addBtn = [&](const QString &label, int row, int col, auto handler) {
        auto *btn = new QPushButton(label, moveGroup);
        btn->setFixedSize(80, 36);
        grid->addWidget(btn, row, col, Qt::AlignCenter);
        connect(btn, &QPushButton::clicked, this, handler);
    };

    addBtn(QStringLiteral("↖"), 0, 0, [this] {
        if (m_positioner) {
            m_positioner->move(m_positioner->position() + QPoint(-kStep, -kStep));
        }
    });
    addBtn(QStringLiteral("↑"), 0, 1, [this] {
        if (m_positioner) {
            m_positioner->move(m_positioner->position() + QPoint(0, -kStep));
        }
    });
    addBtn(QStringLiteral("↗"), 0, 2, [this] {
        if (m_positioner) {
            m_positioner->move(m_positioner->position() + QPoint(kStep, -kStep));
        }
    });

    addBtn(QStringLiteral("←"), 1, 0, [this] {
        if (m_positioner) {
            m_positioner->move(m_positioner->position() + QPoint(-kStep, 0));
        }
    });
    addBtn(QStringLiteral("⌂ Center"), 1, 1, [this] {
        if (!m_positioner || !m_positioner->isActive()) {
            return;
        }
        const QSize zone = m_positioner->zoneSize();
        const QSize win = size();
        const QMargins ext = m_positioner->frameExtents();
        const int x = (zone.width() - win.width() - ext.left() - ext.right()) / 2;
        const int y = (zone.height() - win.height() - ext.top() - ext.bottom()) / 2;
        m_positioner->move(qMax(0, x), qMax(0, y));
    });
    addBtn(QStringLiteral("→"), 1, 2, [this] {
        if (m_positioner) {
            m_positioner->move(m_positioner->position() + QPoint(kStep, 0));
        }
    });

    addBtn(QStringLiteral("↙"), 2, 0, [this] {
        if (m_positioner) {
            m_positioner->move(m_positioner->position() + QPoint(-kStep, kStep));
        }
    });
    addBtn(QStringLiteral("↓"), 2, 1, [this] {
        if (m_positioner) {
            m_positioner->move(m_positioner->position() + QPoint(0, kStep));
        }
    });
    addBtn(QStringLiteral("↘"), 2, 2, [this] {
        if (m_positioner) {
            m_positioner->move(m_positioner->position() + QPoint(kStep, kStep));
        }
    });

    vbox->addWidget(moveGroup);

    // Corner shortcuts
    auto cornerGroup = new QGroupBox(QStringLiteral("Snap to corner"), central);
    auto cornerLayout = new QGridLayout(cornerGroup);

    auto cornerBtn = [&](const QString &label, int row, int col, QPoint offset) {
        auto btn = new QPushButton(label, cornerGroup);
        btn->setFixedSize(120, 32);
        cornerLayout->addWidget(btn, row, col, Qt::AlignCenter);
        connect(btn, &QPushButton::clicked, this, [this, offset] {
            if (!m_positioner) {
                return;
            }
            const QMargins ext = m_positioner->frameExtents();
            const QSize zone = m_positioner->zoneSize();
            const QSize win = size();
            int x = offset.x() >= 0 ? offset.x() : zone.width() - win.width() - ext.left() - ext.right() + offset.x();
            int y = offset.y() >= 0 ? offset.y() : zone.height() - win.height() - ext.top() - ext.bottom() + offset.y();
            m_positioner->move(qMax(0, x), qMax(0, y));
        });
    };

    cornerBtn(QStringLiteral("Top-left"), 0, 0, {0, 0});
    cornerBtn(QStringLiteral("Top-right"), 0, 1, {-1, 0});
    cornerBtn(QStringLiteral("Bottom-left"), 1, 0, {0, -1});
    cornerBtn(QStringLiteral("Bottom-right"), 1, 1, {-1, -1});

    vbox->addWidget(cornerGroup);
    vbox->addStretch();

    // Status bar
    statusBar()->showMessage(QStringLiteral("Waiting for compositor…"));
}

void DemoWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    if (!m_positioner) {
        // windowHandle() is valid after show()
        setupPositioner(windowHandle());
    }
}

void DemoWindow::setupPositioner(QWindow *window)
{
    if (m_positioner != nullptr)
        return;

    m_positioner = new Winpos::WindowPositioner(window, this);

    connect(m_positioner, &Winpos::WindowPositioner::positionChanged, this, [this] {
        updateLabels();
    });
    connect(m_positioner, &Winpos::WindowPositioner::zoneSizeChanged, this, [this] {
        updateLabels();
    });
    connect(m_positioner, &Winpos::WindowPositioner::positionFailed, this, [this] {
        onPositionFailed();
    });
    connect(m_positioner, &Winpos::WindowPositioner::zoneChanged, this, [this] {
        updateLabels();
    });

    connect(Winpos::Manager::instance(), &Winpos::Manager::activeChanged, this, [this] {
        updateLabels();
    });

    updateLabels();
}

void DemoWindow::updateLabels()
{
    const bool zonesActive = Winpos::Manager::instance()->isActive();
    const bool onWayland = QGuiApplication::platformName() == QStringLiteral("wayland");

    if (zonesActive) {
        m_statusLabel->setText(QStringLiteral("Protocol: active (version %1)").arg(Winpos::Manager::instance()->version()));
    } else if (onWayland) {
        m_statusLabel->setText(QStringLiteral("Protocol: <b>not supported</b> by this compositor"));
    } else {
        m_statusLabel->setText(QStringLiteral("Protocol: native positioning (%1)").arg(QGuiApplication::platformName()));
    }

    if (m_positioner && m_positioner->isActive()) {
        const QSize sz = m_positioner->zoneSize();
        m_zoneSizeLabel->setText(sz.width() == 0 || sz.height() == 0 ? QStringLiteral("Zone size: infinite (%1 x %2)").arg(sz.width()).arg(sz.height())
                                                                     : QStringLiteral("Zone size: %1 x %2").arg(sz.width()).arg(sz.height()));
        m_positionLabel->setText(QStringLiteral("Position: (%1, %2)").arg(m_positioner->position().x()).arg(m_positioner->position().y()));
        if (zonesActive) {
            statusBar()->showMessage(QStringLiteral("Zones protocol active"));
        } else if (!onWayland) {
            statusBar()->showMessage(QStringLiteral("Qt native positioning active"));
        }
    } else {
        m_zoneSizeLabel->setText(QStringLiteral("Zone size: unknown"));
        m_positionLabel->setText(QStringLiteral("Position: unknown"));
        if (onWayland && !zonesActive) {
            statusBar()->showMessage(QStringLiteral("xx-zones-v1 is not supported by this compositor!"));
        } else {
            statusBar()->showMessage(QStringLiteral("Waiting for zone assignment…"));
        }
    }
}

void DemoWindow::onPositionFailed()
{
    statusBar()->showMessage(QStringLiteral("Compositor rejected position request"), 3000);
}
