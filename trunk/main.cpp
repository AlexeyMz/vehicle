#include <QtQml/QQmlApplicationEngine>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtCore/QTranslator>

#include "gui/bridge.h"

using namespace vehicle::core;
using namespace vehicle::middleware;

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
	QLocale::setDefault(QLocale::system());

	QTranslator* qtTranslator = new QTranslator(&app);
	if(qtTranslator->load(qApp->applicationDirPath() + "/qt_ru.qm"))
		app.installTranslator(qtTranslator);
	else
		delete qtTranslator;

	QTranslator* translator = new QTranslator(&app);
	if(translator->load(qApp->applicationDirPath() + "/vehicle_ru.qm"))
		app.installTranslator(translator);
	else
		delete translator;

#ifdef _DEBUG
    qputenv("QML_IMPORT_TRACE", "1");
#endif

    QQmlApplicationEngine engine(&app);

    ModelQmlBridge bridge(&app);
    QString error;

    if(bridge.initialize(&engine, &error))
    {
        engine.load(QUrl("qrc:///gui/qml/main.qml"));
        return app.exec();
    }
    else
    {
        QMessageBox::critical(0, QObject::tr("Error"), error);
        return EXIT_FAILURE;
    }
}
