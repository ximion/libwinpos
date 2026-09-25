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

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Demo for zone-relative window positioning"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption restoreOption(QStringLiteral("restore"), QStringLiteral("Restore the previously saved window geometry on startup."));
    parser.addOption(restoreOption);
    parser.process(app);

    DemoWindow window;
    window.setRestoreOnStartup(parser.isSet(restoreOption));
    window.show();

    return app.exec();
}
