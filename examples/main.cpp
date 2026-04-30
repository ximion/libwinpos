/*
 * Copyright (C) 2025-2026 Matthias Klumpp <matthias@tenstral.net>
 *
 * SPDX-License-Identifier: MIT
 */

#include <QApplication>
#include <QCommandLineParser>

#include "demowindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("winpos-demo"));
    app.setApplicationVersion(QStringLiteral("0.1"));

    DemoWindow window;
    window.show();

    return app.exec();
}
