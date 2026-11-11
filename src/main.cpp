// pyver — one window, one job: which installed Python version should be default.

#include "backend.h"
#include "cli.h"
#include "theme.h"
#include "translator.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

int main(int argc, char *argv[]) {
    // Commands, help and JSON output draw no window, so they must not need a display,
    // even where the desktop exports QT_QPA_PLATFORM=wayland.
    const bool command = argc == 1 || isCliCommand(QString::fromLocal8Bit(argv[1]));
    bool windowless = command;
    for (int i = 1; i < argc; ++i) {
        const QString argument = QString::fromLocal8Bit(argv[i]);
        windowless = windowless || argument.startsWith("--") || argument == "-h" || argument == "-v";
    }
    if (windowless)
        qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);
    app.setApplicationName("pyver");
    app.setApplicationVersion("0.1.0");
    app.setDesktopFileName("pyver");
    app.setWindowIcon(QIcon::fromTheme("pyver"));

    QQuickStyle::setStyle("Material");

    // Installed before anything is translated, and every qsTr() binding is
    // re-evaluated when the Omarchy language changes.
    Translator translator;
    app.installTranslator(&translator);

    Theme theme;
    Backend backend;

    QQmlApplicationEngine engine;
    QObject::connect(&translator, &Translator::changed, &engine, &QQmlEngine::retranslate);
    engine.rootContext()->setContextProperty("translator", &translator);
    engine.rootContext()->setContextProperty("theme", &theme);
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return 1;

    // The backend scans in its constructor, before the engine exists; scan again
    // now that the context properties are in place, so the window opens populated.
    backend.refresh();

    if (windowless)
        return runCli(app.arguments());

    return app.exec();
}