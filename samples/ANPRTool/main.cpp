#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "cameraitem.h"

int main( int argc, char *argv[] )
{
	QGuiApplication app( argc, argv );
	qmlRegisterType< CameraItem >( "OpenCV.Controls", 1, 0, "CameraItem" );
	QQmlApplicationEngine engine;
	engine.load( QUrl( QStringLiteral( "qrc:///main.qml" ) ) );
	return app.exec();
}

