/*
 * Copyright (C) 2025-2026 Matthias Klumpp <matthias@tenstral.net>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <QMainWindow>

class QLabel;

namespace Winpos
{

class WindowPositioner;

}

class DemoWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit DemoWindow(QWidget *parent = nullptr);
    ~DemoWindow() override;

protected:
    void showEvent(QShowEvent *event) override;

private:
    void setupUi();
    void updateLabels();
    void setupPositioner(QWindow *window);
    void onPositionFailed();

    QLabel *m_statusLabel = nullptr;
    QLabel *m_zoneSizeLabel = nullptr;
    QLabel *m_positionLabel = nullptr;

    Winpos::WindowPositioner *m_positioner = nullptr;
};
