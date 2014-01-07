#include "engine.h"
#include <iostream>
#include <conio.h>

#include <opencv2/opencv.hpp>

#pragma warning( push )
#pragma warning( disable : 4127 4512 )
#include <QDir>
#pragma warning( pop )
#include <QCoreApplication>
#include <QThreadPool>
#include <QMutex>

#include "sym_recog.h"

struct debug_out : public recog_debug_callback
{
	virtual void out_image( const cv::Mat& ) { }
	virtual void out_string( const std::string& ) { }
};

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
		to_out << m_file_name.substr( m_folder_name.size() + 1, m_file_name.size() - m_folder_name.size() - 5 ) << "   ";
		const cv::Mat image = cv::imread( m_file_name.c_str(), CV_LOAD_IMAGE_COLOR );   // Read the file
		int sum = 0;
		if( image.data )
		{
			const int64 begin = cv::getTickCount();
			debug_out rc;
			const pair< string, int > number = read_number( image, &rc, 10 );
			to_out << number.first << "   " << number.second << "   " << (((double)cv::getTickCount() - begin)/cv::getTickFrequency());
			if ( m_file_name.find( number.first ) == string::npos )
			{
				to_out << "  !!  ";
			}
			to_out << endl;
			sum = number.second;
		}
		else
		{
			to_out << "FAILED READ FILE";
		}
		static QMutex out_locker;
		QMutexLocker lock( &out_locker );
		cout << to_out.str();
		m_sum += sum;
	}

	const std::string m_folder_name;
	const std::string m_file_name;
};

int process_file_task::m_sum = 0;

#include <Windows.h>

int main( int argc, char** argv )
{
	using namespace std;
	QCoreApplication a( argc, argv );

	if ( argc <= 1 )
	{
		cout << "usage: auto_test_desktop image_folder";
		return 1;
	}

	const string nn_config_folder( QDir::toNativeSeparators( QCoreApplication::applicationDirPath() + "/../../other" ).toLocal8Bit() );

	read_nn_config( nn_config_folder + "\\neural_net_num.yml", nn_config_folder + "\\neural_net_char.yml" );

	const int64 begin = cv::getTickCount();
	const string image_folder( argv[ 1 ] );
	// грузим список картинок
	
	QDir image_dir( image_folder.c_str() );
	if ( image_dir.exists() )
	{
		cout << "\"file\"  \"number\"  \"weight\"  \"time\"" << endl;
		const QStringList all_files = image_dir.entryList( QStringList() << "*.*", QDir::Files|QDir::Readable );
		QThreadPool::globalInstance()->setMaxThreadCount( 1 );
		for ( int nn = 0; nn < all_files.size(); ++nn )
		{
			const string next_file_name = image_folder + "\\" + all_files.at( nn ).toLocal8Bit().data();
			QThreadPool::globalInstance()->start( new process_file_task( image_folder, next_file_name ) );
		}
		QThreadPool::globalInstance()->waitForDone();
		cout << "sum: " << process_file_task::m_sum << " " << (((double)cv::getTickCount() - begin)/cv::getTickFrequency()) << endl;

		cv::waitKey();
	}
	else
	{
		cout << "Error, folder not exists";
	}
	return 0;
}