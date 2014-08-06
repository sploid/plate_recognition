#include "engine.h"
#include "std_cout_2_qdebug.h"
#include <memory>
#include <iostream>
#include <iomanip>

#include <opencv2/opencv.hpp>

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4127 4512 4244 4251 4800 )
#endif
#include <QCoreApplication>
#include <QDir>
#include <QThreadPool>
#include <QMutex>
#include <QDebug>
#include <QTemporaryFile>
#ifdef QT_GUI_LIB
#include <QApplication>
#include <QDialog>
#include <QPushButton>
#endif // QT_GUI_LIB
#ifdef _MSC_VER
#pragma warning( pop )
#endif

#include "sym_recog.h"

class process_file_task : public QRunnable
{
public:
	process_file_task( const QString& folder_name, const QString& file_name )
		: m_folder_name( folder_name )
		, m_file_name( file_name )
	{
	}

	static int m_sum;

private:
	Q_DISABLE_COPY( process_file_task );

	void run()
	{
		using namespace std;
		stringstream to_out;
		const QFileInfo fi( m_file_name );
		const QString file_name_ext( fi.suffix() );
		const QString file_number( fi.baseName() );
		string number_to_out( "[" );
		number_to_out += file_number.toLocal8Bit() + "](test_data/" + file_name_ext.toLocal8Bit() + ")";
		to_out << setw( 37 ) << setfill( ' ' ) << number_to_out << "|";
		const cv::Mat image = cv::imread( m_file_name.toLocal8Bit().data(), CV_LOAD_IMAGE_COLOR );   // Read the file
		stringstream size_stream;
		size_stream << image.rows << "x" << image.cols;
		to_out << setw( 9 ) << setfill( ' ' ) << size_stream.str() << "|";
		int sum = 0;
		if( image.data )
		{
			const int64 begin = cv::getTickCount();
			const pair< string, int > number = read_number( image, 10 );
			if ( file_number.toLocal8Bit().data() != number.first )
			{

				to_out << setw( 13 ) << setfill( ' ' ) << ( string( "~~" ) + number.first + "~~" );
			}
			else
			{
				to_out << setw( 13 ) << setfill( ' ' ) << number.first;
			}
			to_out << " - "  << setw( 8 ) << setfill( ' ' ) << setprecision( 5 ) << (((double)cv::getTickCount() - begin)/cv::getTickFrequency()) << "|" << number.second << "   ";
			to_out << endl;
			sum = number.second;
		}
		else
		{
			to_out << "FAILED READ FILE: " << QFile::exists( m_file_name ) << " " << m_file_name.toLocal8Bit().data();
		}
		static QMutex out_locker;
		QMutexLocker lock( &out_locker );
		cout << to_out.str().c_str();
		m_sum += sum;
	}

	const QString m_folder_name;
	const QString m_file_name;
};

class work_thread : public QThread
{
public:
	work_thread( const QString& image_folder )
		: m_image_folder( image_folder )
	{
	}

protected:
	virtual void run()
	{
		using namespace std;
		stringstream to_out;
		try
		{
			init_recognizer();
			const int64 begin = cv::getTickCount();
			// грузим список картинок

			QDir image_dir( m_image_folder );
			if ( image_dir.exists() )
			{
				to_out << "\"file\"  \"number\"  \"weight\"  \"time\"" << endl;
				const QStringList all_files = image_dir.entryList( QStringList() << "*.*", QDir::Files|QDir::Readable );
				QThreadPool::globalInstance()->setMaxThreadCount( 2 ); // тут минимум 2, т.к. если будет 1, то потоки обработки не будут запускаться
				for ( int nn = 0; nn < all_files.size(); ++nn )
				{
					const QString next_file_name = m_image_folder + "/" + all_files.at( nn );
					QThreadPool::globalInstance()->start( new process_file_task( m_image_folder, next_file_name ) );
				}
				QThreadPool::globalInstance()->waitForDone();
				to_out << "sum: " << process_file_task::m_sum << " " << (((double)cv::getTickCount() - begin)/cv::getTickFrequency()) << endl;
			}
			else
			{
				to_out << "Error, folder not exists";
			}
		}
		catch ( const std::exception& e )
		{
			to_out << "Catch exeption, what: " << e.what();
		}
		cout << to_out.str().c_str();
	}
private:
	const QString m_image_folder;
};

int process_file_task::m_sum = 0;

void print_help()
{
	using namespace std;
	cout << "Plate number recognition" << endl;
	cout << "Usage: auto_test_desktop -i input [-oi out_im] [-os out_sym]" << endl << endl;
	cout << "Flags:" << endl;
	cout << "-i      sets folder with pictures for recognition" << endl;
//	cout << "-i      sets or folder with pictures or a single file for recognition" << endl;
//	cout << "-oi     specifies the target folder for pictures with the result" << endl;
//	cout << "-os     specifies the target folder for storing intermediate symbols" << endl << endl;
}

QString get_flag_value( const QString& key )
{
	const QStringList args = QCoreApplication::arguments();
	const int index_key = args.indexOf( key );
	return index_key != -1 && args.size() > index_key + 1 ? args[ index_key + 1 ] : "";
}

// TODO: Переписать вывод в классе process_file_task
int main( int argc, char** argv )
{
	using namespace std;
#ifdef ANDROID
	QApplication a( argc, argv );
	std_cout_2_qdebug cc;
	(void)cc;

	const string image_folder( "/storage/sdcard0/test_data" );

	QDialog *dlg = new QDialog();
	QPushButton* btn = new QPushButton( "WAIT", dlg );
	a.connect( btn, SIGNAL( clicked() ), SLOT( quit() ) );
	dlg->show();
	work_thread wt( image_folder.c_str() );
	a.connect( &wt, SIGNAL( finished() ), SLOT( quit() ) );
	wt.start();
	const int ret = a.exec();
	wt.wait();
	return ret;
#else
	QCoreApplication a( argc, argv );
	const QStringList app_args = QCoreApplication::arguments();
	if ( app_args.indexOf( "-h" ) != -1 || app_args.indexOf( "-help" ) != -1 || app_args.indexOf( "--help" ) != -1 || app_args.indexOf( "/?" ) != -1 )
	{
		print_help();
		return 1;
	}
	const QString image_folder = get_flag_value( "-i" );
	if ( image_folder.isEmpty() )
	{
		cout << "Invalid arguments" << endl << endl;
		print_help();
		return 1;
	}
	work_thread wt( image_folder );
	a.connect( &wt, SIGNAL( finished() ), SLOT( quit() ) );
	wt.start();
	const int ret = a.exec();
	wt.wait();
	return ret;
#endif // QT_GUI_LIB
}
