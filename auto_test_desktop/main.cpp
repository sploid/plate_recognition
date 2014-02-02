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
#include <QDir>
#include <QApplication>
#include <QThreadPool>
#include <QMutex>
#include <QDebug>
#include <QTemporaryFile>
#include <QDialog>
#include <QPushButton>
#ifdef _MSC_VER
#pragma warning( pop )
#endif

#include "sym_recog.h"

class process_file_task : public QRunnable
{
public:
	process_file_task( const std::string& folder_name, const std::string& file_name )
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
		const string file_name_ext( m_file_name.substr( m_folder_name.size() + 1, m_file_name.size() - m_folder_name.size() - 1 ) );
		const string file_number( file_name_ext.substr( 0, file_name_ext.size() - 4 ) );
		string number_to_out( "[" );
		number_to_out += file_number + "](test_data/" + file_name_ext + ")";
		to_out << setw( 37 ) << setfill( ' ' ) << number_to_out << "|";
		const cv::Mat image = cv::imread( m_file_name.c_str(), CV_LOAD_IMAGE_COLOR );   // Read the file
		stringstream size_stream;
		size_stream << image.rows << "x" << image.cols;
		to_out << setw( 9 ) << setfill( ' ' ) << size_stream.str() << "|";
		int sum = 0;
		if( image.data )
		{
			const int64 begin = cv::getTickCount();
			const pair< string, int > number = read_number( image, 10 );
			if ( file_number != number.first )
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
			to_out << "FAILED READ FILE: " << QFile::exists( m_file_name.c_str() ) << " " << m_file_name.c_str();
		}
		static QMutex out_locker;
		QMutexLocker lock( &out_locker );
		cout << to_out.str();
		m_sum += sum;
	}

	const std::string m_folder_name;
	const std::string m_file_name;
};

class work_thread : public QThread
{
public:
	work_thread( const std::string& image_folder )
		: m_image_folder( image_folder )
	{
	}

protected:
	QString create_native_file( const QString& rc_path )
	{
		QTemporaryFile net_char( "net_char.yml" );
		if ( net_char.open() )
		{
			QFile rc_fl( rc_path );
			if ( rc_fl.open( QIODevice::ReadOnly ) )
			{
				net_char.setAutoRemove( false );
				net_char.write( rc_fl.readAll() );
				return net_char.fileName();
			}
		}
		return "";
	}

	virtual void run()
	{
		using namespace std;
		stringstream to_out;
		const QString net_char_file_name = create_native_file( ":/net/neural_net_char.yml" );
		const QString net_num_file_name = create_native_file( ":/net/neural_net_num.yml" );
		if ( !net_char_file_name.isEmpty() && !net_num_file_name.isEmpty() )
		{
			try
			{
				init_recognizer( net_num_file_name.toLocal8Bit().data(), net_char_file_name.toLocal8Bit().data() );
				const int64 begin = cv::getTickCount();
				// грузим список картинок

				QDir image_dir( m_image_folder.c_str() );
				if ( image_dir.exists() )
				{
					to_out << "\"file\"  \"number\"  \"weight\"  \"time\"" << endl;
					const QStringList all_files = image_dir.entryList( QStringList() << "*.*", QDir::Files|QDir::Readable );
					QThreadPool::globalInstance()->setMaxThreadCount( 2 ); // тут минимум 2, т.к. если будет 1, то потоки обработки не будут запускаться
					for ( int nn = 0; nn < all_files.size(); ++nn )
					{
						const string next_file_name = QDir::toNativeSeparators( QString::fromLocal8Bit( m_image_folder.c_str() ) + "/" + all_files.at( nn ) ).toLocal8Bit().data();
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
				qDebug() << "Catch exeption, what: " << e.what();
			}
		}
		if ( !net_char_file_name.isEmpty() )
		{
			QFile::remove( net_char_file_name );
		}
		if ( !net_num_file_name.isEmpty() )
		{
			QFile::remove( net_num_file_name );
		}
		cout << to_out.str();
	}
private:
	const std::string m_image_folder;
};

int process_file_task::m_sum = 0;

int main( int argc, char** argv )
{
	using namespace std;
	QApplication a( argc, argv );
#ifdef ANDROID
	std_cout_2_qdebug cc;
	(void)cc;
#endif // ANDROID

#ifndef ANDROID // for android hardcode image folder
	if ( argc <= 1 )
	{
		cout << "usage: auto_test_desktop image_folder";
		return 1;
	}
	const string image_folder( argv[ 1 ] );
#else
	const string image_folder( "/storage/sdcard0/imgs" );
#endif // ANDROID

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
}
