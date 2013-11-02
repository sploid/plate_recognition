#include "recog_test.h"
#include <QApplication>

int main( int argc, char *argv[] )
{
	QApplication a(argc, argv);
	QCoreApplication::setApplicationName( "RecogSoft10" );
	QCoreApplication::setOrganizationName( "Home" );
	recog_test w( 0 );
	w.show();
	return a.exec();
}
