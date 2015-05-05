#include <QtQml/QQmlApplicationEngine>
#include <QtGui/QGuiApplication>

#include "gui/bridge.h"

using namespace vehicle::core;
using namespace vehicle::middleware;

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));

#ifdef _DEBUG
    qputenv("QML_IMPORT_TRACE", "1");
#endif

    QQmlApplicationEngine engine(&app);

    ModelQmlBridge bridge(&app);
    bridge.initialize(&engine);

    engine.load(QUrl("qrc:///gui/qml/main.qml"));

	return app.exec();
}
